#include "PluginEditor.h"

PluginEditor::PluginEditor(HostSpecificExtensionsPlugin &plug)
    : juce::AudioProcessorEditor(plug), plugin(plug)
{
    addAndMakeVisible(changeTrackColour);
    changeTrackColour.setEnabled(plugin.reaperPluginExtension != nullptr);
    changeTrackColour.onClick = [reaperExt = plugin.reaperPluginExtension] {
        using GetTrackFunc = MediaTrack *(*)(ReaProject *, int);
        auto getTrackFunc = reinterpret_cast<GetTrackFunc>(reaperExt->GetFunc("GetTrack"));
        auto *track0 = getTrackFunc(nullptr, 0);
        if (track0 == nullptr)
            return;

        using ColorToNativeFunc = int (*)(int r, int g, int b);
        auto colorToNativeFunc =
            reinterpret_cast<ColorToNativeFunc>(reaperExt->GetFunc("ColorToNative"));

        using SetTrackColorFunc = void (*)(MediaTrack *track, int color);
        auto setTrackColorFunc =
            reinterpret_cast<SetTrackColorFunc>(reaperExt->GetFunc("SetTrackColor"));

        auto &rand = juce::Random::getSystemRandom();
        const auto red = rand.nextInt(256);
        const auto green = rand.nextInt(256);
        const auto blue = rand.nextInt(256);
        setTrackColorFunc(track0, colorToNativeFunc(red, green, blue));
    };

    setSize(300, 300);
}

void PluginEditor::resized()
{
    changeTrackColour.setBounds(juce::Rectangle{100, 35}.withCentre(getLocalBounds().getCentre()));
}

void PluginEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::grey);

    auto bounds = getLocalBounds();

    g.setColour(juce::Colours::black);
    g.setFont(25.0f);
    const auto titleText = "Host-Specific Extensions Plugin " + plugin.getPluginTypeString();
    g.drawFittedText(titleText, bounds.removeFromTop(30), juce::Justification::centred, 1);

    g.setFont(18.0f);
    const auto reaperExtText =
        "REAPER plugin extension: " +
        juce::String(plugin.reaperPluginExtension != nullptr ? "FOUND" : "NOT FOUND");
    g.drawFittedText(reaperExtText, bounds.removeFromTop(25), juce::Justification::centred, 1);
}
