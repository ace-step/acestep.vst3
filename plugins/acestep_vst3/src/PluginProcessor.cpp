#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace acestep::vst3
{
namespace
{
juce::String deriveSessionName(const PluginState& state)
{
    const auto prompt = state.prompt.trim();
    if (prompt.isEmpty())
    {
        return "Untitled Session";
    }

    return prompt.substring(0, 48);
}

void mirrorTakeIntoLegacyFields(PluginState& state, int slotIndex)
{
    const auto index = static_cast<size_t>(slotIndex);
    const auto& take = state.resultTakes[index];
    state.resultSlots[index] = take.slotLabel;
    state.resultFileUrls[index] = take.remoteFileUrl;
    state.resultLocalPaths[index] = take.localFilePath;
}

void refreshSessionFlags(PluginState& state)
{
    const auto busy = state.transportState == TransportState::submitting
                      || state.transportState == TransportState::queued
                      || state.transportState == TransportState::rendering;
    state.session.canCancelActiveTask = busy;
    if (state.session.sessionName.isEmpty())
    {
        state.session.sessionName = deriveSessionName(state);
    }
}

bool hasValidAudioFilePath(const juce::String& path)
{
    return path.isNotEmpty() && juce::File(path).existsAsFile();
}

juce::String printableField(const juce::String& value, const juce::String& fallback = "-")
{
    const auto trimmed = value.trim();
    return trimmed.isEmpty() ? fallback : trimmed;
}
}  // namespace

ACEStepVST3AudioProcessor::ACEStepVST3AudioProcessor()
    : juce::AudioProcessor(
          BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

ACEStepVST3AudioProcessor::~ACEStepVST3AudioProcessor() = default;

void ACEStepVST3AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    preview_.prepareToPlay(sampleRate, samplesPerBlock);
}

void ACEStepVST3AudioProcessor::releaseResources()
{
    preview_.releaseResources();
}

bool ACEStepVST3AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    {
        return false;
    }

    return layouts.getMainInputChannelSet().isDisabled();
}

void ACEStepVST3AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    buffer.clear();
    preview_.render(buffer);
}

juce::AudioProcessorEditor* ACEStepVST3AudioProcessor::createEditor()
{
    return new ACEStepVST3AudioProcessorEditor(*this);
}

bool ACEStepVST3AudioProcessor::hasEditor() const
{
    return true;
}

const juce::String ACEStepVST3AudioProcessor::getName() const
{
    return kPluginName;
}

bool ACEStepVST3AudioProcessor::acceptsMidi() const
{
    return true;
}

bool ACEStepVST3AudioProcessor::producesMidi() const
{
    return false;
}

bool ACEStepVST3AudioProcessor::isMidiEffect() const
{
    return false;
}

bool ACEStepVST3AudioProcessor::isSynth() const
{
    return true;
}

double ACEStepVST3AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ACEStepVST3AudioProcessor::getNumPrograms()
{
    return 1;
}

int ACEStepVST3AudioProcessor::getCurrentProgram()
{
    return 0;
}

void ACEStepVST3AudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String ACEStepVST3AudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void ACEStepVST3AudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void ACEStepVST3AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = createStateXml(state_))
    {
        copyXmlToBinary(*xml, destData);
    }
}

void ACEStepVST3AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr)
    {
        if (auto parsedState = parseStateXml(*xml))
        {
            state_ = *parsedState;
            syncPreviewFromState();
        }
    }
}

const PluginState& ACEStepVST3AudioProcessor::getState() const noexcept
{
    return state_;
}

PluginState& ACEStepVST3AudioProcessor::getMutableState() noexcept
{
    return state_;
}

bool ACEStepVST3AudioProcessor::loadPreviewFile(const juce::File& file)
{
    juce::String errorMessage;
    if (!preview_.loadFile(file, errorMessage))
    {
        state_.errorMessage = errorMessage;
        return false;
    }

    state_.previewFilePath = file.getFullPathName();
    state_.previewDisplayName = file.getFileName();
    state_.errorMessage = {};
    state_.session.lastCompletedSlot = state_.selectedResultSlot;
    return true;
}

