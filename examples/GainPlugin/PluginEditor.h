#pragma once

#include "GainPlugin.h"

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::AudioProcessorValueTreeState::Listener,
                     private ParamIndicationHelper::Listener
{
  public:
    explicit PluginEditor(GainPlugin &plugin);
    ~PluginEditor() override;

    void resized() override;
    void paint(juce::Graphics &g) override;

    void paramIndicatorInfoChanged(const juce::RangedAudioParameter &) override;

  private:
    void parameterChanged(const juce::String &parameterID, float newValue) override;
    void mouseDown(const juce::MouseEvent &e) override;

    GainPlugin &plugin;

    std::unique_ptr<juce::Slider> gainSlider;
    std::unique_ptr<juce::SliderParameterAttachment> sliderAttachment;

    juce::ComponentBoundsConstrainer constrainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
