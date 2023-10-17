#include "HostSpecificExtensionsPlugin.h"
#include "PluginEditor.h"

HostSpecificExtensionsPlugin::HostSpecificExtensionsPlugin()
    : juce::AudioProcessor(BusesProperties()
                               .withInput("Input", juce::AudioChannelSet::stereo(), true)
                               .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    // load extensions here!
    reaperPluginExtension =
        static_cast<const reaper_plugin_info_t *>(getExtension("cockos.reaper_extension"));
    jassert(reaperPluginExtension != nullptr || !juce::PluginHostType{}.isReaper());

    if (reaperPluginExtension != nullptr)
    {
        // we want to check that we can load/use the extensions in the plugin constructor.
        // for REAPER our silly test is to try muting track 0.
        using GetMasterTrackFunc = MediaTrack *(*)(ReaProject *);
        auto getMasterTrackFunc =
            reinterpret_cast<GetMasterTrackFunc>(reaperPluginExtension->GetFunc("GetMasterTrack"));
        auto *masterTrack = getMasterTrackFunc(nullptr);

        using SetMuteFunc = int (*)(MediaTrack *track, int mute, int igngroupflags);
        auto setMuteFunc =
            reinterpret_cast<SetMuteFunc>(reaperPluginExtension->GetFunc("SetTrackUIMute"));
        auto result = (*setMuteFunc)(masterTrack, 1, 0);
        jassert(result == 1);
    }
}

bool HostSpecificExtensionsPlugin::isBusesLayoutSupported(
    const juce::AudioProcessor::BusesLayout &layouts) const
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

void HostSpecificExtensionsPlugin::prepareToPlay(double, int) {}

void HostSpecificExtensionsPlugin::processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) {}

juce::AudioProcessorEditor *HostSpecificExtensionsPlugin::createEditor()
{
    return new PluginEditor(*this);
}

juce::String HostSpecificExtensionsPlugin::getPluginTypeString() const
{
    if (wrapperType == juce::AudioProcessor::wrapperType_Undefined && is_clap)
        return "CLAP";

    return juce::AudioProcessor::getWrapperTypeDescription(wrapperType);
}

void HostSpecificExtensionsPlugin::getStateInformation(juce::MemoryBlock &) {}

void HostSpecificExtensionsPlugin::setStateInformation(const void *, int) {}

// This creates new instances of the plugin
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new HostSpecificExtensionsPlugin();
}