void ACEStepVST3AudioProcessor::clearPreviewFile()
{
    preview_.clear();
    state_.previewFilePath = {};
    state_.previewDisplayName = {};
    state_.errorMessage = {};
}

void ACEStepVST3AudioProcessor::playPreview()
{
    if (!hasPreviewFile())
    {
        state_.errorMessage = "Load a preview file before playing it.";
        return;
    }

    state_.errorMessage = {};
    preview_.play();
}

void ACEStepVST3AudioProcessor::stopPreview()
{
    preview_.stop();
}

void ACEStepVST3AudioProcessor::revealPreviewFile() const
{
    preview_.revealToUser();
}

bool ACEStepVST3AudioProcessor::hasPreviewFile() const
{
    return preview_.hasLoadedFile();
}

bool ACEStepVST3AudioProcessor::isPreviewPlaying() const
{
    return preview_.isPlaying();
}

bool ACEStepVST3AudioProcessor::exportSessionSummary(const juce::File& file)
{
    if (file == juce::File())
    {
        state_.errorMessage = "Choose an export destination first.";
        return false;
    }

    juce::String summary;
    summary << "ACE-Step VST3 Session Export\n";
    summary << "===========================\n\n";
    summary << "Project: " << printableField(state_.session.projectName, "Untitled Project") << "\n";
    summary << "Session: " << printableField(state_.session.sessionName, "Untitled Session") << "\n";
    summary << "Mode: " << printableField(toString(state_.workflowMode)) << "\n";
    summary << "Prompt: " << printableField(state_.prompt) << "\n";
    summary << "Lyrics: " << printableField(state_.lyrics) << "\n";
    summary << "Sections: " << printableField(state_.sectionPlan) << "\n";
    summary << "Chords: " << printableField(state_.chordProgression) << "\n";
    summary << "Export Notes: " << printableField(state_.exportNotes) << "\n";
    summary << "Duration: " << state_.durationSeconds << " seconds\n";
    summary << "Seed: " << state_.seed << "\n";
    summary << "Model: " << printableField(toString(state_.modelPreset)) << "\n";
    summary << "Quality: " << printableField(toString(state_.qualityMode)) << "\n";
    summary << "Selected Result Slot: " << (state_.selectedResultSlot + 1) << "\n";
    summary << "Preview File: " << printableField(state_.previewFilePath) << "\n";

    for (int index = 0; index < kResultSlotCount; ++index)
    {
        const auto slotIndex = static_cast<size_t>(index);
        const auto& take = state_.resultTakes[slotIndex];
        summary << "\nResult " << (index + 1) << ":\n";
        summary << "  Label: "
                << printableField(take.slotLabel.isNotEmpty() ? take.slotLabel
                                                              : state_.resultSlots[slotIndex],
                                  "Empty")
                << "\n";
        summary << "  Status: " << printableField(take.statusText, "Pending") << "\n";
        summary << "  Remote URL: "
                << printableField(take.remoteFileUrl.isNotEmpty() ? take.remoteFileUrl
                                                                  : state_.resultFileUrls[slotIndex])
                << "\n";
        summary << "  Local Path: "
                << printableField(take.localFilePath.isNotEmpty() ? take.localFilePath
                                                                  : state_.resultLocalPaths[slotIndex])
                << "\n";
        summary << "  Seed: " << take.seed << "\n";
        summary << "  Duration: " << take.durationSeconds << " seconds\n";
        summary << "  Model: " << printableField(toString(take.modelPreset)) << "\n";
        summary << "  Quality: " << printableField(toString(take.qualityMode)) << "\n";
        summary << "  Compare Group: " << printableField(take.compareGroup) << "\n";
    }

    if (file.existsAsFile())
    {
        file.deleteFile();
    }

    juce::FileOutputStream output(file);
    if (!output.openedOk())
    {
        state_.errorMessage = "Could not open the export destination.";
        return false;
    }

    output.writeText(summary, false, false, "\n");
    output.flush();
    state_.lastExportPath = file.getFullPathName();
    state_.errorMessage = {};
    return true;
}

