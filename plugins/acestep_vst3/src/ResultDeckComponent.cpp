#include "ResultDeckComponent.h"

#include "V2Chrome.h"

namespace acestep::vst3
{
ResultDeckComponent::ResultDeckComponent()
{
    resultLabel_.setText("Take Select", juce::dontSendNotification);
    resultLabel_.setColour(juce::Label::textColourId, v2::kLabelMuted);
    comparePrimaryLabel_.setText("Compare A", juce::dontSendNotification);
    compareSecondaryLabel_.setText("Compare B", juce::dontSendNotification);
    comparePrimaryLabel_.setColour(juce::Label::textColourId, v2::kLabelMuted);
    compareSecondaryLabel_.setColour(juce::Label::textColourId, v2::kLabelMuted);
    summaryLabel_.setColour(juce::Label::textColourId, v2::kLabelPrimary);
    summaryLabel_.setJustificationType(juce::Justification::centredLeft);
    compareSummaryLabel_.setColour(juce::Label::textColourId, v2::kAccentMint);
    compareSummaryLabel_.setJustificationType(juce::Justification::centredLeft);
    dragToDawButton_.setButtonText("Drag WAV to Track");
    addAndMakeVisible(resultLabel_);
    addAndMakeVisible(resultSelector_);
    addAndMakeVisible(summaryLabel_);
    addAndMakeVisible(comparePrimaryLabel_);
    addAndMakeVisible(compareSecondaryLabel_);
    addAndMakeVisible(comparePrimarySelector_);
    addAndMakeVisible(compareSecondarySelector_);
    addAndMakeVisible(cueCompareAButton_);
    addAndMakeVisible(cueCompareBButton_);
    addAndMakeVisible(toggleCompareButton_);
    addAndMakeVisible(dragToDawButton_);
    addAndMakeVisible(compareSummaryLabel_);
}

void ResultDeckComponent::paint(juce::Graphics& g)
{
    v2::drawModule(g, getLocalBounds(), "Result Deck", v2::kAccentBlue);
}

void ResultDeckComponent::resized()
{
    auto area = getLocalBounds().reduced(18);
    area.removeFromTop(24);
    resultLabel_.setBounds(area.removeFromTop(18));
    resultSelector_.setBounds(area.removeFromTop(32));
    area.removeFromTop(10);
    summaryLabel_.setBounds(area.removeFromTop(46));
    area.removeFromTop(8);

    auto handoffRow = area.removeFromTop(34);
    dragToDawButton_.setBounds(handoffRow.removeFromLeft(184));
    handoffRow.removeFromLeft(12);
    compareSummaryLabel_.setBounds(handoffRow);

    area.removeFromTop(10);

    auto comparePrimaryRow = area.removeFromTop(26);
    comparePrimaryLabel_.setBounds(comparePrimaryRow.removeFromLeft(84));
    comparePrimarySelector_.setBounds(comparePrimaryRow.removeFromLeft(240));
    comparePrimaryRow.removeFromLeft(10);
    cueCompareAButton_.setBounds(comparePrimaryRow.removeFromLeft(84));

    area.removeFromTop(8);
    auto compareSecondaryRow = area.removeFromTop(26);
    compareSecondaryLabel_.setBounds(compareSecondaryRow.removeFromLeft(84));
    compareSecondarySelector_.setBounds(compareSecondaryRow.removeFromLeft(240));
    compareSecondaryRow.removeFromLeft(10);
    cueCompareBButton_.setBounds(compareSecondaryRow.removeFromLeft(84));

    area.removeFromTop(10);
    toggleCompareButton_.setBounds(area.removeFromTop(28).removeFromLeft(128));
}

juce::ComboBox& ResultDeckComponent::resultSelector() noexcept { return resultSelector_; }
juce::ComboBox& ResultDeckComponent::comparePrimarySelector() noexcept
{
    return comparePrimarySelector_;
}
juce::ComboBox& ResultDeckComponent::compareSecondarySelector() noexcept
{
    return compareSecondarySelector_;
}
juce::TextButton& ResultDeckComponent::cueCompareAButton() noexcept { return cueCompareAButton_; }
juce::TextButton& ResultDeckComponent::cueCompareBButton() noexcept { return cueCompareBButton_; }
juce::TextButton& ResultDeckComponent::toggleCompareButton() noexcept
{
    return toggleCompareButton_;
}
juce::TextButton& ResultDeckComponent::dragToDawButton() noexcept { return dragToDawButton_; }

void ResultDeckComponent::setTakeSummary(const juce::String& title, const juce::String& detail)
{
    summaryLabel_.setText(title + "\n" + detail, juce::dontSendNotification);
}

void ResultDeckComponent::setCompareSummary(const juce::String& summary)
{
    compareSummaryLabel_.setText(summary, juce::dontSendNotification);
}
}  // namespace acestep::vst3
