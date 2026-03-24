#include "V2Chrome.h"

namespace acestep::vst3::v2
{
void drawChassis(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto area = bounds.toFloat();
    g.setGradientFill(
        juce::ColourGradient(kChassisOuter, area.getTopLeft(), kChassisInner, area.getBottomLeft(), false));
    g.fillRoundedRectangle(area, 24.0f);

    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.drawRoundedRectangle(area.reduced(1.0f), 24.0f, 3.0f);

    g.setColour(kModuleStroke.withAlpha(0.55f));
    g.drawRoundedRectangle(area.reduced(8.0f), 18.0f, 1.4f);

    g.setColour(kLabelMuted.withAlpha(0.07f));
    for (int x = bounds.getX() + 20; x < bounds.getRight() - 20; x += 12)
    {
        g.drawVerticalLine(x, static_cast<float>(bounds.getY() + 18), static_cast<float>(bounds.getBottom() - 18));
    }
}

void drawModule(juce::Graphics& g,
                juce::Rectangle<int> bounds,
                const juce::String& title,
                juce::Colour accent)
{
    auto area = bounds.toFloat();
    g.setColour(accent.withAlpha(0.12f));
    g.fillRoundedRectangle(area.expanded(1.0f), 18.0f);

    g.setGradientFill(
        juce::ColourGradient(kModuleRaised, area.getTopLeft(), kModuleFill, area.getBottomLeft(), false));
    g.fillRoundedRectangle(area, 18.0f);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawRoundedRectangle(area.translated(0.0f, 1.0f), 18.0f, 2.0f);
    g.setColour(kModuleStroke);
    g.drawRoundedRectangle(area, 18.0f, 1.1f);

    auto titleBar = area.removeFromTop(30.0f).reduced(14.0f, 6.0f);
    g.setColour(accent.withAlpha(0.22f));
    g.fillRoundedRectangle(titleBar, 8.0f);
    g.setColour(kLabelPrimary);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText(title.toUpperCase(), titleBar.reduced(12.0f, 0.0f), juce::Justification::centredLeft);
}

void drawDisplay(juce::Graphics& g, juce::Rectangle<int> bounds, bool active)
{
    auto area = bounds.toFloat();
    auto fill = active ? kDisplayFill.brighter(0.08f) : kDisplayFill;
    g.setGradientFill(juce::ColourGradient(fill.brighter(0.08f), area.getTopLeft(), fill, area.getBottomLeft(), false));
    g.fillRoundedRectangle(area, 14.0f);
    g.setColour(kDisplayGlow.withAlpha(active ? 0.42f : 0.18f));
    g.drawRoundedRectangle(area, 14.0f, 1.2f);
}

void drawLamp(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour colour, bool active)
{
    const auto base = active ? colour : kModuleStroke.darker(0.6f);
    g.setColour(base.darker(0.7f));
    g.fillEllipse(bounds);
    g.setColour(base.brighter(active ? 0.2f : 0.05f));
    g.fillEllipse(bounds.reduced(2.0f));
    if (active)
    {
        g.setColour(base.withAlpha(0.22f));
        g.fillEllipse(bounds.expanded(6.0f));
    }
}

void drawTapeReel(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour accent, bool active)
{
    g.setColour(kModuleStroke.withAlpha(0.5f));
    g.fillEllipse(bounds);
    g.setColour(accent.withAlpha(active ? 0.28f : 0.12f));
    g.fillEllipse(bounds.reduced(6.0f));
    g.setColour(kLabelPrimary.withAlpha(0.75f));
    g.drawEllipse(bounds.reduced(8.0f), 1.2f);

    const auto centre = bounds.getCentre();
    const auto radius = bounds.getWidth() * 0.18f;
    for (int index = 0; index < 3; ++index)
    {
        juce::Path spoke;
        const auto angle = juce::MathConstants<float>::twoPi * static_cast<float>(index) / 3.0f
                           + (active ? 0.35f : 0.0f);
        spoke.startNewSubPath(centre);
        spoke.lineTo(centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius);
        g.strokePath(spoke, juce::PathStrokeType(2.0f));
    }

    g.setColour(kChassisOuter);
    g.fillEllipse(juce::Rectangle<float>(16.0f, 16.0f).withCentre(centre));
}

juce::Colour statusColour(BackendStatus status)
{
    switch (status)
    {
        case BackendStatus::ready:
            return kAccentMint;
        case BackendStatus::offline:
            return kAccentRed;
        case BackendStatus::degraded:
            return kAccentAmber;
    }

    return kAccentMint;
}

juce::Colour statusColour(JobStatus status)
{
    switch (status)
    {
        case JobStatus::idle:
            return kAccentBlue;
        case JobStatus::submitting:
        case JobStatus::queuedOrRunning:
            return kAccentAmber;
        case JobStatus::succeeded:
            return kAccentMint;
        case JobStatus::failed:
            return kAccentRed;
    }

    return kAccentBlue;
}
}  // namespace acestep::vst3::v2