void ACEStepVST3AudioProcessor::requestLoadLoRA()
{
    if (state_.jobStatus == JobStatus::submitting || state_.jobStatus == JobStatus::queuedOrRunning)
    {
        state_.errorMessage = "Wait for the active render to finish before changing LoRA.";
        return;
    }

    if (state_.loraPath.trim().isEmpty())
    {
        state_.errorMessage = "LoRA path is required before loading.";
        return;
    }

    state_.loraStatusText = "Loading LoRA...";
    state_.errorMessage = {};
    pendingLoadLoRA_ = true;
}

void ACEStepVST3AudioProcessor::requestUnloadLoRA()
{
    if (state_.jobStatus == JobStatus::submitting || state_.jobStatus == JobStatus::queuedOrRunning)
    {
        state_.errorMessage = "Wait for the active render to finish before changing LoRA.";
        return;
    }

    state_.loraStatusText = "Unloading LoRA...";
    state_.errorMessage = {};
    pendingUnloadLoRA_ = true;
}

void ACEStepVST3AudioProcessor::requestToggleLoRA(bool useLora)
{
    if (state_.jobStatus == JobStatus::submitting || state_.jobStatus == JobStatus::queuedOrRunning)
    {
        state_.errorMessage = "Wait for the active render to finish before changing LoRA.";
        return;
    }

    state_.loraStatusText = useLora ? "Enabling LoRA..." : "Disabling LoRA...";
    state_.errorMessage = {};
    pendingLoRAToggle_ = useLora;
}

void ACEStepVST3AudioProcessor::requestLoRAScale(double scale)
{
    if (state_.jobStatus == JobStatus::submitting || state_.jobStatus == JobStatus::queuedOrRunning)
    {
        state_.errorMessage = "Wait for the active render to finish before changing LoRA.";
        return;
    }

    state_.loraStatusText = "Updating LoRA scale...";
    state_.errorMessage = {};
    pendingLoRAScale_ = scale;
}

void ACEStepVST3AudioProcessor::requestGeneration()
{
    if (state_.workflowMode == WorkflowMode::text && state_.prompt.trim().isEmpty())
    {
        state_.jobStatus = JobStatus::failed;
        state_.transportState = TransportState::failed;
        state_.progressText = {};
        state_.errorMessage = "Prompt is required before generation.";
        refreshSessionFlags(state_);
        return;
    }

    if (state_.workflowMode == WorkflowMode::reference
        && !hasValidAudioFilePath(state_.referenceAudioPath))
    {
        state_.jobStatus = JobStatus::failed;
        state_.transportState = TransportState::failed;
        state_.progressText = {};
        state_.errorMessage = "Reference mode requires a valid reference audio file.";
        refreshSessionFlags(state_);
        return;
    }

    if (state_.workflowMode == WorkflowMode::coverRemix
        && !hasValidAudioFilePath(state_.sourceAudioPath))
    {
        state_.jobStatus = JobStatus::failed;
        state_.transportState = TransportState::failed;
        state_.progressText = {};
        state_.errorMessage = "Cover / Remix mode requires a valid source audio file.";
        refreshSessionFlags(state_);
        return;
    }

    if (state_.workflowMode == WorkflowMode::coverRemix
        && state_.referenceAudioPath.isNotEmpty()
        && !hasValidAudioFilePath(state_.referenceAudioPath))
    {
        state_.jobStatus = JobStatus::failed;
        state_.transportState = TransportState::failed;
        state_.progressText = {};
        state_.errorMessage = "Reference audio path is set but the file does not exist.";
        refreshSessionFlags(state_);
        return;
    }

    if (state_.workflowMode == WorkflowMode::customConditioning
        && state_.customConditioningCodes.trim().isEmpty())
    {
        state_.jobStatus = JobStatus::failed;
        state_.transportState = TransportState::failed;
        state_.progressText = {};
        state_.errorMessage = "Custom Conditioning mode requires audio semantic codes.";
        refreshSessionFlags(state_);
        return;
    }

    clearGeneratedResults();
    stopPreview();
    clearPreviewFile();
    state_.session.sessionName = deriveSessionName(state_);
    state_.jobStatus = JobStatus::submitting;
    state_.transportState = TransportState::submitting;
    state_.progressText = "Submitting request...";
    state_.errorMessage = {};
    refreshSessionFlags(state_);
}

