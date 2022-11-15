#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wunused-parameter")
#include <clap-juce-extensions/clap-juce-extensions.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

class NoteNamesPlugin : public juce::AudioProcessor,
                        public clap_juce_extensions::clap_juce_audio_processor_capabilities
{
  public:
    NoteNamesPlugin();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    bool supportsNoteName() const noexcept override { return true; }
    int noteNameCount() noexcept override;
    bool noteNameGet(int index, clap_note_name *noteName) noexcept override;

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
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

  private:
    juce::AudioProcessorValueTreeState vts;
    juce::ParameterAttachment noteNamesParamAttachment;
    std::atomic<float> *noteNamesParam = nullptr;

    struct NoteName
    {
        int key = -1;
        juce::String name{};
    };

    using NoteNameMap = std::vector<NoteName>;

    static constexpr size_t numNoteMaps = 3;
    std::array<NoteNameMap, numNoteMaps> noteMaps;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteNamesPlugin)
};
