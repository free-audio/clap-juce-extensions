#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wunused-parameter", "-Wextra-semi", "-Wnon-virtual-dtor")
#include <clap-juce-extensions/clap-juce-extensions.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wc++98-compat-extra-semi",
                                    "-Wgnu-anonymous-struct",
                                    "-Wzero-as-null-pointer-constant",
                                    "-Wextra-semi",
                                    "-Wunused-parameter")
#include <reaper_plugin.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

class ModulatableFloatParameter;
class HostSpecificExtensionsPlugin : public juce::AudioProcessor,
                   public clap_juce_extensions::clap_juce_audio_processor_capabilities,
                   protected clap_juce_extensions::clap_properties
{
  public:
    HostSpecificExtensionsPlugin();

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return juce::String(); }
    void changeProgramName(int, const juce::String &) override {}

    bool isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &layouts) const override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    void processBlock(juce::AudioBuffer<double> &, juce::MidiBuffer &) override {}

    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor *createEditor() override;

    void getStateInformation(juce::MemoryBlock &data) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::String getPluginTypeString() const;

    const reaper_plugin_info_t* reaperPluginExtension = nullptr;

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HostSpecificExtensionsPlugin)
};