void ACEStepVST3AudioProcessor::selectResultSlot(int index)
{
    state_.selectedResultSlot = juce::jlimit(0, kResultSlotCount - 1, index);
    const auto& take = state_.resultTakes[static_cast<size_t>(state_.selectedResultSlot)];
    const auto& localPath = take.localFilePath.isNotEmpty()
                                ? take.localFilePath
                                : state_.resultLocalPaths[static_cast<size_t>(state_.selectedResultSlot)];
    if (localPath.isNotEmpty())
    {
        [[maybe_unused]] const auto loaded = loadPreviewFile(juce::File(localPath));
    }
}

void ACEStepVST3AudioProcessor::selectCompareSlots(int primaryIndex, int secondaryIndex)
{
    state_.session.comparePrimarySlot = juce::jlimit(0, kResultSlotCount - 1, primaryIndex);
    state_.session.compareSecondarySlot = juce::jlimit(0, kResultSlotCount - 1, secondaryIndex);
}

void ACEStepVST3AudioProcessor::cueCompareSlot(bool primarySlot)
{
    state_.compareOnPrimary = primarySlot;
    selectResultSlot(primarySlot ? state_.session.comparePrimarySlot
                                 : state_.session.compareSecondarySlot);
}

void ACEStepVST3AudioProcessor::toggleCompareSlot()
{
    cueCompareSlot(!state_.compareOnPrimary);
}

void ACEStepVST3AudioProcessor::pumpBackendWorkflow()
{
    std::optional<BackendTaskResult> completedTask;
    {
        const juce::ScopedLock lock(backendTaskLock_);
        if (completedBackendTask_.has_value())
        {
            completedTask = std::move(completedBackendTask_);
            completedBackendTask_.reset();
        }
    }

    if (completedTask.has_value())
    {
        applyCompletedTask(*completedTask);
    }

    if (backendTaskRunning_.load())
    {
        return;
    }

    if (pendingPreviewDownloadSlot_.has_value())
    {
        schedulePreviewDownload(*pendingPreviewDownloadSlot_);
        pendingPreviewDownloadSlot_.reset();
        return;
    }

    if (pendingUnloadLoRA_)
    {
        pendingUnloadLoRA_ = false;
        scheduleUnloadLoRA();
        return;
    }

    if (pendingLoadLoRA_)
    {
        pendingLoadLoRA_ = false;
        scheduleLoadLoRA();
        return;
    }

    if (pendingLoRAToggle_.has_value())
    {
        const auto useLora = *pendingLoRAToggle_;
        pendingLoRAToggle_.reset();
        scheduleToggleLoRA(useLora);
        return;
    }

    if (pendingLoRAScale_.has_value())
    {
        const auto scale = *pendingLoRAScale_;
        pendingLoRAScale_.reset();
        scheduleLoRAScale(scale);
        return;
    }

    if (pendingLoRAStatusRefresh_)
    {
        pendingLoRAStatusRefresh_ = false;
        scheduleLoRAStatusCheck();
        return;
    }

    const auto now = juce::Time::getMillisecondCounter();
    if (state_.jobStatus == JobStatus::submitting)
    {
        scheduleGenerationStart();
        return;
    }

    if (state_.jobStatus == JobStatus::queuedOrRunning && state_.currentTaskId.isNotEmpty()
        && now - lastPollRequestAtMs_ >= 1500)
    {
        scheduleGenerationPoll();
        return;
    }

    if (state_.jobStatus == JobStatus::idle || state_.jobStatus == JobStatus::failed
        || state_.jobStatus == JobStatus::succeeded)
    {
        if (lastHealthCheckedBaseUrl_ != state_.backendBaseUrl
            || now - lastHealthCheckAtMs_ >= 5000)
        {
            scheduleHealthCheck();
        }
        if (now - lastLoRAStatusCheckAtMs_ >= 7000)
        {
            scheduleLoRAStatusCheck();
        }
    }
}

