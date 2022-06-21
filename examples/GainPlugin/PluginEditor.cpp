#include "PluginEditor.h"

PluginEditor::PluginEditor(GainPlugin &plug) : juce::AudioProcessorEditor(plug), plugin(plug)
{
    setSize(300, 300);

    addAndMakeVisible(gainSlider);
    gainSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);

    sliderAttachment = std::make_unique<juce::SliderParameterAttachment>(*plugin.getGainParameter(),
                                                                         gainSlider, nullptr);
}

void PluginEditor::resized()
{
    gainSlider.setBounds(juce::Rectangle<int>{200, 200}.withCentre(getLocalBounds().getCentre()));
}

void PluginEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::grey);

    g.setColour(juce::Colours::black);
    g.setFont(25.0f);
    const auto titleBounds = getLocalBounds().removeFromTop(30);
    const auto titleText = "Gain Plugin " + plugin.getPluginTypeString();
    g.drawFittedText(titleText, titleBounds, juce::Justification::centred, 1);
}
