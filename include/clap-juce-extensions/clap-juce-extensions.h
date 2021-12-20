
#ifndef SURGE_CLAP_JUCE_EXTENSIONS_H
#define SURGE_CLAP_JUCE_EXTENSIONS_H

namespace clap_juce_extensions
{
struct clap_properties
{
    static bool building_clap;

    clap_properties();

    virtual ~clap_properties() = default;
    bool is_clap{false};
};
} // namespace clap_juce_extensions

#endif // SURGE_CLAP_JUCE_EXTENSIONS_H