void ACEStepVST3AudioProcessor::scheduleHealthCheck()
{
    if (backendTaskRunning_.exchange(true))
    {
        return;
    }

    lastHealthCheckedBaseUrl_ = state_.backendBaseUrl;
    lastHealthCheckAtMs_ = juce::Time::getMillisecondCounter();
    const auto baseUrl = state_.backendBaseUrl;
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>([this, baseUrl]() {
        BackendTaskResult taskResult;
        taskResult.kind = BackendTaskKind::healthCheck;
        taskResult.health = backendClient_.checkHealth(baseUrl);
        const juce::ScopedLock lock(backendTaskLock_);
        completedBackendTask_ = std::move(taskResult);
        backendTaskRunning_.store(false);
        return juce::ThreadPoolJob::jobHasFinished;
    }));
}

void ACEStepVST3AudioProcessor::scheduleGenerationStart()
{
    if (backendTaskRunning_.exchange(true))
    {
        return;
    }

    const auto stateSnapshot = state_;
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>([this, stateSnapshot]() {
        BackendTaskResult taskResult;
        taskResult.kind = BackendTaskKind::submitGeneration;
        taskResult.generationStart = backendClient_.startGeneration(stateSnapshot);
        const juce::ScopedLock lock(backendTaskLock_);
        completedBackendTask_ = std::move(taskResult);
        backendTaskRunning_.store(false);
        return juce::ThreadPoolJob::jobHasFinished;
    }));
}

void ACEStepVST3AudioProcessor::scheduleGenerationPoll()
{
    if (backendTaskRunning_.exchange(true))
    {
        return;
    }

    const auto baseUrl = state_.backendBaseUrl;
    const auto taskId = state_.currentTaskId;
    lastPollRequestAtMs_ = juce::Time::getMillisecondCounter();
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>([this, baseUrl, taskId]() {
        BackendTaskResult taskResult;
        taskResult.kind = BackendTaskKind::pollGeneration;
        taskResult.generationPoll = backendClient_.pollGeneration(baseUrl, taskId);
        const juce::ScopedLock lock(backendTaskLock_);
        completedBackendTask_ = std::move(taskResult);
        backendTaskRunning_.store(false);
        return juce::ThreadPoolJob::jobHasFinished;
    }));
}

void ACEStepVST3AudioProcessor::schedulePreviewDownload(int slotIndex)
{
    if (backendTaskRunning_.exchange(true))
    {
        pendingPreviewDownloadSlot_ = slotIndex;
        return;
    }

    const auto baseUrl = state_.backendBaseUrl;
    const auto remoteFileUrl = state_.resultFileUrls[static_cast<size_t>(slotIndex)];
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>(
        [this, baseUrl, remoteFileUrl, slotIndex]() {
            BackendTaskResult taskResult;
            taskResult.kind = BackendTaskKind::downloadPreview;
            taskResult.previewDownload =
                backendClient_.downloadPreviewFile(baseUrl, remoteFileUrl, slotIndex);
            const juce::ScopedLock lock(backendTaskLock_);
            completedBackendTask_ = std::move(taskResult);
            backendTaskRunning_.store(false);
            return juce::ThreadPoolJob::jobHasFinished;
        }));
}

