#include <memory>

#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>

#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/format_types/juce_LegacyAudioParameter.cpp>


/*
 * This is a utility lock free queue based on the JUCE abstract fifo
 */
template <typename T, int qSize = 4096> class PushPopQ
{
  public:
    PushPopQ() : af(qSize) {}
    bool push(const T &ad)
    {
        auto ret = false;
        int start1, size1, start2, size2;
        af.prepareToWrite(1, start1, size1, start2, size2);
        if (size1 > 0)
        {
            dq[start1] = ad;
            ret = true;
        }
        af.finishedWrite(size1 + size2);
        return ret;
    }
    bool pop(T &ad)
    {
        bool ret = false;
        int start1, size1, start2, size2;
        af.prepareToRead(1, start1, size1, start2, size2);
        if (size1 > 0)
        {
            ad = dq[start1];
            ret = true;
        }
        af.finishedRead(size1 + size2);
        return ret;
    }
    juce::AbstractFifo af;
    T dq[qSize];
};

/*
 * The ClapJuceWrapper is a class which immplements a collection
 * of CLAP and JUCE APIs
 */
class ClapJuceWrapper : public clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Minimal>,
                        public juce::AudioProcessorListener,
                        public juce::AudioPlayHead,
                        public juce::AudioProcessorParameter::Listener,
                        public juce::ComponentListener
{
public:
    static clap_plugin_descriptor desc;
    std::unique_ptr<juce::AudioProcessor> processor;

    ClapJuceWrapper(const clap_host *host, juce::AudioProcessor *p)
    :clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Minimal>(&desc, host),
        processor(p)
    {
        processor->setRateAndBufferSizeDetails(0, 0);
        processor->setPlayHead(this);
        processor->addListener(this);

        const bool forceLegacyParamIDs = false;

        juceParameters.update(*processor, forceLegacyParamIDs);
        auto numParameters = juceParameters.getNumParameters();

        int i = 0;
        for (auto *juceParam : juceParameters.params)
        {
            uint32_t clap_id = generateClapIDForJuceParam(juceParam);

            allParams.insert(clap_id);
            paramMap[clap_id] = juceParam;
        }
    }

    uint32_t generateClapIDForJuceParam(juce::AudioProcessorParameter *param) const
    {
        auto juceParamID = juce::LegacyAudioParameter::getParamID(param, false);
        auto clap_id = static_cast<uint32_t>(juceParamID.hashCode());
        return clap_id;
    }

    void audioProcessorParameterChanged(juce::AudioProcessor *, int index, float newValue) override {
        auto pbi = juceParameters.getParamForIndex(index);
        auto id = generateClapIDForJuceParam(pbi); // a lookup obviously
        uiParamChangeQ.push({CLAP_EVENT_PARAM_VALUE, id, newValue});
    }
    void audioProcessorChanged(juce::AudioProcessor *processor, const ChangeDetails &details) override {}
    void audioProcessorParameterChangeGestureBegin (juce::AudioProcessor*, int index) override
    {
        auto pbi = juceParameters.getParamForIndex(index);
        auto id = generateClapIDForJuceParam(pbi);
        uiParamChangeQ.push({CLAP_EVENT_PARAM_BEGIN_ADJUST, id, 0});
    }

    void audioProcessorParameterChangeGestureEnd (juce::AudioProcessor*, int index) override
    {
        auto pbi = juceParameters.getParamForIndex(index);
        auto id = generateClapIDForJuceParam(pbi);
        uiParamChangeQ.push({CLAP_EVENT_PARAM_END_ADJUST, id, 0});
    }

    bool getCurrentPosition(juce::AudioPlayHead::CurrentPositionInfo &info) override { return false; }

    void parameterValueChanged(int, float newValue) override
    {
        // this can only come from the bypass parameter
    }

    void parameterGestureChanged(int, bool) override {}

  private:
    struct ParamChange
    {
        int type;
        uint32_t id;
        float newval;
    };
    PushPopQ<ParamChange, 4096*16> uiParamChangeQ;


    std::unordered_map<uint32_t, juce::AudioProcessorParameter *> paramMap;
    std::unordered_set<uint32_t> allParams;
    juce::LegacyAudioParametersWrapper juceParameters;

};

clap_plugin_descriptor ClapJuceWrapper::desc = {CLAP_VERSION,
                                                "fix.me.with.id",
                                                JucePlugin_Name,
                                                JucePlugin_Manufacturer,
                                                JucePlugin_ManufacturerWebsite,
                                                "FIXME",
                                                "FIXME",
                                                JucePlugin_VersionString,
                                                JucePlugin_Desc,
                                                "FIXME",
                                                CLAP_PLUGIN_INSTRUMENT};


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();


namespace ClapAdapter
{
bool clap_init(const char *p) { return true; }

void clap_deinit(void) {}

uint32_t clap_get_plugin_count() { return 1; }

const clap_plugin_descriptor *clap_get_plugin_descriptor(uint32_t w)
{
    return &ClapJuceWrapper::desc;
}


static const clap_plugin *clap_create_plugin(const clap_host *host, const char *plugin_id) {
    if (strcmp(plugin_id, ClapJuceWrapper::desc.id))
    {
        std::cout << "Warning: CLAP asked for plugin_id '" << plugin_id
                  << "' and JuceCLAPWrapper ID is '" << ClapJuceWrapper::desc.id << "'"
                  << std::endl;
        return nullptr;
    }
    auto* const pluginInstance = ::createPluginFilter();
    auto *wrapper = new ClapJuceWrapper(host, pluginInstance);
    return wrapper->clapPlugin();
}

static uint32_t clap_get_invalidation_sources_count(void) { return 0; }

static const clap_plugin_invalidation_source *clap_get_invalidation_sources(uint32_t index)
{
    return nullptr;
}

static void clap_refresh(void) {}
} // namespace ClapAdapter

extern "C"
{
    const CLAP_EXPORT struct clap_plugin_entry clap_plugin_entry = {
        CLAP_VERSION,
        ClapAdapter::clap_init,
        ClapAdapter::clap_deinit,
        ClapAdapter::clap_get_plugin_count,
        ClapAdapter::clap_get_plugin_descriptor,
        ClapAdapter::clap_create_plugin,
        ClapAdapter::clap_get_invalidation_sources_count,
        ClapAdapter::clap_get_invalidation_sources,
        ClapAdapter::clap_refresh,
    };
}
