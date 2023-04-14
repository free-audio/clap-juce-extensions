#include "PluginEditor.h"

struct SliderWithContextMenu : juce::Slider
{
    juce::AudioProcessorEditor &editor;
    juce::AudioProcessorParameter &param;

    explicit SliderWithContextMenu(juce::AudioProcessorEditor &ed,
                                   juce::AudioProcessorParameter &parameter)
        : editor(ed), param(parameter)
    {
    }

    void mouseDown(const juce::MouseEvent &e) override
    {
        if (e.mods.isPopupMenu())
        {
            if (auto *hostContext = editor.getHostContext())
            {
                if (auto menu = hostContext->getContextMenuForParameterIndex(&param))
                {
                    // If we wanted to show the native menu, we could do:
                    // menu->showNativeMenu(editor.getLocalBounds().getCentre());

                    // Instead we'll show a JUCE-style menu:
                    menu->getEquivalentPopupMenu().showMenuAsync(
                        juce::PopupMenu::Options().withMinimumWidth (500).withParentComponent(&editor));
                }
            }
            return;
        }

        juce::Slider::mouseDown(e);
    }
};

PluginEditor::PluginEditor(GainPlugin &plug) : juce::AudioProcessorEditor(plug), plugin(plug)
{
    auto *gainParameter = plugin.getGainParameter();

    gainSlider = std::make_unique<SliderWithContextMenu>(*this, *gainParameter);
    addAndMakeVisible(*gainSlider);
    gainSlider->setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    gainSlider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 20);

    sliderAttachment =
        std::make_unique<juce::SliderParameterAttachment>(*gainParameter, *gainSlider, nullptr);

    plugin.getValueTreeState().addParameterListener(gainParameter->paramID, this);

    setSize(300, 300);
}

PluginEditor::~PluginEditor()
{
    auto *gainParameter = plugin.getGainParameter();
    plugin.getValueTreeState().removeParameterListener(gainParameter->paramID, this);
}

void PluginEditor::resized()
{
    gainSlider->setBounds(juce::Rectangle<int>{200, 200}.withCentre(getLocalBounds().getCentre()));
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

void PluginEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (auto *hostContext = getHostContext())
        {
            if (auto menu = hostContext->getContextMenuForParameterIndex(nullptr))
            {
                // If we wanted to show the native menu, we could do:
                // menu->showNativeMenu(e.getPosition());

                // Instead we'll show a JUCE-style menu:
                menu->getEquivalentPopupMenu().showMenuAsync(
                    juce::PopupMenu::Options().withMinimumWidth (200).withParentComponent(this));
            }
        }
    }
}

void PluginEditor::parameterChanged(const juce::String &, float)
{
    // visual feedback so we know the parameter listeners are getting called:
    struct FlashComponent : Component
    {
        void paint(juce::Graphics &g) override { g.fillAll(juce::Colours::red); }
    } flashComp;

    addAndMakeVisible(flashComp);
    flashComp.setBounds(juce::Rectangle<int>{getWidth() - 10, 0, 10, 10});

    auto &animator = juce::Desktop::getInstance().getAnimator();
    animator.fadeOut(&flashComp, 100);
}