void ACEStepVST3AudioProcessor::scheduleLoRAStatusCheck()
{
    if (backendTaskRunning_.exchange(true))
    {
        pendingLoRAStatusRefresh_ = true;
        return;
    }

    const auto baseUrl = state_.backendBaseUrl;
    lastLoRAStatusCheckAtMs_ = juce::Time::getMillisecondCounter();
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>([this, baseUrl]() {
        BackendTaskResult taskResult;
        taskResult.kind = BackendTaskKind::loraStatus;
        taskResult.loraStatus = backendClient_.getLoRAStatus(baseUrl);
        const juce::ScopedLock lock(backendTaskLock_);
        completedBackendTask_ = std::move(taskResult);
        backendTaskRunning_.store(false);
        return juce::ThreadPoolJob::jobHasFinished;
    }));
}

void ACEStepVST3AudioProcessor::scheduleLoadLoRA()
{
    if (backendTaskRunning_.exchange(true))
    {
        pendingLoadLoRA_ = true;
        return;
    }

    const auto baseUrl = state_.backendBaseUrl;
    const auto loraPath = state_.loraPath;
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>([this, baseUrl, loraPath]() {
        BackendTaskResult taskResult;
        taskResult.kind = BackendTaskKind::loraLoad;
        taskResult.loraOperation = backendClient_.loadLoRA(baseUrl, loraPath);
        const juce::ScopedLock lock(backendTaskLock_);
        completedBackendTask_ = std::move(taskResult);
        backendTaskRunning_.store(false);
        return juce::ThreadPoolJob::jobHasFinished;
    }));
}

void ACEStepVST3AudioProcessor::scheduleUnloadLoRA()
{
    if (backendTaskRunning_.exchange(true))
    {
        pendingUnloadLoRA_ = true;
        return;
    }

    const auto baseUrl = state_.backendBaseUrl;
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>([this, baseUrl]() {
        BackendTaskResult taskResult;
        taskResult.kind = BackendTaskKind::loraUnload;
        taskResult.loraOperation = backendClient_.unloadLoRA(baseUrl);
        const juce::ScopedLock lock(backendTaskLock_);
        completedBackendTask_ = std::move(taskResult);
        backendTaskRunning_.store(false);
        return juce::ThreadPoolJob::jobHasFinished;
    }));
}

void ACEStepVST3AudioProcessor::scheduleToggleLoRA(bool useLora)
{
    if (backendTaskRunning_.exchange(true))
    {
        pendingLoRAToggle_ = useLora;
        return;
    }

    const auto baseUrl = state_.backendBaseUrl;
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>([this, baseUrl, useLora]() {
        BackendTaskResult taskResult;
        taskResult.kind = BackendTaskKind::loraToggle;
        taskResult.loraOperation = backendClient_.toggleLoRA(baseUrl, useLora);
        const juce::ScopedLock lock(backendTaskLock_);
        completedBackendTask_ = std::move(taskResult);
        backendTaskRunning_.store(false);
        return juce::ThreadPoolJob::jobHasFinished;
    }));
}

void ACEStepVST3AudioProcessor::scheduleLoRAScale(double scale)
{
    if (backendTaskRunning_.exchange(true))
    {
        pendingLoRAScale_ = scale;
        return;
    }

    const auto baseUrl = state_.backendBaseUrl;
    const auto adapterName = state_.activeLoraAdapter;
    backendThreadPool_.addJob(std::function<juce::ThreadPoolJob::JobStatus()>(
        [this, baseUrl, scale, adapterName]() {
            BackendTaskResult taskResult;
            taskResult.kind = BackendTaskKind::loraScale;
            taskResult.loraOperation = backendClient_.setLoRAScale(baseUrl, scale, adapterName);
            const juce::ScopedLock lock(backendTaskLock_);
            completedBackendTask_ = std::move(taskResult);
            backendTaskRunning_.store(false);
            return juce::ThreadPoolJob::jobHasFinished;
        }));
}

