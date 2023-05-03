#include "NoteNamesPlugin.h"

namespace
{
const juce::String noteNamesParamTag = "note_names";
}

NoteNamesPlugin::NoteNamesPlugin()
    : juce::AudioProcessor(
          BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      vts(*this, nullptr, juce::Identifier("Parameters"), createParameters()),
      noteNamesParamAttachment(*vts.getParameter(noteNamesParamTag),
                               [this](float)
                               {
                                   // This is annoying... when a JUCE parameter attachment is triggered
                                   // the parameter object itself is not guaranteed to have the up-to-date
                                   // value, so we'll have to delay a few milliseconds before alerting the host.
                                   juce::Timer::callAfterDelay (50, [this] { noteNamesChanged(); });
                               }),
      noteNamesParam(vts.getRawParameterValue(noteNamesParamTag))
{
    noteMaps[0] = {
        {60, "C"}, {62, "D"}, {64, "E"}, {65, "F"}, {67, "G"}, {69, "A"}, {71, "B"}, {72, "C"},
    };
    noteMaps[1] = {
        {60, "Do"}, {62, "Re"}, {64, "Mi"}, {65, "Fa"},
        {67, "So"}, {69, "La"}, {71, "Ti"}, {72, "Do"},
    };
    noteMaps[2] = {
        {60, "1"}, {62, "2"}, {64, "3"}, {65, "4"}, {67, "5"}, {69, "6"}, {71, "7"}, {72, "8"},
    };
}

juce::AudioProcessorValueTreeState::ParameterLayout NoteNamesPlugin::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        noteNamesParamTag, "Note Names", juce::StringArray{"Letters", "Do Re Mi", "Numbers"}, 0));

    return {params.begin(), params.end()};
}

int NoteNamesPlugin::noteNameCount() noexcept
{
    const auto noteNamesIndex = static_cast<size_t>(noteNamesParam->load());
    return (int)noteMaps[noteNamesIndex].size();
}

bool NoteNamesPlugin::noteNameGet(int index, clap_note_name *noteName) noexcept
{
    const auto noteNamesIndex = static_cast<size_t>(noteNamesParam->load());
    const auto &noteMap = noteMaps[noteNamesIndex];
    if (index < (int)noteMap.size())
    {
        const auto &note = noteMap[(size_t)index];
        strcpy(noteName->name, note.name.getCharPointer());
        noteName->key = (int16_t)note.key;
        noteName->channel = 1;
        noteName->port = -1;
        return true;
    }

    return false;
}

bool NoteNamesPlugin::isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &) const
{
    return true;
}

void NoteNamesPlugin::prepareToPlay(double, int) {}

void NoteNamesPlugin::processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) {}

juce::AudioProcessorEditor *NoteNamesPlugin::createEditor()
{
    return new juce::GenericAudioProcessorEditor{*this};
}

void NoteNamesPlugin::getStateInformation(juce::MemoryBlock &data)
{
    auto state = vts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, data);
}

void NoteNamesPlugin::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(vts.state.getType()))
            vts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This creates new instances of the plugin
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new NoteNamesPlugin(); }
