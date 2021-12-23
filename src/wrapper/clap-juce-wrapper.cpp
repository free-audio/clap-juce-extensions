/*
 * BaconPaul's running todo
 *
 * - We always say we are an instrument....
 * - midi out (try stochas perhaps?)
 * - why does dexed not work?
 * - Finish populating the desc
 * - Cleanup and comment of course (including the CMake) including what's skipped
 */

#include <memory>

#include <clap/helpers/host-proxy.hh>
#include <clap/helpers/host-proxy.hxx>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>

#define FIXME(x)                                                                                   \
    {                                                                                              \
        static bool onetime_ = false;                                                              \
        if (!onetime_)                                                                             \
        {                                                                                          \
            std::ostringstream oss;                                                                \
            oss << "FIXME: " << x << " @" << __LINE__;                                             \
            DBG(oss.str());                                                                        \
        }                                                                                          \
        jassert(onetime_);                                                                         \
        onetime_ = true;                                                                           \
    }

#define JUCE_GUI_BASICS_INCLUDE_XHEADERS 1
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/format_types/juce_LegacyAudioParameter.cpp>

#include <clap-juce-extensions/clap-juce-extensions.h>

#if JUCE_LINUX
#include <juce_audio_plugin_client/utility/juce_LinuxMessageThread.h>
#endif

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
 * These functions are the JUCE VST2/3 NSView attachment functions. We compile them into
 * our clap dll by, on macos, also linking clap_juce_mac.mm
 */
namespace juce
{
extern JUCE_API void initialiseMacVST();
extern JUCE_API void *attachComponentToWindowRefVST(Component *, void *parentWindowOrView,
                                                    bool isNSView);
} // namespace juce

/*
 * The ClapJuceWrapper is a class which immplements a collection
 * of CLAP and JUCE APIs
 */
