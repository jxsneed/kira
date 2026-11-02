#include "PluginProcessor.h"
#include "PluginEditor.h"

class KIRALook : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float pos, float start, float end, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(8.0f);
        auto c = b.getCentre();
        const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.43f;
        g.setColour(juce::Colour(0xff19182b));
        g.fillEllipse(b);
        g.setColour(juce::Colour(0xff383451));
        g.drawEllipse(b, 2.0f);
        juce::Path arc;
        arc.addCentredArc(c.x, c.y, r, r, 0.0f, start, end, true);
        g.setColour(juce::Colour(0xff514b68));
        g.strokePath(arc, juce::PathStrokeType(3.0f));
        juce::Path value;
        value.addCentredArc(c.x, c.y, r, r, 0.0f, start, start + pos * (end - start), true);
        g.setColour(juce::Colour(0xffff78c8));
        g.strokePath(value, juce::PathStrokeType(4.0f));
        const float a = start + pos * (end - start);
        juce::Line<float> pointer(c.x + std::cos(a) * (r - 7.0f), c.y + std::sin(a) * (r - 7.0f),
                                  c.x + std::cos(a) * (r - 19.0f), c.y + std::sin(a) * (r - 19.0f));
        g.setColour(juce::Colours::white);
        g.drawLine(pointer, 2.5f);
        g.setColour(juce::Colour(0xfff6eaff));
        g.fillEllipse(c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
    }
};

KIRAEditor::KIRAEditor(KIRAProcessor& p) : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&look);
    setSize(650, 390);
    auto setup = [this](juce::Slider& s, const juce::String& name)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
        s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffffd8ed));
        s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        s.setName(name);
        addAndMakeVisible(s);
    };
    setup(bend, "BEND"); setup(tune, "TUNE"); setup(speed, "SPEED");
    setup(vibe, "VIBE"); setup(smile, "SMILE");
    bendAtt.reset(new SliderAttachment(processor.params, "bend", bend));
    tuneAtt.reset(new SliderAttachment(processor.params, "tune", tune));
    speedAtt.reset(new SliderAttachment(processor.params, "speed", speed));
    vibeAtt.reset(new SliderAttachment(processor.params, "vibe", vibe));
    smileAtt.reset(new SliderAttachment(processor.params, "smile", smile));

    key.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    scale.addItemList({"Chromatic","Major","Minor"}, 1);
    addAndMakeVisible(key); addAndMakeVisible(scale);
    key.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff211f36));
    scale.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff211f36));
    key.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    scale.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    keyAtt.reset(new ComboAttachment(processor.params, "root", key));
    scaleAtt.reset(new ComboAttachment(processor.params, "scale", scale));
}

KIRAEditor::~KIRAEditor() { setLookAndFeel(nullptr); }

void KIRAEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff100f1c));
    juce::ColourGradient bg(juce::Colour(0xff27213d), 0, 0, juce::Colour(0xff11101c), 650, 390, false);
    g.setGradientFill(bg); g.fillAll();
    g.setColour(juce::Colour(0xff3b3155)); g.drawRoundedRectangle(10, 10, 630, 370, 18, 1.5f);

    // Minimal anime-inspired mascot: hair silhouette, face, eyes and blush.
    g.setColour(juce::Colour(0xfff178bd));
    juce::Path hair; hair.addEllipse(22, 24, 76, 78); g.fillPath(hair);
    g.setColour(juce::Colour(0xffffdfec)); g.fillEllipse(35, 36, 52, 58);
    g.setColour(juce::Colour(0xff29243d));
    juce::Path fringe; fringe.startNewSubPath(34, 52); fringe.lineTo(39, 25); fringe.lineTo(53, 39);
    fringe.lineTo(68, 22); fringe.lineTo(84, 49); fringe.lineTo(76, 58); fringe.lineTo(68, 46);
    fringe.lineTo(54, 55); fringe.lineTo(44, 47); fringe.closeSubPath(); g.fillPath(fringe);
    g.setColour(juce::Colour(0xff5d4b9a));
    g.fillEllipse(47, 61, 5, 7); g.fillEllipse(68, 61, 5, 7);
    g.setColour(juce::Colour(0xffff8bbd));
    g.fillEllipse(39, 72, 10, 4); g.fillEllipse(72, 72, 10, 4);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(27.0f, juce::Font::bold)); g.drawText("KIRA", 112, 25, 160, 38, juce::Justification::left);
    g.setColour(juce::Colour(0xffff83c8)); g.setFont(juce::Font(10.5f));
    g.drawText("VOCAL SHIFT / TUNE / BLOOM", 114, 61, 260, 18, juce::Justification::left);
    g.setColour(juce::Colour(0xffb8aecb)); g.setFont(juce::Font(10.0f));
    g.drawText("01  •  LIVE VOICE UNIT", 475, 31, 145, 18, juce::Justification::right);

    const juce::String labels[] = {"BEND","TUNE","SPEED","VIBE","SMILE"};
    const juce::Slider* sliders[] = {&bend,&tune,&speed,&vibe,&smile};
    for (int i=0;i<5;++i)
    {
        int x = 24 + i*124;
        g.setColour(juce::Colour(0xffe8d9f0));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(labels[i], x, 143, 110, 20, juce::Justification::centred);
    }
    g.setColour(juce::Colour(0xffb8aecb));
    g.setFont(juce::Font(10.0f));
    g.drawText("KEY", 420, 328, 40, 20, juce::Justification::centred);
    g.drawText("SCALE", 520, 328, 50, 20, juce::Justification::centred);
    g.setColour(juce::Colour(0xfff178bd));
    g.drawText("KIRA-CHAN // DON'T BE SHY", 20, 355, 300, 16, juce::Justification::left);
}

void KIRAEditor::resized()
{
    const juce::Slider* unused[] = {};
    juce::Slider* sliders[] = {&bend,&tune,&speed,&vibe,&smile};
    for (int i=0;i<5;++i) sliders[i]->setBounds(24+i*124, 168, 110, 142);
    key.setBounds(455, 326, 58, 25);
    scale.setBounds(570, 326, 65, 25);
}
