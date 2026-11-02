#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

static constexpr float kPi = 3.14159265358979323846f;

KIRAProcessor::KIRAProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      params(*this, nullptr, "PARAMS", createParams())
#endif
{
}

juce::AudioProcessorValueTreeState::ParameterLayout KIRAProcessor::createParams()
{
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<P>("bend", "BEND", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<P>("tune", "TUNE", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 65.0f));
    layout.add(std::make_unique<P>("speed", "SPEED", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 75.0f));
    layout.add(std::make_unique<P>("vibe", "VIBE", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 18.0f));
    layout.add(std::make_unique<P>("smile", "SMILE", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<juce::AudioParameterChoice>("root", "KEY",
        juce::StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("scale", "SCALE",
        juce::StringArray{"Chromatic","Major","Minor"}, 2));
    return layout;
}

void KIRAProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;
    const int maxDelay = 8192;
    for (auto& d : delay) d.assign((size_t)maxDelay, 0.0f);
    writePos = 0;
    pitchHistory.assign(2048, 0.0f);
    pitchWrite = 0; pitchCounter = 0;
    phase = 0.0f;
    detectedMidi = 60.0f;
    smoothRatio = 1.0f;
    hpState = {0.0f, 0.0f};
    echoState = {0.0f, 0.0f};
    echo.assign(2, std::vector<float>(static_cast<size_t>(sr * 0.45) + 8, 0.0f));
    echoPos = 0;
}

void KIRAProcessor::releaseResources() {}

bool KIRAProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
            || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo());
}

float KIRAProcessor::estimateMidi(const juce::AudioBuffer<float>&)
{
    const int n = (int)pitchHistory.size();
    const int minLag = (int)(sampleRate / 1000.0);
    const int maxLag = juce::jmin((int)(sampleRate / 75.0), n / 2);
    float best = 0.0f;
    int bestLag = 0;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double corr = 0.0, e1 = 0.0, e2 = 0.0;
        for (int i = 0; i < n - lag; i += 4)
        {
            const float a = pitchHistory[(size_t)((pitchWrite + i) % n)];
            const float b = pitchHistory[(size_t)((pitchWrite + i + lag) % n)];
            corr += a * b; e1 += a * a; e2 += b * b;
        }
        const float norm = (float)(corr / std::sqrt(e1 * e2 + 1.0e-12));
        if (norm > best) { best = norm; bestLag = lag; }
    }
    if (bestLag > 0 && best > 0.35f)
        return 69.0f + 12.0f * std::log2((float)(sampleRate / bestLag) / 440.0f);
    return detectedMidi;
}

float KIRAProcessor::quantiseMidi(float midi, int root, int scale)
{
    if (scale == 0) return midi;
    static const int major[] = {0,2,4,5,7,9,11};
    static const int minor[] = {0,2,3,5,7,8,10};
    const int* notes = scale == 1 ? major : minor;
    const int count = 7;
    const int nearest = (int)std::round(midi);
    float best = (float)nearest;
    float bestDistance = 100.0f;
    for (int candidate = nearest - 2; candidate <= nearest + 2; ++candidate)
    {
        int pc = ((candidate - root) % 12 + 12) % 12;
        bool allowed = false;
        for (int i = 0; i < count; ++i) if (pc == notes[i]) allowed = true;
        const float dist = std::abs(midi - candidate);
        if (allowed && dist < bestDistance) { bestDistance = dist; best = (float)candidate; }
    }
    return best;
}

void KIRAProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels();
    const int n = buffer.getNumSamples();
    auto get = [this](const char* id) { return params.getRawParameterValue(id)->load(); };

    const float bend = get("bend");
    const float tune = get("tune") * 0.01f;
    const float speed = get("speed") * 0.01f;
    const float vibe = get("vibe") * 0.01f;
    const float smile = get("smile") * 0.01f;
    const int root = (int)get("root");
    const int scale = (int)get("scale");

    // Maintain a rolling analysis window so pitch tracking works with small Ableton buffers too.
    for (int i = 0; i < n; ++i)
    {
        pitchHistory[(size_t)pitchWrite] = buffer.getSample(0, i);
        pitchWrite = (pitchWrite + 1) % (int)pitchHistory.size();
        ++pitchCounter;
    }
    if (pitchCounter >= 1024)
    {
        detectedMidi = estimateMidi(buffer);
        pitchCounter = 0;
    }
    const float targetMidi = quantiseMidi(detectedMidi, root, scale);
    const float correction = (targetMidi - detectedMidi) * tune;
    const float targetRatio = std::pow(2.0f, (bend + correction) / 12.0f);
    const float smoothing = 0.002f + (1.0f - speed) * 0.08f;

    const int delaySize = (int)delay[0].size();
    const float baseDelay = 1700.0f;
    const float depth = 1150.0f;
    const float phaseRate = juce::jlimit(0.00002f, 0.004f, std::abs(1.0f - targetRatio) / depth);
    const int echoDelay = juce::jlimit(1, (int)echo[0].size() - 1, (int)(sampleRate * (0.16 + 0.12 * smile)));

    for (int i = 0; i < n; ++i)
    {
        smoothRatio += (targetRatio - smoothRatio) * smoothing;
        phase += phaseRate;
        if (phase >= 1.0f) phase -= 1.0f;

        for (int ch = 0; ch < channels; ++ch)
        {
            const float in = buffer.getSample(ch, i);
            delay[(size_t)ch][(size_t)writePos] = in;

            // Two crossfaded moving delay heads provide a compact real-time pitch shifter.
            const float p1 = phase;
            const float p2 = std::fmod(phase + 0.5f, 1.0f);
            const float d1 = baseDelay + p1 * depth;
            const float d2 = baseDelay + p2 * depth;
            auto read = [&](float d)
            {
                float pos = (float)writePos - d;
                while (pos < 0.0f) pos += (float)delaySize;
                const int a = (int)pos;
                const int b = (a + 1) % delaySize;
                const float f = pos - (float)a;
                return delay[(size_t)ch][(size_t)a] * (1.0f - f) + delay[(size_t)ch][(size_t)b] * f;
            };
            const float w1 = 0.5f - 0.5f * std::cos(2.0f * kPi * p1);
            const float w2 = 1.0f - w1;
            float shifted = read(d1) * w1 + read(d2) * w2;
            if (std::abs(smoothRatio - 1.0f) < 0.002f) shifted = in;

            // VIBE adds a restrained animated pitch wobble.
            const float wobble = 1.0f + std::sin((float)i * 0.002f + (float)writePos * 0.0003f) * vibe * 0.0018f;
            float out = shifted * wobble;

            // SMILE: rising high-pass + tempo-free echo bloom + soft saturation.
            const float hpCut = 25.0f + smile * 1500.0f;
            const float hpA = std::exp(-2.0f * kPi * hpCut / (float)sampleRate);
            hpState[(size_t)ch] = hpA * hpState[(size_t)ch] + (1.0f - hpA) * out;
            out = out - hpState[(size_t)ch] * (0.15f + smile * 0.72f);

            float delayed = echo[(size_t)ch][(size_t)((echoPos + (int)echo[0].size() - echoDelay) % (int)echo[0].size())];
            const float feedback = smile * 0.58f;
            echo[(size_t)ch][(size_t)echoPos] = out + delayed * feedback;
            out += delayed * smile * 0.55f;
            out = std::tanh(out * (1.0f + smile * 1.8f)) / (1.0f + smile * 0.15f);

            // Dry/wet blend keeps the effect usable at low settings.
            const float wet = juce::jlimit(0.0f, 1.0f, 0.25f + tune * 0.55f + std::abs(bend) / 24.0f);
            buffer.setSample(ch, i, in * (1.0f - wet) + out * wet);
        }
        writePos = (writePos + 1) % delaySize;
        echoPos = (echoPos + 1) % (int)echo[0].size();
    }
}

juce::AudioProcessorEditor* KIRAProcessor::createEditor() { return new KIRAEditor(*this); }
bool KIRAProcessor::hasEditor() const { return true; }
const juce::String KIRAProcessor::getName() const { return JucePlugin_Name; }
bool KIRAProcessor::acceptsMidi() const { return false; }
bool KIRAProcessor::producesMidi() const { return false; }
bool KIRAProcessor::isMidiEffect() const { return false; }
double KIRAProcessor::getTailLengthSeconds() const { return 2.0; }
int KIRAProcessor::getNumPrograms() { return 1; }
int KIRAProcessor::getCurrentProgram() { return 0; }
void KIRAProcessor::setCurrentProgram(int) {}
const juce::String KIRAProcessor::getProgramName(int) { return {}; }
void KIRAProcessor::changeProgramName(int, const juce::String&) {}
void KIRAProcessor::getStateInformation(juce::MemoryBlock& dest) { auto s = params.copyState(); std::unique_ptr<juce::XmlElement> xml(s.createXml()); copyXmlToBinary(*xml, dest); }
void KIRAProcessor::setStateInformation(const void* data, int size) { std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size)); if (xml && xml->hasTagName(params.state.getType())) params.replaceState(juce::ValueTree::fromXml(*xml)); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KIRAProcessor(); }
