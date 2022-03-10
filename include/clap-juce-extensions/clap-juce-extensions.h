
#ifndef SURGE_CLAP_JUCE_EXTENSIONS_H
#define SURGE_CLAP_JUCE_EXTENSIONS_H

#include <clap/events.h>

namespace clap_juce_extensions
{
struct clap_properties
{
    static bool building_clap;
    static uint32_t clap_version_major, clap_version_minor, clap_version_revision;

    clap_properties();

    virtual ~clap_properties() = default;
    bool is_clap{false};

    const clap_event_transport *clap_transport{nullptr};
};
} // namespace clap_juce_extensions

#endif // SURGE_CLAP_JUCE_EXTENSIONS_H
