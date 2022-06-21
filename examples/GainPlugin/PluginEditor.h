#pragma once

#include "GainPlugin.h"

class PluginEditor : public juce::AudioProcessorEditor
{
  public:
    explicit PluginEditor(GainPlugin &plugin);

    void resized() override;
    void paint(juce::Graphics &g) override;

  private:
    GainPlugin &plugin;

    juce::Slider gainSlider;
    std::unique_ptr<juce::SliderParameterAttachment> sliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
