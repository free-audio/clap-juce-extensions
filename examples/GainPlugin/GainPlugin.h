#pragma once

#include <juce_dsp/juce_dsp.h>
#include "ModulatableFloatParameter.h"
#include "ParamIndicationHelper.h"

class ModulatableFloatParameter;
class GainPlugin : public juce::AudioProcessor,
                   public clap_juce_extensions::clap_juce_audio_processor_capabilities,
                   protected clap_juce_extensions::clap_properties
{
  public:
    GainPlugin();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

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

    void updateTrackProperties(const TrackProperties &properties) override;

    bool supportsParamIndication() const noexcept override { return true; }
    void paramIndicationSetMapping(const juce::RangedAudioParameter &param, bool has_mapping,
                                   const juce::Colour *colour, const juce::String &label,
                                   const juce::String &description) noexcept override;
    void paramIndicationSetAutomation(const juce::RangedAudioParameter &param,
                                      uint32_t automation_state,
                                      const juce::Colour *colour) noexcept override;
    ParamIndicationHelper paramIndicationHelper;

    juce::String getPluginTypeString() const;
    auto *getGainParameter() { return gainDBParameter; }
    auto &getValueTreeState() { return vts; }
    const auto& getTrackProperties() const { return trackProperties; }

    std::function<void()> updateEditor = nullptr;

  private:
    ModulatableFloatParameter *gainDBParameter = nullptr;

    juce::AudioProcessorValueTreeState vts;

    juce::dsp::Gain<float> gain;

    TrackProperties trackProperties{};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainPlugin)
};
