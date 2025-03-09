/*
 * This file contains C++ interface classes which allow your AudioProcessor or
 * AudioProcessorParameter to implement additional clap-specific capabilities, and then allows the
 * CLAP wrapper to detect those capabilities and activate advanced features beyond the base
 * JUCE model.
 */

#ifndef SURGE_CLAP_JUCE_EXTENSIONS_H
#define SURGE_CLAP_JUCE_EXTENSIONS_H

#include <clap/events.h>
#include <clap/plugin.h>
#include <clap/helpers/plugin.hh>

#include <atomic>

/** Forward declaration of the wrapper class. */
class ClapJuceWrapper;
struct JUCEParameterVariant;

namespace ClapAdapter
{
const clap_plugin *clap_create_plugin(const struct clap_plugin_factory *, const clap_host *,
                                      const char *);
}

/** Forward declarations for any JUCE classes we might need. */
namespace juce
{
class MidiBuffer;
class AudioProcessorParameter;
class RangedAudioParameter;
class String;
class Colour;
} // namespace juce

namespace clap_juce_extensions
{
/*
 * clap_properties contains simple properties about clap which you may want to use.
 */
struct clap_properties
{
    clap_properties();
    virtual ~clap_properties() = default;

    // The three part clap version
    static uint32_t clap_version_major, clap_version_minor, clap_version_revision;

    // this will be true for the clap instance and false for all other flavors
    bool is_clap{false};

    // this will be non-null in the process block of a clap where the DAW provides transport
    const clap_event_transport *clap_transport{nullptr};

    // The processing and active clap state
    std::atomic<bool> is_clap_active{false}, is_clap_processing{false};

    // Internal implementation detail. Please disregard (and FIXME)
    static bool building_clap;
};

/*
 * clap_juce_audio_processor_capabilities allows you to interact with advanced properties of the
 * CLAP api. The default implementations here mean if you implement
 * clap_juce_audio_processor_capabilities and override nothing, you get the same behaviour as if you
 * hadn't implemented it.
 */
struct clap_juce_audio_processor_capabilities
{
    virtual ~clap_juce_audio_processor_capabilities() = default;

    /*
     * In some cases, there is no main input, and input 0 is not main. Allow your plugin
     * to advertise that. (This case is usually for synths with sidechains).
     */
    virtual bool isInputMain(int input)
    {
        if (input == 0)
            return true;
        else
            return false;
    }

    /*
     * If you want to provide information about voice structure, as documented
     * in the voice-info clap extension.
     */
    virtual bool supportsVoiceInfo() { return false; }
    virtual bool voiceInfoGet(clap_voice_info * /*info*/) { return false; }

    /*
     * Do you want to receive note expression messages? Note that if you return true
     * here and don't implement supportsDirectProcess, the note expression messages will
     * be received and ignored.
     */
    virtual bool supportsNoteExpressions() { return false; }

    /**
     * The regular CLAP/JUCE wrapper handles the following CLAP events:
     * - MIDI note on/off events
     * - MIDI CC events
     * - Parameter change events
     * - Parameter modulation events
     *
     * If you would like to handle these events using some custom behaviour, or if you would like
     * to handle other CLAP events (e.g. note expression), or events from another namespace, you
     * should override this method to return true for those event types.
     *
     * @param space_id  The namespace ID for the given event.
     * @param type      The event type.
     */
    virtual bool supportsDirectEvent(uint16_t /*space_id*/, uint16_t /*type*/) { return false; }

    /**
     * If your plugin returns true for supportsDirectEvent, then you'll need to
     * implement this method to actually handle that event when it comes along.
     *
     * @param event         The header for the incoming event.
     * @param sampleOffset  If the CLAP wrapper has split up the incoming buffer (e.g. to
     *                      apply sample-accurate automation), then you'll need to apply
     *                      this sample offset to the timestamp of the incoming event
     *                      to get the actual event time relative to the start of the
     *                      next incoming buffer to your processBlock method. For example:
     *                      `const auto actualNoteTime = noteEvent->header.time - sampleOffset;`
     */
    virtual void handleDirectEvent(const clap_event_header_t * /*event*/, int /*sampleOffset*/) {}

