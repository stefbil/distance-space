#include "CueActivityComponent.h"
// juce::Colour is not a literal type -- cannot be constexpr.
// Defined with runtime static initialisation.
const std::array<CueActivityComponent::CueDef,5>
CueActivityComponent::CUES{{
    {"INTENSITY",  juce::Colour(0xFFFFAA28)},
    {"HF ROLLOFF", juce::Colour(0xFFFFCC60)},
    {"DRR WET",    juce::Colour(0xFF60B0FF)},
    {"PRE-DELAY",  juce::Colour(0xFF80D0A0)},
    {"HRTF BLEND", juce::Colour(0xFFC080FF)},
}};
