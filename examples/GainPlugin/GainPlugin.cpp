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

clap_process_status GainPlugin::clap_direct_process(const clap_process *process) noexcept
{
    const auto numSamples = (int)process->frames_count;
    auto events = process->in_events;
    auto numEvents = (int)events->size(events);
    int currentEvent = 0;
    int nextEventTime = numSamples;

    if (numEvents > 0)
    {
        auto event = events->get(events, 0);
        nextEventTime = (int)event->time;
    }

    // We process in place so...
    static constexpr uint32_t maxBuses = 128;
    std::array<float *, maxBuses> busses{};
    busses.fill(nullptr);
    juce::MidiBuffer midiBuffer;

    static constexpr int smallestBlockSize = 64;
    for (int n = 0; n < numSamples;)
    {
        const auto numSamplesToProcess =
            (numSamples - n >= smallestBlockSize)
                ? juce::jmax(nextEventTime - n,
                             smallestBlockSize) // process until next event, but no smaller than
                                                // smallest block size
                : (numSamples - n);             // process a few leftover samples

        while (nextEventTime < n + numSamplesToProcess && currentEvent < numEvents)
        {
            auto event = events->get(events, (uint32_t)currentEvent);
            process_clap_event(event);

            currentEvent++;
            nextEventTime = (currentEvent < numEvents)
                                ? (int)events->get(events, (uint32_t)currentEvent)->time
                                : numSamples;
        }

        uint32_t outputChannels = 0;
        for (uint32_t idx = 0; idx < process->audio_outputs_count && outputChannels < maxBuses;
             ++idx)
        {
            for (uint32_t ch = 0; ch < process->audio_outputs[idx].channel_count; ++ch)
            {
                busses[outputChannels] = process->audio_outputs[idx].data32[ch] + n;
                outputChannels++;
            }
        }

        uint32_t inputChannels = 0;
        for (uint32_t idx = 0; idx < process->audio_inputs_count && inputChannels < maxBuses; ++idx)
        {
            for (uint32_t ch = 0; ch < process->audio_inputs[idx].channel_count; ++ch)
            {
                auto *ic = process->audio_inputs[idx].data32[ch] + n;
                if (inputChannels < outputChannels)
                {
                    if (ic == busses[inputChannels])
                    {
                        // The buffers overlap - no need to do anything
                    }
                    else
                    {
                        juce::FloatVectorOperations::copy(busses[inputChannels], ic,
                                                          numSamplesToProcess);
                    }
                }
                else
                {
                    busses[inputChannels] = ic;
                }
                inputChannels++;
            }
        }

        auto totalChans = juce::jmax(inputChannels, outputChannels);
        juce::AudioBuffer<float> buffer(busses.data(), (int)totalChans, numSamplesToProcess);

        processBlock(buffer, midiBuffer);

        midiBuffer.clear();
        n += numSamplesToProcess;
    }

    // process any leftover events
    for (; currentEvent < numEvents; ++currentEvent)
    {
        auto event = events->get(events, (uint32_t)currentEvent);
        process_clap_event(event);
    }

    return CLAP_PROCESS_CONTINUE;
}

void GainPlugin::process_clap_event(const clap_event_header_t *event)
{
    if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
        return;

    switch (event->type)
    {
    case CLAP_EVENT_PARAM_VALUE:
    {
        auto paramEvent = reinterpret_cast<const clap_event_param_value *>(event);
        auto juceParameter = static_cast<juce::AudioProcessorParameter *>(paramEvent->cookie);

        if (juceParameter->getValue() == (float)paramEvent->value)
            return;

        juceParameter->setValueNotifyingHost((float)paramEvent->value);
    }
    break;
    case CLAP_EVENT_PARAM_MOD:
    {
        auto paramModEvent = reinterpret_cast<const clap_event_param_mod *>(event);
        auto *modulatableParam = static_cast<ModulatableFloatParameter *>(paramModEvent->cookie);
        if (paramModEvent->note_id >= 0)
        {
            // no polyphonic modulation
        }
        else
        {
            if (modulatableParam->supportsMonophonicModulation())
                modulatableParam->applyMonophonicModulation(paramModEvent->amount);
        }
    }
    break;
    default:
        break;
    }
}

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
