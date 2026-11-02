#pragma once
#include <JuceHeader.h>

class KIRAProcessor;

class KIRAEditor : public juce::AudioProcessorEditor
{
public:
    explicit KIRAEditor(KIRAProcessor&);
    ~KIRAEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    KIRAProcessor& processor;
    juce::Slider bend, tune, speed, vibe, smile;
    juce::ComboBox key, scale;
    std::unique_ptr<SliderAttachment> bendAtt, tuneAtt, speedAtt, vibeAtt, smileAtt;
    std::unique_ptr<ComboAttachment> keyAtt, scaleAtt;
    juce::LookAndFeel_V4 look;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KIRAEditor)
};
