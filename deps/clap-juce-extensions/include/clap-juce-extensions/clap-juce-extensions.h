/*
 * This file contains interface extensions which allow your AudioProcessor or
 * AudioProcessorParameter to implement additional clap-specific API points, and then allows the
 * CLAP wrapper to detect those implementation points and activate advanced features beyond the base
 * JUCE model.
 */

#ifndef SURGE_CLAP_JUCE_EXTENSIONS_H
#define SURGE_CLAP_JUCE_EXTENSIONS_H

#include <clap/events.h>
#include <clap/plugin.h>
#include <clap/helpers/plugin.hh>

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
 * clap_extensions allows you to interact with advanced properties of the CLAP api.
 * The default implementations here mean if you implement clap_extensions and override
 * nothing, you get the same behaviour as if you hadn't implemented it.
 */
struct clap_extensions
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

    /*
     * The JUCE process loop makes it difficult to do things like note expressions,
     * sample accurate parameter automation, and other CLAP features. In most cases that
     * is fine, but for some use cases, a synth may want the entirety of the JUCE infrastructure
     * *except* the process loop. (Surge is one such synth).
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
};

/*
 * clap_param_extensions is intended to be applied to AudioParameter subclasses. When
 * asking your JUCE plugin for parameters, the clap wrapper will check if your parameter
 * implements the extensions and call the associated functions.
 */
struct clap_param_extensions
{
    /*
     * Return true if this parameter should receive non-destructive
     * monophonic modulation rather than simple setValue when a DAW
     * initiated modulation changes. Requires you to implement
     * clap_direct_process
     */
    virtual bool supportsMonophonicModulation() { return false; };

    /*
     * Return true if this parameter should receive non-destructive
     * polyphonic modulation. As well as supporting the monophonic case
     * this also requires your process to return note end events when
     * voices are terminated.
     */
    virtual bool supportsPolyphonicModulation() { return false; };
};
} // namespace clap_juce_extensions

#endif // SURGE_CLAP_JUCE_EXTENSIONS_H
