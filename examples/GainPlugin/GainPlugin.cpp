#include "GainPlugin.h"
#include "PluginEditor.h"

namespace
{
static const juce::String gainParamTag = "gain_db";
}

GainPlugin::GainPlugin()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      vts(*this, nullptr, juce::Identifier("Parameters"), createParameters())
{
    gainDBParameter = dynamic_cast<ModulatableFloatParameter *>(vts.getParameter(gainParamTag));
}

juce::AudioProcessorValueTreeState::ParameterLayout GainPlugin::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<ModulatableFloatParameter>(
        gainParamTag, "Gain", juce::NormalisableRange<float>{-30.0f, 30.0f}, 0.0f,
        [](float val) {
            juce::String gainStr = juce::String(val, 2, false);
            return gainStr + " dB";
        },
        [](const juce::String &s) { return s.getFloatValue(); }));

    return {params.begin(), params.end()};
}

bool GainPlugin::isBusesLayoutSupported(const juce::AudioProcessor::BusesLayout &layouts) const
{
    // only supports mono and stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // input and output layout must be the same
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void GainPlugin::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    gain.prepare(
        {sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)getMainBusNumOutputChannels()});
    gain.setRampDurationSeconds(0.05);
}

void GainPlugin::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &)
{
    auto &&block = juce::dsp::AudioBlock<float>{buffer};

    gain.setGainDecibels(gainDBParameter->getCurrentValue());
    gain.process(juce::dsp::ProcessContextReplacing<float>{block});
}

// bool GainPlugin::supportsDirectEvent(uint16_t space_id, uint16_t type)
//{
//     if (space_id != CLAP_CORE_EVENT_SPACE_ID)
//         return false;
//
//     return type == CLAP_EVENT_PARAM_MOD; // custom handling for parameter modulation events only
// }
//
// void GainPlugin::handleDirectEvent(const clap_event_header_t *event, int /*sampleOffset*/)
//{
//     if (event->space_id != CLAP_CORE_EVENT_SPACE_ID || event->type != CLAP_EVENT_PARAM_MOD)
//     {
//         // we should not be receiving events of this type!
//         jassertfalse;
//         return;
//     }
//
//     // custom handling for parameter modulation events:
//     auto paramModEvent = reinterpret_cast<const clap_event_param_mod *>(event);
//     auto *parameterVariant = static_cast<JUCEParameterVariant *>(paramModEvent->cookie);
//
//     if (auto *modulatableParam = parameterVariant->clapExtParameter)
//     {
//         if (paramModEvent->note_id >= 0)
//         {
//             if (!modulatableParam->supportsPolyphonicModulation())
//             {
//                 // The host is misbehaving! The host should know that this parameter does not
//                 // support polyphonic modulation, and should not have sent this event.
//                 jassertfalse;
//                 return;
//             }
//
//             // no polyphonic modulation at this time
//         }
//         else
//         {
//             if (!modulatableParam->supportsMonophonicModulation())
//             {
//                 // The host is misbehaving! The host should know that this parameter does not
//                 // support monophonic modulation, and should not have sent this event.
//                 jassertfalse;
//                 return;
//             }
//
//             modulatableParam->applyMonophonicModulation(paramModEvent->amount);
//         }
//     }
// }

juce::AudioProcessorEditor *GainPlugin::createEditor() { return new PluginEditor(*this); }

juce::String GainPlugin::getPluginTypeString() const
{
    if (wrapperType == juce::AudioProcessor::wrapperType_Undefined && is_clap)
        return "CLAP";

    return juce::AudioProcessor::getWrapperTypeDescription(wrapperType);
}

void GainPlugin::getStateInformation(juce::MemoryBlock &data)
{
    auto state = vts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, data);
}

void GainPlugin::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(vts.state.getType()))
            vts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// This creates new instances of the plugin
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new GainPlugin(); }