    /**
     * If your plugin needs to send outbound events (for example, telling the host that a
     * note has ended), you should override this method to return true.
     */
    virtual bool supportsOutboundEvents() { return false; }

    /**
     * If your plugin returns true for supportsOutboundEvents, then this method will be
     * called after your `processBlock()` method, so that any outbound events can be
     * added to the output event queue.
     *
     * NOTE: if your plugin produces MIDI, you must take care to make sure that any outgoing
     * events which are not MIDI events are correctly interleaved with the outgoing events
     * from the midiBuffer, such that all the events in the output queue are ordered sequentially.
     *
     * @param out_events    The output event queue.
     * @param midiBuffer    The JUCE MIDI Buffer from the previous `processBlock()` call.
     * @param sampleOffset  If the CLAP wrapper has split up the incoming buffer, then
     *                      you'll need to apply this sample offset to the timestamp of
     *                      the outgoing event. For example:
     *                      `auto eventTime = eventTimeRelativeToStartOfLastBlock + sampleOffset;`
     */
    virtual void addOutboundEventsToQueue(const clap_output_events * /*out_events*/,
                                          const juce::MidiBuffer & /* midiBuffer */,
                                          int /*sampleOffset*/)
    {
    }

    /*
     * The JUCE process loop makes it difficult to do things like note expressions,
     * sample accurate parameter automation, and other CLAP features. The custom event handlers
     * (above) help make some of these features possible, but for some use cases, a synth may
     * want the entirety of the JUCE infrastructure *except* the process loop. (Surge is one
     * such synth).
     *
     * In this case, you can implement supportsDirectProcess to return true and then the clap
     * juce wrapper will skip most parts of the process loop (it will still set up transport
     * and deal with UI thread -> audio thread change events), and then call clap_direct_process.
     *
     * In this mode, it is the synth designer responsibility to implement clap_direct_process
     * side by side with AudioProcessor::processBlock to use the CLAP api and synth internals
     * directly.
     *
     * In order to do this, you almost definitely need to both implement clap_direct_process and
     * clap_direct_paramsFlush
     */
    virtual bool supportsDirectProcess() { return false; }
    virtual clap_process_status clap_direct_process(const clap_process * /*process*/) noexcept
    {
        return CLAP_PROCESS_CONTINUE;
    }
    virtual bool supportsDirectParamsFlush() { return false; }
    virtual void clap_direct_paramsFlush(const clap_input_events * /*in*/,
                                         const clap_output_events * /*out*/) noexcept
    {
    }

    /**
     * If you're implementing `clap_direct_process`, you should use this method
     * to handle `CLAP_EVENT_PARAM_VALUE`, so that the parameter listeners are
     * called on the main thread without creating a feedback loop.
     */
    void handleParameterChange(const clap_event_param_value *paramEvent)
    {
        parameterChangeHandler(paramEvent);
    }

    /*
     * Do I support the CLAP_NOTE_DIALECT_CLAP? And prefer it if so? By default this
     * is true if I support either note expressions, direct processing, or voice info,
     * but you can override it for other reasons also, including not liking that default.
     *
     * The strictest hosts will not send note expression without this dialect, and so
     * if you override this to return false, hosts may not give you NE or Voice level
     * modulators in clap_direct_process.
     */
    virtual bool supportsNoteDialectClap(bool /* isInput */)
    {
        return supportsNoteExpressions() || supportsVoiceInfo() || supportsDirectProcess();
    }

    virtual bool prefersNoteDialectClap(bool isInput) { return supportsNoteDialectClap(isInput); }

