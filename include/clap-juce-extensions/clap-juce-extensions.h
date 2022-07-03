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

/** Forward declaration of the wrapper class. */
class ClapJuceWrapper;

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
     * If you want your plugin to handle a specific CLAP event in a custom way,
     * you should override this method to return true for that event.
     *
     * @param space_id  The namespace ID for the given event.
     * @param type      The event type.
     */
    virtual bool supportsDirectEvent(uint16_t /*space_id*/, uint16_t /*type*/) { return false; }

    /**
     * If your plugin returns true for supportsCustomCLAPEvent, then you'll need to
     * implement this method to actually handle that event when it comes along.
     * @param event         The header for the incoming event.
     * @param sampleOffset  If the CLAP wrapper has split up the incoming buffer (e.g. to
     *                      apply sample-accurate automation), then you'll need to apply
     *                      this sample offset to the timestamp of the incoming event
     *                      to get the actual event time relative to the start of the
     *                      next incoming buffer to your processBlock method. For example:
     *                      `const auto actualNoteTime = noteEvent->header.time - sampleOffset;`
     */
    virtual void handleEventDirect(const clap_event_header_t * /*event*/, int /*sampleOffset*/) {}

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
     */
    virtual bool supportsDirectProcess() { return false; }
    virtual clap_process_status clap_direct_process(const clap_process * /*process*/) noexcept
    {
        return CLAP_PROCESS_CONTINUE;
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

  private:
    friend class ::ClapJuceWrapper;
    std::function<void(const clap_event_param_value *)> parameterChangeHandler = nullptr;
};

/*
 * clap_juce_parameter_capabilities is intended to be applied to AudioParameter subclasses. When
 * asking your JUCE plugin for parameters, the clap wrapper will check if your parameter
 * implements the capabilities and call the associated functions.
 */
struct clap_juce_parameter_capabilities
{
    /*
     * Return true if this parameter should receive non-destructive
     * monophonic modulation rather than simple setValue when a DAW
     * initiated modulation changes. Requires you to implement
     * clap_direct_process
     */
    virtual bool supportsMonophonicModulation() { return false; }

    /*
     * Return true if this parameter should receive non-destructive
     * polyphonic modulation. As well as supporting the monophonic case
     * this also requires your process to return note end events when
     * voices are terminated.
     */
    virtual bool supportsPolyphonicModulation() { return false; }
};
} // namespace clap_juce_extensions

#endif // SURGE_CLAP_JUCE_EXTENSIONS_H
