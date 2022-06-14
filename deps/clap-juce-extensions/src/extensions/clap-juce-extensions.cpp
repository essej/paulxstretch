//
// Created by Paul Walker on 12/19/21.
//

#include "clap-juce-extensions/clap-juce-extensions.h"

namespace clap_juce_extensions
{
bool clap_properties::building_clap{false};
uint32_t clap_properties::clap_version_major{0}, clap_properties::clap_version_minor{0},
    clap_properties::clap_version_revision{0};

clap_properties::clap_properties() : is_clap{building_clap} {}

} // namespace clap_juce_extensions