    // JUCE added support for note names in juce::AudioProcessor
    // in version 8.0.5, but we still allow users to implement
    // CLAP-style note names if they want to.

    /** If your plugin supports custom note names, then override this method to return true. */
    [[nodiscard]] virtual bool supportsNoteName() const noexcept { return false; }

    /**
     * If your plugin supports custom note names, then this method should be overriden
     * to return how the number of note names that your plugin has.
     */
    virtual uint32_t noteNameCount() noexcept { return 0; }

    /**
     * The host will call this method to retrieve the note name for a given index
     * in the range [0, noteNameCount()).
     */
    virtual bool noteNameGet(uint32_t /*index*/, clap_note_name * /*noteName*/) noexcept
    {
        return false;
    }

    /** Plugins should call this method when their note names have changed. */
    void noteNamesChanged()
    {
        if (noteNamesChangedSignal != nullptr)
            noteNamesChangedSignal();
    }

    virtual bool supportsParamIndication() const noexcept { return false; }
    virtual void paramIndicationSetMapping(const juce::RangedAudioParameter & /*param*/,
                                           bool /*has_mapping*/, const juce::Colour * /*colour*/,
                                           const juce::String & /*label*/,
                                           const juce::String & /*description*/) noexcept
    {
    }

    virtual void paramIndicationSetAutomation(const juce::RangedAudioParameter & /*param*/,
                                              uint32_t /*automation_state*/,
                                              const juce::Colour * /*colour*/) noexcept
    {
    }

    /** If your plugin supports remote controls, then override this method to return true. */
    virtual bool supportsRemoteControls() const noexcept { return false; }

    /** If your plugin supports remote controls, then override this method to return the number of
     * pages. */
    virtual uint32_t remoteControlsPageCount() noexcept { return 0; }

    /** If your plugin supports remote controls, then override this method to fill in the page
     * information. If you would like to leave some of the parameter slots blank, then set the
     * parameters for those slots to `nullptr`. */
    virtual bool
    remoteControlsPageFill(uint32_t /*pageIndex*/, juce::String & /*sectionName*/,
                           uint32_t & /*pageID*/, juce::String & /*pageName*/,
                           std::array<juce::AudioProcessorParameter *, CLAP_REMOTE_CONTROLS_COUNT>
                               & /*params*/) noexcept
    {
        return false;
    }

    /** Plugins should call this method when their remote controls have changed. */
    void remoteControlsChanged()
    {
        if (remoteControlsChangedSignal != nullptr)
            remoteControlsChangedSignal();
    }

    /** Plugins should call this method to suggest the host show a remote controls page. */
    void suggestRemoteControlsPage(uint32_t pageID)
    {
        if (suggestRemoteControlsPageSignal != nullptr)
            suggestRemoteControlsPageSignal(pageID);
    }

    /** If your plugin supports preset load, then override this method to return true. */
    virtual bool supportsPresetLoad() const noexcept { return false; }

    /**
     * The plugin should override this method to attempt to load a preset from a location,
     * and return true if the load occurred successfully.
     */
    virtual bool presetLoadFromLocation(uint32_t /*location_kind*/, const char * /*location*/,
                                        const char * /*load_key*/) noexcept
    {
        return false;
    }

    /**
     * The plugin should call this from within presetLoadFromLocation() to report an error when
     * trying to load the preset, and then return false from presetLoadFromLocation().
     */
    void reportPresetLoadError(uint32_t location_kind, const char *location, const char *load_key,
                               int32_t os_error, const juce::String &message)
    {
        if (onPresetLoadError != nullptr)
            onPresetLoadError(location_kind, location, load_key, os_error, message);
    }

    /**
     * The plugin should call this when it has loaded a preset (whether requested from the host,
     * or from internally within the plugin). The provided information will help the host to
     * keep it's preset browser in-sync with the plugin.
     */
    void reportPresetLoaded(uint32_t location_kind, const char *location, const char *load_key)
    {
        if (onPresetLoaded != nullptr)
            onPresetLoaded(location_kind, location, load_key);
    }

