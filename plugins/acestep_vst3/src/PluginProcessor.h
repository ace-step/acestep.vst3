#pragma once

#include <atomic>
#include <optional>

#include <JuceHeader.h>

#include "PluginBackendClient.h"
#include "PluginConfig.h"
#include "PluginPreview.h"
#include "PluginState.h"

namespace acestep::vst3
{
class ACEStepVST3AudioProcessor final : public juce::AudioProcessor
{
public:
    ACEStepVST3AudioProcessor();
    ~ACEStepVST3AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    bool isSynth() const;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    const PluginState& getState() const noexcept;
    PluginState& getMutableState() noexcept;
    [[nodiscard]] bool loadPreviewFile(const juce::File& file);
    void clearPreviewFile();
    void playPreview();
    void stopPreview();
    void revealPreviewFile() const;
    [[nodiscard]] bool hasPreviewFile() const;
    [[nodiscard]] bool isPreviewPlaying() const;
    [[nodiscard]] bool exportSessionSummary(const juce::File& file);
    void requestGeneration();
    void requestLoadLoRA();
    void requestUnloadLoRA();
    void requestToggleLoRA(bool useLora);
    void requestLoRAScale(double scale);
    void selectResultSlot(int index);
    void selectCompareSlots(int primaryIndex, int secondaryIndex);
    void cueCompareSlot(bool primarySlot);
    void toggleCompareSlot();
    void pumpBackendWorkflow();

private:
    enum class BackendTaskKind
    {
        none,
        healthCheck,
        submitGeneration,
        pollGeneration,
        downloadPreview,
        loraStatus,
        loraLoad,
        loraUnload,
        loraToggle,
        loraScale,
    };

    struct BackendTaskResult final
    {
        BackendTaskKind kind = BackendTaskKind::none;
        PluginHealthCheckResult health;
        PluginGenerationStartResult generationStart;
        PluginGenerationPollResult generationPoll;
        PluginPreviewDownloadResult previewDownload;
        PluginLoRAStatusResult loraStatus;
        PluginLoRAOperationResult loraOperation;
    };

    void scheduleHealthCheck();
    void scheduleGenerationStart();
    void scheduleGenerationPoll();
    void schedulePreviewDownload(int slotIndex);
    void scheduleLoRAStatusCheck();
    void scheduleLoadLoRA();
    void scheduleUnloadLoRA();
    void scheduleToggleLoRA(bool useLora);
    void scheduleLoRAScale(double scale);
    void applyLoRAStatus(const PluginLoRAStatusResult& result);
    void applyCompletedTask(const BackendTaskResult& taskResult);
    void clearGeneratedResults();
    void syncPreviewFromState();

    PluginState state_;
    PluginPreview preview_;
    PluginBackendClient backendClient_;
    juce::ThreadPool backendThreadPool_ {1};
    juce::CriticalSection backendTaskLock_;
    std::optional<BackendTaskResult> completedBackendTask_;
    std::atomic<bool> backendTaskRunning_ {false};
    juce::uint32 lastHealthCheckAtMs_ = 0;
    juce::uint32 lastLoRAStatusCheckAtMs_ = 0;
    juce::uint32 lastPollRequestAtMs_ = 0;
    juce::String lastHealthCheckedBaseUrl_;
    std::optional<int> pendingPreviewDownloadSlot_;
    bool pendingLoadLoRA_ = false;
    bool pendingUnloadLoRA_ = false;
    bool pendingLoRAStatusRefresh_ = true;
    std::optional<bool> pendingLoRAToggle_;
    std::optional<double> pendingLoRAScale_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ACEStepVST3AudioProcessor)
};
}  // namespace acestep::vst3
