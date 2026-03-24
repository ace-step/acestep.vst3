#pragma once

#include <JuceHeader.h>

#include "PluginEnums.h"

namespace acestep::vst3::v2
{
inline const auto kChassisOuter = juce::Colour::fromRGB(10, 15, 20);
inline const auto kChassisInner = juce::Colour::fromRGB(18, 24, 31);
inline const auto kModuleFill = juce::Colour::fromRGB(17, 23, 30);
inline const auto kModuleRaised = juce::Colour::fromRGB(29, 38, 49);
inline const auto kModuleStroke = juce::Colour::fromRGB(63, 82, 100);
inline const auto kDisplayFill = juce::Colour::fromRGB(13, 28, 31);
inline const auto kDisplayGlow = juce::Colour::fromRGB(96, 224, 205);
inline const auto kLabelPrimary = juce::Colour::fromRGB(229, 235, 240);
inline const auto kLabelMuted = juce::Colour::fromRGB(134, 152, 167);
inline const auto kAccentMint = juce::Colour::fromRGB(83, 226, 203);
inline const auto kAccentAmber = juce::Colour::fromRGB(255, 177, 92);
inline const auto kAccentRed = juce::Colour::fromRGB(255, 108, 112);
inline const auto kAccentBlue = juce::Colour::fromRGB(108, 154, 255);

void drawChassis(juce::Graphics& g, juce::Rectangle<int> bounds);
void drawModule(juce::Graphics& g,
                juce::Rectangle<int> bounds,
                const juce::String& title,
                juce::Colour accent);
void drawDisplay(juce::Graphics& g, juce::Rectangle<int> bounds, bool active);
void drawLamp(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, bool active);
void drawTapeReel(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent, bool active);
juce::Colour statusColour(BackendStatus status);
juce::Colour statusColour(JobStatus status);
}  // namespace acestep::vst3::v2