    /*
     * If you are working with a host that chooses to not implement cookies you will
     * need to look up parameters by param_id. Use this method to do so.
     */
    JUCEParameterVariant *findParameterByParameterId(clap_id param_id)
    {
        if (lookupParamByID)
            return lookupParamByID(param_id);
        return nullptr;
    }

    const void *getExtension(const char *name)
    {
        if (clapHostStatic != nullptr)
            return clapHostStatic->get_extension(clapHostStatic, name);
        if (extensionGet)
            return extensionGet(name);
        return nullptr;
    }

  private:
    friend class ::ClapJuceWrapper;
    std::function<void(const clap_event_param_value *)> parameterChangeHandler = nullptr;
    std::function<JUCEParameterVariant *(clap_id)> lookupParamByID = nullptr;
    std::function<void()> noteNamesChangedSignal = nullptr;
    std::function<void()> remoteControlsChangedSignal = nullptr;
    std::function<void(uint32_t)> suggestRemoteControlsPageSignal = nullptr;
    std::function<void(uint32_t location_kind, const char *location, const char *load_key,
                       int32_t os_error, const juce::String &msg)>
        onPresetLoadError = nullptr;
    std::function<void(uint32_t location_kind, const char *location, const char *load_key)>
        onPresetLoaded = nullptr;
    std::function<const void *(const char *)> extensionGet = nullptr;

    friend const clap_plugin *ClapAdapter::clap_create_plugin(const struct clap_plugin_factory *,
                                                              const clap_host *, const char *);
    static const clap_host *clapHostStatic;
};

/*
 * clap_juce_parameter_capabilities is intended to be applied to AudioParameter subclasses. When
 * asking your JUCE plugin for parameters, the clap wrapper will check if your parameter
 * implements the capabilities and call the associated functions.
 */
struct clap_juce_parameter_capabilities
{
    virtual ~clap_juce_parameter_capabilities() = default;

    /*
     * Return true if this parameter should receive non-destructive
     * monophonic modulation rather than simple setValue when a DAW
     * initiated modulation changes.
     */
    virtual bool supportsMonophonicModulation() { return false; }

    /** Implement this method to apply the parameter modulation event to your parameter. */
    virtual void applyMonophonicModulation(double /*amount*/) {}

    /*
     * Return true if this parameter should receive non-destructive polyphonic modulation. If this
     * method returns true, then the host will also expect that the paramter can handle monophonic
     * modulation. Additionally, your plugin must return note end events when notes are terminated,
     * by implementing either `addOutboundEventsToQueue()` or `clap_direct_process()`.
     */
    virtual bool supportsPolyphonicModulation() { return false; }

    /** Implement this method to apply the parameter modulation event to your parameter. */
    virtual void applyPolyphonicModulation(int32_t /*note_id*/, int16_t /*port_index*/,
                                           int16_t /*channel*/, int16_t /*key*/, double /*amount*/)
    {
    }
};
} // namespace clap_juce_extensions

/**
 * JUCE parameter that could be ranged, or could extend the clap_juce_parameter_capabilities.
 *
 * When handling CLAP parameter events (e.g. CLAP_EVENT_PARAM_VALUE or CLAP_EVENT_PARAM_MOD),
 * the event `cookie` will be a `JUCEParameterVariant*`.
 */
struct JUCEParameterVariant
{
    /** After the plugin has been initialized, this field should never be a nullptr! */
    juce::AudioProcessorParameter *processorParam = nullptr;

    /** Depending on the underlying parameter type, these could be nullptr. */
    juce::RangedAudioParameter *rangedParameter = nullptr;
    clap_juce_extensions::clap_juce_parameter_capabilities *clapExtParameter = nullptr;
};

#endif // SURGE_CLAP_JUCE_EXTENSIONS_H
