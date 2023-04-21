#pragma once

#include <unordered_map>
#include <juce_audio_processors/juce_audio_processors.h>

class ParamIndicationHelper
{
  public:
    ParamIndicationHelper() = default;

    struct ParamIndicatorInfo
    {
        juce::Colour colour;
        juce::String text;
    };

    const ParamIndicatorInfo *getParamIndicatorInfo(juce::RangedAudioParameter &param) const
    {
        const auto entry = indicatorInfoMap.find(&param);
        if (entry == indicatorInfoMap.end())
            return nullptr;
        return &entry->second;
    }

    void paramIndicationSetMapping(const juce::RangedAudioParameter &param, bool has_mapping,
                                   const juce::Colour *colour, const juce::String &label,
                                   const juce::String &description) noexcept
    {
        if (!has_mapping)
        {
            removeIndicatorInfo(param);
        }
        else
        {
            ParamIndicatorInfo info;
            info.colour = colour == nullptr ? juce::Colours::transparentBlack : *colour;
            info.text = label + ", " + description;
            indicatorInfoMap[&param] = std::move(info);
        }

        listeners.call(&Listener::paramIndicatorInfoChanged, param);
    }

    void paramIndicationSetAutomation(const juce::RangedAudioParameter &param,
                                      uint32_t automation_state,
                                      const juce::Colour *colour) noexcept
    {
        if (automation_state == CLAP_PARAM_INDICATION_AUTOMATION_NONE)
        {
            removeIndicatorInfo(param);
        }
        else
        {
            ParamIndicatorInfo info;
            info.colour = colour == nullptr ? juce::Colours::transparentBlack : *colour;
            info.text = "Automation: " + [automation_state]() -> juce::String {
                if (automation_state == CLAP_PARAM_INDICATION_AUTOMATION_PRESENT)
                    return "PRESENT";
                else if (automation_state == CLAP_PARAM_INDICATION_AUTOMATION_RECORDING)
                    return "RECORDING";
                else if (automation_state == CLAP_PARAM_INDICATION_AUTOMATION_PLAYING)
                    return "PLAYING";
                else if (automation_state == CLAP_PARAM_INDICATION_AUTOMATION_OVERRIDING)
                    return "OVERRIDING";
                return "";
            }();
            indicatorInfoMap[&param] = std::move(info);
        }

        listeners.call(&Listener::paramIndicatorInfoChanged, param);
    }

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void paramIndicatorInfoChanged(const juce::RangedAudioParameter &) {}
    };

    void addListener(Listener *listener) { listeners.add(listener); }
    void removeListener(Listener *listener) { listeners.remove(listener); }

  private:
    void removeIndicatorInfo(const juce::RangedAudioParameter &param)
    {
        const auto entry = indicatorInfoMap.find(&param);
        if (entry != indicatorInfoMap.end())
            indicatorInfoMap.erase(entry);
    }

    std::unordered_map<const juce::RangedAudioParameter *, ParamIndicatorInfo> indicatorInfoMap;

    juce::ListenerList<Listener> listeners;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ParamIndicationHelper)
};