void ACEStepVST3AudioProcessor::applyLoRAStatus(const PluginLoRAStatusResult& result)
{
    if (!result.succeeded)
    {
        if (result.errorMessage.isNotEmpty())
        {
            state_.errorMessage = result.errorMessage;
            state_.loraStatusText = result.errorMessage;
        }
        return;
    }

    state_.loraLoaded = result.loaded;
    state_.useLora = result.useLora;
    state_.loraScale = result.scale;
    state_.activeLoraAdapter = result.activeAdapter;
    state_.loraAdapters = result.adapters;
    if (result.loaded)
    {
        auto message = juce::String("Loaded");
        if (result.activeAdapter.isNotEmpty())
        {
            message << ": " << result.activeAdapter;
        }
        state_.loraStatusText = message;
    }
    else
    {
        state_.loraStatusText = "No LoRA loaded.";
    }
}

void ACEStepVST3AudioProcessor::applyCompletedTask(const BackendTaskResult& taskResult)
{
    switch (taskResult.kind)
    {
        case BackendTaskKind::healthCheck:
            state_.backendStatus = taskResult.health.status;
            if (taskResult.health.status == BackendStatus::ready
                && state_.jobStatus != JobStatus::failed)
            {
                state_.errorMessage = {};
            }
            else if (taskResult.health.status != BackendStatus::ready
                     && state_.jobStatus == JobStatus::idle)
            {
                state_.errorMessage = taskResult.health.errorMessage;
            }
            refreshSessionFlags(state_);
            return;
        case BackendTaskKind::submitGeneration:
            if (!taskResult.generationStart.succeeded)
            {
                state_.jobStatus = JobStatus::failed;
                state_.transportState = TransportState::failed;
                state_.progressText = {};
                state_.errorMessage = taskResult.generationStart.errorMessage;
                state_.currentTaskId = {};
                refreshSessionFlags(state_);
                return;
            }

            state_.backendStatus = BackendStatus::ready;
            state_.jobStatus = JobStatus::queuedOrRunning;
            state_.transportState = TransportState::queued;
            state_.currentTaskId = taskResult.generationStart.taskId;
            state_.progressText = "Task started: " + state_.currentTaskId;
            state_.errorMessage = {};
            refreshSessionFlags(state_);
            return;
        case BackendTaskKind::pollGeneration:
        {
            state_.jobStatus = taskResult.generationPoll.status;
            state_.progressText = taskResult.generationPoll.progressText;
            if (taskResult.generationPoll.status == JobStatus::failed)
            {
                state_.transportState = TransportState::failed;
                state_.errorMessage = taskResult.generationPoll.errorMessage;
                state_.currentTaskId = {};
                refreshSessionFlags(state_);
                return;
            }

            if (taskResult.generationPoll.status != JobStatus::succeeded)
            {
                state_.transportState = state_.progressText.isEmpty() ? TransportState::queued
                                                                     : TransportState::rendering;
                refreshSessionFlags(state_);
                return;
            }

            const auto compareGroup = state_.currentTaskId;
            state_.currentTaskId = {};
            state_.errorMessage = {};
            auto readyTakeCount = 0;
            for (int index = 0; index < kResultSlotCount; ++index)
            {
                const auto& slot = taskResult.generationPoll.resultSlots[static_cast<size_t>(index)];
                auto& take = state_.resultTakes[static_cast<size_t>(index)];
                take.slotLabel = slot.label;
                take.remoteFileUrl = slot.remoteFileUrl;
                take.localFilePath = {};
                take.statusText = "Rendered";
                take.compareGroup = compareGroup;
                take.seed = state_.seed;
                take.durationSeconds = state_.durationSeconds;
                take.modelPreset = state_.modelPreset;
                take.qualityMode = state_.qualityMode;
                take.updatedAtMs = juce::Time::currentTimeMillis();
                take.readyForCompare = slot.remoteFileUrl.isNotEmpty();
                if (take.readyForCompare)
                {
                    ++readyTakeCount;
                }
                mirrorTakeIntoLegacyFields(state_, index);
            }
            state_.selectedResultSlot = 0;
            state_.session.lastCompletedSlot = 0;
            state_.session.comparePrimarySlot = readyTakeCount > 1 ? 0 : -1;
            state_.session.compareSecondarySlot = readyTakeCount > 1 ? 1 : -1;
            state_.transportState = readyTakeCount > 1 ? TransportState::compareReady
                                                       : TransportState::succeeded;
            if (state_.resultTakes[0].remoteFileUrl.isNotEmpty())
            {
                pendingPreviewDownloadSlot_ = 0;
            }
            else
            {
                state_.errorMessage = "Task finished but no audio file was returned.";
            }
            refreshSessionFlags(state_);
            return;
        }
        case BackendTaskKind::downloadPreview:
        {
            if (!taskResult.previewDownload.succeeded)
            {
                state_.errorMessage = taskResult.previewDownload.errorMessage;
                return;
            }

            if (taskResult.previewDownload.slotIndex >= 0
                && taskResult.previewDownload.slotIndex < kResultSlotCount)
            {
                const auto slotIndex = static_cast<size_t>(taskResult.previewDownload.slotIndex);
                auto& take = state_.resultTakes[slotIndex];
                take.localFilePath = taskResult.previewDownload.localFilePath;
                take.statusText = "Preview cached";
                take.updatedAtMs = juce::Time::currentTimeMillis();
                mirrorTakeIntoLegacyFields(state_, taskResult.previewDownload.slotIndex);
                if (state_.selectedResultSlot == taskResult.previewDownload.slotIndex)
                {
                    [[maybe_unused]] const auto loaded =
                        loadPreviewFile(juce::File(taskResult.previewDownload.localFilePath));
                }
            }
            refreshSessionFlags(state_);
            return;
        case BackendTaskKind::loraStatus:
            applyLoRAStatus(taskResult.loraStatus);
            return;
        case BackendTaskKind::loraLoad:
        case BackendTaskKind::loraUnload:
        case BackendTaskKind::loraToggle:
        case BackendTaskKind::loraScale:
            if (!taskResult.loraOperation.succeeded)
            {
                state_.errorMessage = taskResult.loraOperation.errorMessage;
                if (state_.errorMessage.isEmpty())
                {
                    state_.errorMessage = "LoRA operation failed.";
                }
                state_.loraStatusText = state_.errorMessage;
                return;
            }
            state_.errorMessage = {};
            if (taskResult.loraOperation.message.isNotEmpty())
            {
                state_.loraStatusText = taskResult.loraOperation.message;
            }
            applyLoRAStatus(taskResult.loraOperation.status);
            return;
        }
        case BackendTaskKind::none:
            return;
    }
}

void ACEStepVST3AudioProcessor::clearGeneratedResults()
{
    state_.currentTaskId = {};
    state_.progressText = {};
    state_.transportState = TransportState::idle;
    state_.selectedResultSlot = 0;
    state_.session.lastCompletedSlot = -1;
    state_.session.comparePrimarySlot = -1;
    state_.session.compareSecondarySlot = -1;
    state_.compareOnPrimary = true;
    pendingPreviewDownloadSlot_.reset();
    for (int index = 0; index < kResultSlotCount; ++index)
    {
        state_.resultTakes[static_cast<size_t>(index)] = {};
        mirrorTakeIntoLegacyFields(state_, index);
    }
    refreshSessionFlags(state_);
}

void ACEStepVST3AudioProcessor::syncPreviewFromState()
{
    preview_.clear();
    if (state_.previewFilePath.isEmpty())
    {
        return;
    }

    juce::String errorMessage;
    const juce::File previewFile(state_.previewFilePath);
    if (!preview_.loadFile(previewFile, errorMessage))
    {
        state_.errorMessage = errorMessage;
        return;
    }

    state_.previewDisplayName = previewFile.getFileName();
}
}  // namespace acestep::vst3

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new acestep::vst3::ACEStepVST3AudioProcessor();
}
