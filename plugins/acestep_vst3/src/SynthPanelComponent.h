#pragma once

#include <JuceHeader.h>

namespace acestep::vst3
{
class SynthPanelComponent final : public juce::Component
{
public:
    SynthPanelComponent();

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::TextEditor& backendUrlEditor() noexcept;
    juce::TextEditor& promptEditor() noexcept;
    juce::TextEditor& lyricsEditor() noexcept;
    juce::TextEditor& referenceAudioEditor() noexcept;
    juce::TextEditor& sourceAudioEditor() noexcept;
    juce::TextEditor& conditioningCodesEditor() noexcept;
    juce::TextEditor& loraPathEditor() noexcept;
    juce::TextEditor& seedEditor() noexcept;
    juce::ComboBox& modeBox() noexcept;
    juce::ComboBox& durationBox() noexcept;
    juce::ComboBox& modelBox() noexcept;
    juce::ComboBox& qualityBox() noexcept;
    juce::ComboBox& loraAdapterBox() noexcept;
    juce::Slider& coverStrengthSlider() noexcept;
    juce::Slider& loraScaleSlider() noexcept;
    juce::TextButton& chooseReferenceButton() noexcept;
    juce::TextButton& clearReferenceButton() noexcept;
    juce::TextButton& chooseSourceButton() noexcept;
    juce::TextButton& clearSourceButton() noexcept;
    juce::TextButton& loadLoRAButton() noexcept;
    juce::TextButton& unloadLoRAButton() noexcept;
    juce::ToggleButton& useLoRAToggle() noexcept;
    juce::Label& loraStatusLabel() noexcept;

private:
    juce::Label backendUrlLabel_;
    juce::Label promptLabel_;
    juce::Label lyricsLabel_;
    juce::Label modeLabel_;
    juce::Label referenceAudioLabel_;
    juce::Label sourceAudioLabel_;
    juce::Label conditioningCodesLabel_;
    juce::Label coverStrengthLabel_;
    juce::Label loraPathLabel_;
    juce::Label loraAdapterLabel_;
    juce::Label loraScaleLabel_;
    juce::Label loraStatusLabel_;
    juce::Label durationLabel_;
    juce::Label seedLabel_;
    juce::Label modelLabel_;
    juce::Label qualityLabel_;
    juce::TextEditor backendUrlEditor_;
    juce::TextEditor promptEditor_;
    juce::TextEditor lyricsEditor_;
    juce::TextEditor referenceAudioEditor_;
    juce::TextEditor sourceAudioEditor_;
    juce::TextEditor conditioningCodesEditor_;
    juce::TextEditor loraPathEditor_;
    juce::TextEditor seedEditor_;
    juce::ComboBox modeBox_;
    juce::ComboBox durationBox_;
    juce::ComboBox modelBox_;
    juce::ComboBox qualityBox_;
    juce::ComboBox loraAdapterBox_;
    juce::Slider coverStrengthSlider_;
    juce::Slider loraScaleSlider_;
    juce::TextButton chooseReferenceButton_ {"Choose"};
    juce::TextButton clearReferenceButton_ {"Clear"};
    juce::TextButton chooseSourceButton_ {"Choose"};
    juce::TextButton clearSourceButton_ {"Clear"};
    juce::TextButton loadLoRAButton_ {"Load"};
    juce::TextButton unloadLoRAButton_ {"Unload"};
    juce::ToggleButton useLoRAToggle_ {"Use LoRA"};
};
}  // namespace acestep::vst3
