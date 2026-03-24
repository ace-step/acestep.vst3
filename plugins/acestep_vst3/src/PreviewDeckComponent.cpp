#include "PreviewDeckComponent.h"

#include "V2Chrome.h"

namespace acestep::vst3
{
PreviewDeckComponent::PreviewDeckComponent()
{
    summaryLabel_.setColour(juce::Label::textColourId, v2::kLabelPrimary);
    summaryLabel_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(summaryLabel_);
    for (auto* button : {&loadButton_, &playButton_, &stopButton_, &clearButton_, &revealButton_})
    {
        addAndMakeVisible(*button);
    }
}

void PreviewDeckComponent::paint(juce::Graphics& g)
{
    v2::drawModule(g, getLocalBounds(), "Preview Deck", v2::kAccentMint);

    auto waveformZone = getLocalBounds().reduced(22);
    waveformZone.removeFromTop(34);
    waveformZone.removeFromBottom(56);
    g.setColour(v2::kModuleRaised.withAlpha(0.55f));
    g.fillRoundedRectangle(waveformZone.toFloat(), 12.0f);
    g.setColour(v2::kModuleStroke.withAlpha(0.9f));
    g.drawRoundedRectangle(waveformZone.toFloat(), 12.0f, 1.0f);

    auto lane = waveformZone.reduced(16, 18);
    g.setColour(v2::kAccentMint.withAlpha(0.18f));
    for (int index = 0; index < 18; ++index)
    {
        const auto x = lane.getX() + index * (lane.getWidth() / 18);
        const auto height = 10 + ((index % 5) * 8);
        g.fillRoundedRectangle(
            juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(lane.getCentreY() - height / 2), 8.0f,
                                   static_cast<float>(height)),
            3.0f);
    }
}

void PreviewDeckComponent::resized()
{
    auto area = getLocalBounds().reduced(18);
    area.removeFromTop(24);
    summaryLabel_.setBounds(area.removeFromTop(84));
    area.removeFromTop(20);
    auto buttons = area.removeFromTop(36);
    loadButton_.setBounds(buttons.removeFromLeft(148));
    buttons.removeFromLeft(8);
    playButton_.setBounds(buttons.removeFromLeft(92));
    buttons.removeFromLeft(8);
    stopButton_.setBounds(buttons.removeFromLeft(92));
    buttons.removeFromLeft(8);
    clearButton_.setBounds(buttons.removeFromLeft(92));
    buttons.removeFromLeft(8);
    revealButton_.setBounds(buttons.removeFromLeft(136));
}

juce::TextButton& PreviewDeckComponent::loadButton() noexcept { return loadButton_; }
juce::TextButton& PreviewDeckComponent::playButton() noexcept { return playButton_; }
juce::TextButton& PreviewDeckComponent::stopButton() noexcept { return stopButton_; }
juce::TextButton& PreviewDeckComponent::clearButton() noexcept { return clearButton_; }
juce::TextButton& PreviewDeckComponent::revealButton() noexcept { return revealButton_; }

void PreviewDeckComponent::setPreviewSummary(const juce::String& summary)
{
    summaryLabel_.setText(summary, juce::dontSendNotification);
}
}  // namespace acestep::vst3
