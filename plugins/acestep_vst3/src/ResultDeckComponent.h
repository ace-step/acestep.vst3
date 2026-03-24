#pragma once

#include <JuceHeader.h>

namespace acestep::vst3
{
class ResultDeckComponent final : public juce::Component
{
public:
    ResultDeckComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::ComboBox& resultSelector() noexcept;
    juce::ComboBox& comparePrimarySelector() noexcept;
    juce::ComboBox& compareSecondarySelector() noexcept;
    juce::TextButton& cueCompareAButton() noexcept;
    juce::TextButton& cueCompareBButton() noexcept;
    juce::TextButton& toggleCompareButton() noexcept;
    juce::TextButton& dragToDawButton() noexcept;
    void setTakeSummary(const juce::String& title, const juce::String& detail);
    void setCompareSummary(const juce::String& summary);

private:
    juce::Label resultLabel_;
    juce::ComboBox resultSelector_;
    juce::Label summaryLabel_;
    juce::Label comparePrimaryLabel_;
    juce::Label compareSecondaryLabel_;
    juce::ComboBox comparePrimarySelector_;
    juce::ComboBox compareSecondarySelector_;
    juce::TextButton cueCompareAButton_ {"Cue A"};
    juce::TextButton cueCompareBButton_ {"Cue B"};
    juce::TextButton toggleCompareButton_ {"Toggle A/B"};
    juce::TextButton dragToDawButton_ {"Drag to DAW"};
    juce::Label compareSummaryLabel_;
};
}  // namespace acestep::vst3
