#pragma once

#include "HostSpecificExtensionsPlugin.h"

class PluginEditor : public juce::AudioProcessorEditor
{
  public:
    explicit PluginEditor(HostSpecificExtensionsPlugin &plugin);
    ~PluginEditor() override = default;

    void resized() override;
    void paint(juce::Graphics &g) override;

  private:
    HostSpecificExtensionsPlugin &plugin;

    juce::TextButton changeTrackColour { "Change Track Colour" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
