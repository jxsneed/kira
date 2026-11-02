#pragma once
#include <JuceHeader.h>

class KIRAProcessor : public juce::AudioProcessor
{
public:
    KIRAProcessor();
    ~KIRAProcessor() override = default;
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState params;
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
    float estimateMidi(const juce::AudioBuffer<float>&);
    float quantiseMidi(float, int, int);
    double sampleRate = 44100.0;
    std::vector<float> delay[2];
    std::vector<std::vector<float>> echo;
    std::vector<float> pitchHistory;
    int pitchWrite = 0, pitchCounter = 0;
    int writePos = 0, echoPos = 0;
    float phase = 0.0f, detectedMidi = 60.0f, smoothRatio = 1.0f;
    float hpState[2] = {0.0f, 0.0f};
    float echoState[2] = {0.0f, 0.0f};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KIRAProcessor)
};