class ClapJuceWrapper : public clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                                                     clap::helpers::CheckingLevel::Minimal>,
                        public juce::AudioProcessorListener,
                        public juce::AudioPlayHead,
                        public juce::AudioProcessorParameter::Listener,
                        public juce::ComponentListener
{
  public:
    static clap_plugin_descriptor desc;
    std::unique_ptr<juce::AudioProcessor> processor;
    clap_juce_extensions::clap_properties *processorAsClapProperties{nullptr};

    ClapJuceWrapper(const clap_host *host, juce::AudioProcessor *p)
        : clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate,
                                clap::helpers::CheckingLevel::Minimal>(&desc, host),
          processor(p)
    {
        processor->setRateAndBufferSizeDetails(0, 0);
        processor->setPlayHead(this);
        processor->addListener(this);

        processorAsClapProperties = dynamic_cast<clap_juce_extensions::clap_properties *>(p);

        const bool forceLegacyParamIDs = false;

        juceParameters.update(*processor, forceLegacyParamIDs);

        int i = 0;
        for (auto *juceParam :
#if JUCE_VERSION >= 0x060103
             juceParameters
#else
             juceParameters.params
#endif

        )
        {
            uint32_t clap_id = generateClapIDForJuceParam(juceParam);

            allClapIDs.insert(clap_id);
            paramPtrByClapID[clap_id] = juceParam;
            clapIDByParamPtr[juceParam] = clap_id;
        }
    }

    ~ClapJuceWrapper()
    {
#if JUCE_LINUX
        if (_host.canUseTimerSupport())
        {
            _host.timerSupportUnregisterTimer(idleTimer);
        }
#endif
    }

    bool init() noexcept override
    {

#if JUCE_LINUX
        if (_host.canUseTimerSupport())
        {
            _host.timerSupportRegisterTimer(1000 / 50, &idleTimer);
        }
#endif
        return true;
    }

  public:
    bool implementsTimerSupport() const noexcept override { return true; }
    void onTimer(clap_id timerId) noexcept override
    {
#if LINUX
        juce::ScopedJuceInitialiser_GUI libraryInitialiser;
        const juce::MessageManagerLock mmLock;

        while (juce::dispatchNextMessageOnSystemQueue(true))
        {
        }
#endif
    }

    clap_id idleTimer{0};

    uint32_t generateClapIDForJuceParam(juce::AudioProcessorParameter *param) const
    {
        auto juceParamID = juce::LegacyAudioParameter::getParamID(param, false);
        auto clap_id = static_cast<uint32_t>(juceParamID.hashCode());
        return clap_id;
    }

    void audioProcessorChanged(juce::AudioProcessor *processor,
                               const ChangeDetails &details) override
    {
        FIXME("audio processor changed");
    }

    clap_id clapIdFromParameterIndex(int index)
    {
        auto pbi = juceParameters.getParamForIndex(index);
        auto pf = clapIDByParamPtr.find(pbi);
        if (pf != clapIDByParamPtr.end())
            return pf->second;

        auto id = generateClapIDForJuceParam(pbi); // a lookup obviously
        return id;
    }

    void audioProcessorParameterChanged(juce::AudioProcessor *, int index, float newValue) override
    {
        auto id = clapIdFromParameterIndex(index);
        uiParamChangeQ.push({CLAP_EVENT_PARAM_VALUE, id, newValue});
    }

    void audioProcessorParameterChangeGestureBegin(juce::AudioProcessor *, int index) override
    {
        auto id = clapIdFromParameterIndex(index);
        uiParamChangeQ.push({CLAP_EVENT_PARAM_BEGIN_ADJUST, id, 0});
    }

    void audioProcessorParameterChangeGestureEnd(juce::AudioProcessor *, int index) override
    {
        auto id = clapIdFromParameterIndex(index);
        uiParamChangeQ.push({CLAP_EVENT_PARAM_END_ADJUST, id, 0});
    }

    /*
     * According to the JUCE docs this is *only* called on the processing thread
     */
    bool getCurrentPosition(juce::AudioPlayHead::CurrentPositionInfo &info) override
    {
        if (hasTransportInfo && transportInfo)
        {
            auto flags = transportInfo->flags;

            if (flags & CLAP_TRANSPORT_HAS_TEMPO)
                info.bpm = transportInfo->tempo;
            if (flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE)
            {
                info.timeSigNumerator = transportInfo->tsig_num;
                info.timeSigDenominator = transportInfo->tsig_denom;
            }

            if (flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE)
            {
                info.ppqPosition = 1.0 * transportInfo->song_pos_beats / CLAP_BEATTIME_FACTOR;
                info.ppqPositionOfLastBarStart =
                    1.0 * transportInfo->bar_start / CLAP_BEATTIME_FACTOR;
            }
            if (flags & CLAP_TRANSPORT_HAS_SECONDS_TIMELINE)
            {
                info.timeInSeconds = 1.0 * transportInfo->song_pos_seconds / CLAP_SECTIME_FACTOR;
            }
            info.isPlaying = flags & CLAP_TRANSPORT_IS_PLAYING;
            info.isRecording = flags & CLAP_TRANSPORT_IS_RECORDING;
            info.isLooping = flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE;
        }
        return hasTransportInfo;
    }

    void parameterValueChanged(int, float newValue) override
    {
        FIXME("parameter value changed");
        // this can only come from the bypass parameter
    }

    void parameterGestureChanged(int, bool) override { FIXME("parameter gesture changed"); }

    bool activate(double sampleRate, uint32_t minFrameCount,
                  uint32_t maxFrameCount) noexcept override
    {
        processor->prepareToPlay(sampleRate, maxFrameCount);
        return true;
    }

    /* CLAP API */

    bool implementsAudioPorts() const noexcept override { return true; }
    uint32_t audioPortsCount(bool isInput) const noexcept override
    {
        DBG("audioPortsCount - for " << (isInput ? "Input" : "Output") << " returning "
                                     << processor->getBusCount(isInput));
        return processor->getBusCount(isInput);
    }

    bool audioPortsInfo(uint32_t index, bool isInput,
                        clap_audio_port_info *info) const noexcept override
    {
        // For now hardcode to stereo out. Fix this obviously.
        DBG("audioPortsInfo " << (int)index << " " << (isInput ? "INPUT" : "OUTPUT"));
        auto clob = processor->getChannelLayoutOfBus(isInput, index);
        auto bus = processor->getBus(isInput, index);

        // For now we only support stereo channels
        jassert(clob.size() == 1 || clob.size() == 2);
        jassert(clob.size() == 1 || (clob.getTypeOfChannel(0) == juce::AudioChannelSet::left &&
                                     clob.getTypeOfChannel(1) == juce::AudioChannelSet::right));
        // if (isInput || index != 0) return false;
        info->id = (isInput ? 1 << 15 : 1) + index;
        strncpy(info->name, bus->getName().toRawUTF8(), sizeof(info->name));
        DBG("Constructing port '" << bus->getName() << "'");
        info->is_main = (index == 0);
        info->is_cv = false;

        FIXME("Float vs Double Precisions busses");
        info->sample_size = 32;
        info->in_place = true;
        info->channel_count = clob.size();
        info->channel_map = clob.size() == 1 ? CLAP_CHMAP_MONO : CLAP_CHMAP_STEREO;

        FIXME("Channel Set; and this threading of bus layout");
        auto requested = processor->getBusesLayout();
        if (clob.size() == 1)
            requested.getChannelSet(isInput, index) = juce::AudioChannelSet::mono();
        if (clob.size() == 2)
            requested.getChannelSet(isInput, index) = juce::AudioChannelSet::stereo();
        processor->setBusesLayoutWithoutEnabling(requested);

        return true;
    }
    uint32_t audioPortsConfigCount() const noexcept override
    {
        DBG("audioPortsConfigCount CALLED - returning 0Por");
        return 0;
    }
    bool audioPortsGetConfig(uint32_t index,
                             clap_audio_ports_config *config) const noexcept override
    {
        return false;
    }
    bool audioPortsSetConfig(clap_id configId) noexcept override { return false; }

    bool implementsParams() const noexcept override { return true; }
    bool isValidParamId(clap_id paramId) const noexcept override
    {
        return allClapIDs.find(paramId) != allClapIDs.end();
    }
    uint32_t paramsCount() const noexcept override { return allClapIDs.size(); }
    bool paramsInfo(int32_t paramIndex, clap_param_info *info) const noexcept override
    {
        auto pbi = juceParameters.getParamForIndex(paramIndex);

        auto *parameterGroup = processor->getParameterTree().getGroupsForParameter(pbi).getLast();
        juce::String group = "";
        while (parameterGroup && parameterGroup->getParent() &&
               parameterGroup->getParent()->getName().isNotEmpty())
        {
            group = parameterGroup->getName() + "/" + group;
            parameterGroup = parameterGroup->getParent();
        }

        if (group.isNotEmpty())
            group = "/" + group;

        // Fixme - using parameter groups here would be lovely but until then
        info->id = generateClapIDForJuceParam(pbi);
        strncpy(info->name, (pbi->getName(CLAP_NAME_SIZE)).toRawUTF8(), CLAP_NAME_SIZE);
        strncpy(info->module, group.toRawUTF8(), CLAP_NAME_SIZE);

        info->min_value = 0; // FIXME
        info->max_value = 1;
        info->default_value = pbi->getDefaultValue();
        info->cookie = pbi;

        return true;
    }

    bool paramsValue(clap_id paramId, double *value) noexcept override
    {
        auto pbi = paramPtrByClapID[paramId];
        *value = pbi->getValue();
        return true;
    }

    clap_process_status process(const clap_process *process) noexcept override
    {
        auto ev = process->in_events;
        auto sz = ev->size(ev);

        // Since the playhead is *only* good inside juce audio processor process,
        // we can just keep this little transient pointer here
        if (process->transport)
        {
            hasTransportInfo = true;
            transportInfo = process->transport;
        }
        else
        {
            hasTransportInfo = false;
            transportInfo = nullptr;
        }

        if (processorAsClapProperties)
            processorAsClapProperties->clap_transport = process->transport;

        juce::MidiBuffer mbuf;
        if (sz != 0)
        {
            for (auto i = 0; i < sz; ++i)
            {
                auto evt = ev->get(ev, i);

                switch (evt->type)
                {
                case CLAP_EVENT_NOTE_ON:
                {
                    auto n = evt->note;

                    mbuf.addEvent(
                        juce::MidiMessage::noteOn(n.channel + 1, n.key, (float)n.velocity),
                        evt->time);
                }
                break;
                case CLAP_EVENT_NOTE_OFF:
                {
                    auto n = evt->note;
                    mbuf.addEvent(
                        juce::MidiMessage::noteOff(n.channel + 1, n.key, (float)n.velocity),
                        evt->time); // how to get time
                }
                break;
                case CLAP_EVENT_MIDI:
                {
                    mbuf.addEvent(juce::MidiMessage(evt->midi.data[0], evt->midi.data[1],
                                                    evt->midi.data[2], evt->time),
                                  evt->time);
                }
                break;
                case CLAP_EVENT_TRANSPORT:
                {
                    // handle this case
                }
                break;
                case CLAP_EVENT_PARAM_VALUE:
                {
                    auto v = evt->param_value;

                    auto id = v.param_id;
                    auto nf = v.value;
                    jassert(v.cookie == paramPtrByClapID[id]);
                    auto jp = static_cast<juce::AudioProcessorParameter *>(v.cookie);
                    jp->setValue(nf);
                }
                break;
                }
            }
        }

        auto pc = ParamChange();
        auto ov = process->out_events;
        auto evt = clap_event();

        while (uiParamChangeQ.pop(pc))
        {
            evt.type = pc.type;
            evt.time = 0; // for now
            evt.param_value.param_id = pc.id;
            evt.param_value.value = pc.newval;
            ov->push_back(ov, &evt);
        }

        // We process in place so
        static constexpr uint32_t maxBuses = 128;
        std::array<float *, maxBuses> busses{};
        busses.fill(nullptr);

        /*DBG("IO Configuration: I=" << (int)process->audio_inputs_count << " O="
                                   << (int)process->audio_outputs_count << " MX=" << (int)mx);
        DBG("Plugin Configuration: IC=" << processor->getTotalNumInputChannels()
                                        << " OC=" << processor->getTotalNumOutputChannels());
        */

        /*
         * OK so here is what JUCE expects in its audio buffer. It *always* uses input as output
         * buffer so we need to create a buffer where each channel is the channel of the associated
         * output pointer (fine) and then the inputs need to either check they are the same or copy.
         */

        /*
         * So first lets load up with our outputs
         */
        uint32_t ochans = 0;
        for (auto idx = 0; idx < process->audio_outputs_count && ochans < maxBuses; ++idx)
        {
            for (int ch = 0; ch < process->audio_outputs[idx].channel_count; ++ch)
            {
                busses[ochans] = process->audio_outputs[idx].data32[ch];
                ochans++;
            }
        }

        uint32_t ichans = 0;
        for (auto idx = 0; idx < process->audio_inputs_count && ichans < maxBuses; ++idx)
        {
            for (int ch = 0; ch < process->audio_inputs[idx].channel_count; ++ch)
            {
                auto *ic = process->audio_inputs[idx].data32[ch];
                if (ichans < ochans)
                {
                    if (ic == busses[ichans])
                    {
                        // The buffers overlap - no need to do anything
                    }
                    else
                    {
                        juce::FloatVectorOperations::copy(busses[ichans], ic,
                                                          process->frames_count);
                    }
                }
                else
                {
                    busses[ichans] = ic;
                }
                ichans++;
            }
        }

        auto totalChans = std::max(ichans, ochans);
        juce::AudioBuffer<float> buf(busses.data(), totalChans, process->frames_count);

        FIXME("Handle bypass and deactivated states");
        processor->processBlock(buf, mbuf);

        return CLAP_PROCESS_CONTINUE;
    }

    void componentMovedOrResized(juce::Component &component, bool wasMoved,
                                 bool wasResized) override
    {
        if (wasResized)
            _host.guiResize(component.getWidth(), component.getHeight());
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    bool implementsGui() const noexcept override { return processor->hasEditor(); }
    bool guiCanResize() const noexcept override { return true; }

    std::unique_ptr<juce::ScopedJuceInitialiser_GUI> juceGuiInit;
    bool guiCreate() noexcept override
    {
        juceGuiInit = std::make_unique<juce::ScopedJuceInitialiser_GUI>();
        const juce::MessageManagerLock mmLock;
        editor.reset(processor->createEditor());
        editor->addComponentListener(this);
        return editor != nullptr;
    }

    void guiDestroy() noexcept override
    {
        editor.reset(nullptr);
        juceGuiInit.reset(nullptr);
    }
    bool guiSize(uint32_t *width, uint32_t *height) noexcept override
    {
        const juce::MessageManagerLock mmLock;
        if (editor)
        {
            auto b = editor->getBounds();
            *width = b.getWidth();
            *height = b.getHeight();
            return true;
        }
        else
        {
            *width = 1000;
            *height = 800;
        }
        return false;
    }

  protected:
    juce::CriticalSection stateInformationLock;
    juce::MemoryBlock chunkMemory;

  public:
    bool implementsState() const noexcept override { return true; }
    bool stateSave(clap_ostream *stream) noexcept override
    {
        if (processor == nullptr)
            return false;

        juce::ScopedLock lock(stateInformationLock);
        chunkMemory.reset();

        processor->getStateInformation(chunkMemory);

        auto written = stream->write(stream, chunkMemory.getData(), chunkMemory.getSize());
        return written == chunkMemory.getSize();
    }
    bool stateLoad(clap_istream *stream) noexcept override
    {
        if (processor == nullptr)
            return false;

        juce::ScopedLock lock(stateInformationLock);
        chunkMemory.reset();
        // There must be a better way
        char *block[256];
        int64_t rd;
        while ((rd = stream->read(stream, block, 256)) > 0)
            chunkMemory.append(block, rd);

        processor->setStateInformation(chunkMemory.getData(), chunkMemory.getSize());
        chunkMemory.reset();
        return true;
    }

  public:
#if JUCE_MAC
    bool implementsGuiCocoa() const noexcept override { return processor->hasEditor(); };
    bool guiCocoaAttach(void *nsView) noexcept override
    {
        juce::initialiseMacVST();
        auto hostWindow = juce::attachComponentToWindowRefVST(editor.get(), nsView, true);
        return true;
        return false;
    }
#endif

#if JUCE_LINUX
    bool implementsGuiX11() const noexcept override { return processor->hasEditor(); }
    bool guiX11Attach(const char *displayName, unsigned long window) noexcept
    {
        const juce::MessageManagerLock mmLock;
        editor->setVisible(false);
        editor->addToDesktop(0, (void *)window);
        auto *display = juce::XWindowSystem::getInstance()->getDisplay();
        juce::X11Symbols::getInstance()->xReparentWindow(display, (Window)editor->getWindowHandle(),
                                                         window, 0, 0);
        editor->setVisible(true);
        return true;
    }
#endif

#if JUCE_WINDOWS
    bool implementsGuiWin32() const noexcept { return processor->hasEditor(); }
    bool guiWin32Attach(clap_hwnd window) noexcept
    {
        editor->setVisible(false);
        editor->setOpaque(true);
        editor->setTopLeftPosition(0, 0);
        editor->addToDesktop(0, (void *)window);
        editor->setVisible(true);
        return true;
    }
#endif

  private:
    struct ParamChange
    {
        int type;
        uint32_t id;
        float newval;
    };
    PushPopQ<ParamChange, 4096 * 16> uiParamChangeQ;

    /*
     * Various maps for ID lookups
     */
    // clap_id to param *
    std::unordered_map<clap_id, juce::AudioProcessorParameter *> paramPtrByClapID;
    // param * to clap_id
    std::unordered_map<juce::AudioProcessorParameter *, clap_id> clapIDByParamPtr;
    // Every id we have issued
    std::unordered_set<clap_id> allClapIDs;

    juce::LegacyAudioParametersWrapper juceParameters;

    const clap_event_transport *transportInfo{nullptr};
    bool hasTransportInfo{false};
};

clap_plugin_descriptor ClapJuceWrapper::desc = {CLAP_VERSION,
                                                CLAP_ID,
                                                JucePlugin_Name,
                                                JucePlugin_Manufacturer,
                                                JucePlugin_ManufacturerWebsite,
                                                CLAP_MANUAL_URL,
                                                CLAP_SUPPORT_URL,
                                                JucePlugin_VersionString,
                                                JucePlugin_Desc,
                                                "FIXME",
                                                CLAP_PLUGIN_INSTRUMENT};

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter();

namespace ClapAdapter
{
bool clap_init(const char *p) { return true; }

void clap_deinit(void) {}

uint32_t clap_get_plugin_count(const struct clap_plugin_factory *) { return 1; }

const clap_plugin_descriptor *clap_get_plugin_descriptor(const struct clap_plugin_factory *,
                                                         uint32_t w)
{
    return &ClapJuceWrapper::desc;
}

static const clap_plugin *clap_create_plugin(const struct clap_plugin_factory *,
                                             const clap_host *host, const char *plugin_id)
{
    juce::ScopedJuceInitialiser_GUI libraryInitialiser;

    if (strcmp(plugin_id, ClapJuceWrapper::desc.id))
    {
        std::cout << "Warning: CLAP asked for plugin_id '" << plugin_id
                  << "' and JuceCLAPWrapper ID is '" << ClapJuceWrapper::desc.id << "'"
                  << std::endl;
        return nullptr;
    }
    clap_juce_extensions::clap_properties::building_clap = true;
    auto *const pluginInstance = ::createPluginFilter();
    clap_juce_extensions::clap_properties::building_clap = false;
    auto *wrapper = new ClapJuceWrapper(host, pluginInstance);
    return wrapper->clapPlugin();
}

const struct clap_plugin_factory juce_clap_plugin_factory = {
    ClapAdapter::clap_get_plugin_count,
    ClapAdapter::clap_get_plugin_descriptor,
    ClapAdapter::clap_create_plugin,
};

const void *clap_get_factory(const char *factory_id)
{
    if (strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
    {
        return &juce_clap_plugin_factory;
    }

    return nullptr;
}

} // namespace ClapAdapter

extern "C"
{
#if JUCE_LINUX
#pragma GCC diagnostic ignored "-Wattributes"
#endif
    const CLAP_EXPORT struct clap_plugin_entry clap_entry = {CLAP_VERSION, ClapAdapter::clap_init,
                                                             ClapAdapter::clap_deinit,
                                                             ClapAdapter::clap_get_factory};
}
