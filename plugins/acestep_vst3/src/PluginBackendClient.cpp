#include "PluginBackendClient.h"

namespace acestep::vst3
{
namespace
{
constexpr int kNetworkTimeoutMs = 15000;

juce::String normalizeBaseUrl(juce::String baseUrl)
{
    baseUrl = baseUrl.trim();
    while (baseUrl.endsWithChar('/'))
    {
        baseUrl = baseUrl.dropLastCharacters(1);
    }
    return baseUrl;
}

juce::URL buildUrl(const juce::String& baseUrl, const juce::String& path)
{
    if (path.startsWithIgnoreCase("http://") || path.startsWithIgnoreCase("https://"))
    {
        return juce::URL(path);
    }

    const auto normalizedBaseUrl = normalizeBaseUrl(baseUrl);
    const auto normalizedPath = path.startsWithChar('/') ? path : "/" + path;
    return juce::URL(normalizedBaseUrl + normalizedPath);
}

bool readJsonResponse(const juce::URL& url,
                      const juce::String& httpMethod,
                      const juce::String& requestBody,
                      const juce::String& contentType,
                      juce::var& parsedResponse,
                      juce::String& errorMessage)
{
    int statusCode = 0;
    auto requestUrl = requestBody.isNotEmpty() ? url.withPOSTData(requestBody) : url;
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                       .withConnectionTimeoutMs(kNetworkTimeoutMs)
                       .withStatusCode(&statusCode)
                       .withNumRedirectsToFollow(4)
                       .withHttpRequestCmd(httpMethod)
                       .withExtraHeaders(contentType.isEmpty() ? juce::String()
                                                              : "Content-Type: " + contentType
                                                                    + "\r\n");

    auto stream = requestUrl.createInputStream(options);
    if (stream == nullptr)
    {
        errorMessage = "Could not connect to the ACE-Step backend.";
        return false;
    }

    const auto responseText = stream->readEntireStreamAsString();
    parsedResponse = juce::JSON::parse(responseText);
    if (parsedResponse.isVoid())
    {
        errorMessage = responseText.isEmpty() ? "Backend returned an empty response."
                                              : "Backend returned invalid JSON.";
        return false;
    }

    auto* rootObject = parsedResponse.getDynamicObject();
    if (rootObject == nullptr)
    {
        errorMessage = "Backend returned an unexpected response shape.";
        return false;
    }

    if (statusCode != 200)
    {
        errorMessage = rootObject->getProperty("error").toString();
        if (errorMessage.isEmpty())
        {
            errorMessage = "Backend request failed with HTTP " + juce::String(statusCode) + ".";
        }
        return false;
    }

    return true;
}

juce::DynamicObject* getWrappedDataObject(const juce::var& response)
{
    auto* rootObject = response.getDynamicObject();
    if (rootObject == nullptr)
    {
        return nullptr;
    }

    if (static_cast<int>(rootObject->getProperty("code")) != 200)
    {
        return nullptr;
    }

    return rootObject->getProperty("data").getDynamicObject();
}

juce::String buildResultLabel(const juce::DynamicObject& itemObject, int slotIndex)
{
    const auto prompt = itemObject.getProperty("prompt").toString().trim();
    const auto promptLabel = prompt.isEmpty() ? "Untitled prompt" : prompt.substring(0, 28);
    const auto metasVar = itemObject.getProperty("metas");
    const auto* metas = metasVar.getDynamicObject();
    const auto duration = metas != nullptr ? metas->getProperty("duration").toString() : juce::String();
    const auto keyScale = metas != nullptr ? metas->getProperty("keyscale").toString() : juce::String();
    juce::String suffix;
    if (duration.isNotEmpty())
    {
        suffix << duration << "s";
    }
    if (keyScale.isNotEmpty())
    {
        if (suffix.isNotEmpty())
        {
            suffix << ", ";
        }
        suffix << keyScale;
    }

    juce::String label = "Result " + juce::String(slotIndex + 1) + " - " + promptLabel;
    if (suffix.isNotEmpty())
    {
        label << " (" << suffix << ")";
    }
    return label;
}

juce::String chooseModelName(ModelPreset preset)
{
    switch (preset)
    {
        case ModelPreset::turbo:
            return "acestep-v15-turbo";
        case ModelPreset::standard:
        case ModelPreset::quality:
            break;
    }

    return {};
}

int chooseInferenceSteps(QualityMode qualityMode)
{
    switch (qualityMode)
    {
        case QualityMode::fast:
            return 6;
        case QualityMode::balanced:
            return 8;
        case QualityMode::high:
            return 12;
    }

    return 8;
}

bool modeNeedsPrompt(WorkflowMode mode)
{
    return mode == WorkflowMode::text || mode == WorkflowMode::reference
           || mode == WorkflowMode::coverRemix;
}

juce::String chooseFileExtension(const juce::String& remoteFileUrl)
{
    const auto decoded = juce::URL::removeEscapeChars(remoteFileUrl);
    const auto dotIndex = decoded.lastIndexOfChar('.');
    if (dotIndex < 0)
    {
        return ".wav";
    }

    const auto extension = decoded.substring(dotIndex);
    if (extension.length() > 8 || extension.containsAnyOf("/?&"))
    {
        return ".wav";
    }

    return extension;
}

PluginLoRAStatusResult parseLoRAStatusResponse(const juce::var& response)
{
    PluginLoRAStatusResult result;
    auto* dataObject = getWrappedDataObject(response);
    if (dataObject == nullptr)
    {
        result.errorMessage = "Backend did not return LoRA status data.";
        return result;
    }

    result.succeeded = true;
    result.loaded = static_cast<bool>(dataObject->getProperty("lora_loaded"));
    result.useLora = static_cast<bool>(dataObject->getProperty("use_lora"));
    result.scale = static_cast<double>(dataObject->getProperty("lora_scale"));
    result.activeAdapter = dataObject->getProperty("active_adapter").toString();
    if (const auto adaptersVar = dataObject->getProperty("adapters"); adaptersVar.isArray())
    {
        for (const auto& item : *adaptersVar.getArray())
        {
            const auto adapter = item.toString();
            if (adapter.isNotEmpty())
            {
                result.adapters.addIfNotAlreadyThere(adapter);
            }
        }
    }
    return result;
}

PluginLoRAOperationResult buildLoRAOperationResult(const juce::String& baseUrl,
                                                   const juce::var& response,
                                                   const PluginBackendClient& client)
{
    PluginLoRAOperationResult result;
    auto* dataObject = getWrappedDataObject(response);
    if (dataObject == nullptr)
    {
        result.errorMessage = "Backend did not return a LoRA operation payload.";
        return result;
    }

    result.message = dataObject->getProperty("message").toString();
    result.status = client.getLoRAStatus(baseUrl);
    result.succeeded = result.status.succeeded;
    if (!result.succeeded && result.errorMessage.isEmpty())
    {
        result.errorMessage = result.status.errorMessage;
    }
    return result;
}
}  // namespace

PluginHealthCheckResult PluginBackendClient::checkHealth(const juce::String& baseUrl) const
{
    PluginHealthCheckResult result;
    juce::var response;
    juce::String errorMessage;
    if (!readJsonResponse(buildUrl(baseUrl, "/health"), "GET", {}, {}, response, errorMessage))
    {
        result.status = BackendStatus::offline;
        result.errorMessage = errorMessage;
        return result;
    }

    auto* rootObject = response.getDynamicObject();
    auto* dataObject = rootObject != nullptr ? rootObject->getProperty("data").getDynamicObject() : nullptr;
    const auto status = dataObject != nullptr ? dataObject->getProperty("status").toString() : juce::String();
    result.status = status == "ok" ? BackendStatus::ready : BackendStatus::degraded;
    if (result.status != BackendStatus::ready)
    {
        result.errorMessage = "Backend health check returned a degraded status.";
    }
    return result;
}

PluginGenerationStartResult PluginBackendClient::startGeneration(const PluginState& state) const
{
    PluginGenerationStartResult result;

    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty("prompt", modeNeedsPrompt(state.workflowMode) ? state.prompt : juce::String());
    payload->setProperty("lyrics", state.lyrics);
    payload->setProperty("task_type",
                         state.workflowMode == WorkflowMode::coverRemix ? "cover" : "text2music");
    payload->setProperty("audio_duration", state.durationSeconds);
    payload->setProperty("batch_size", 1);
    payload->setProperty("audio_format", "wav");
    payload->setProperty("thinking", false);
    payload->setProperty("use_random_seed", false);
    payload->setProperty("seed", state.seed);
    payload->setProperty("inference_steps", chooseInferenceSteps(state.qualityMode));
    payload->setProperty("guidance_scale", 7.0);

    if (const auto modelName = chooseModelName(state.modelPreset); modelName.isNotEmpty())
    {
        payload->setProperty("model", modelName);
    }

    switch (state.workflowMode)
    {
        case WorkflowMode::text:
            break;
        case WorkflowMode::reference:
            payload->setProperty("reference_audio_path", state.referenceAudioPath);
            break;
        case WorkflowMode::coverRemix:
            payload->setProperty("src_audio_path", state.sourceAudioPath);
            payload->setProperty("audio_cover_strength", state.audioCoverStrength);
            if (state.referenceAudioPath.isNotEmpty())
            {
                payload->setProperty("reference_audio_path", state.referenceAudioPath);
            }
            break;
        case WorkflowMode::customConditioning:
            payload->setProperty("audio_code_string", state.customConditioningCodes);
            payload->setProperty("audio_cover_strength", state.audioCoverStrength);
            break;
    }

    juce::var response;
    juce::String errorMessage;
    if (!readJsonResponse(buildUrl(state.backendBaseUrl, "/release_task"),
                          "POST",
                          juce::JSON::toString(juce::var(payload.get())),
                          "application/json",
                          response,
                          errorMessage))
    {
        result.errorMessage = errorMessage;
        return result;
    }

    auto* dataObject = getWrappedDataObject(response);
    if (dataObject == nullptr)
    {
        auto* rootObject = response.getDynamicObject();
        result.errorMessage = rootObject != nullptr ? rootObject->getProperty("error").toString()
                                                    : "Backend did not return a task id.";
        if (result.errorMessage.isEmpty())
        {
            result.errorMessage = "Backend did not return a task id.";
        }
        return result;
    }

    result.taskId = dataObject->getProperty("task_id").toString();
    result.succeeded = result.taskId.isNotEmpty();
    if (!result.succeeded)
    {
        result.errorMessage = "Backend accepted the request but did not return a task id.";
    }
    return result;
}

PluginGenerationPollResult PluginBackendClient::pollGeneration(const juce::String& baseUrl,
                                                               const juce::String& taskId) const
{
    PluginGenerationPollResult result;
    result.status = JobStatus::failed;

    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    juce::Array<juce::var> taskIds;
    taskIds.add(taskId);
    payload->setProperty("task_id_list", juce::var(taskIds));

    juce::var response;
    juce::String errorMessage;
    if (!readJsonResponse(buildUrl(baseUrl, "/query_result"),
                          "POST",
                          juce::JSON::toString(juce::var(payload.get())),
                          "application/json",
                          response,
                          errorMessage))
    {
        result.errorMessage = errorMessage;
        return result;
    }

    auto* rootObject = response.getDynamicObject();
    if (rootObject == nullptr)
    {
        result.errorMessage = "Backend returned an invalid query_result payload.";
        return result;
    }

    const auto dataVar = rootObject->getProperty("data");
    if (!dataVar.isArray() || dataVar.size() <= 0)
    {
        result.status = JobStatus::queuedOrRunning;
        result.progressText = "Waiting for backend result...";
        return result;
    }

    auto* taskObject = dataVar[0].getDynamicObject();
    if (taskObject == nullptr)
    {
        result.errorMessage = "Backend returned an invalid task payload.";
        return result;
    }

    const auto statusCode = static_cast<int>(taskObject->getProperty("status"));
    result.progressText = taskObject->getProperty("progress_text").toString();

    if (statusCode == 0)
    {
        result.status = JobStatus::queuedOrRunning;
        if (result.progressText.isEmpty())
        {
            result.progressText = "Queued / Running";
        }
        return result;
    }

    const auto parsedResult = juce::JSON::parse(taskObject->getProperty("result").toString());
    if (!parsedResult.isArray() || parsedResult.size() <= 0)
    {
        result.status = statusCode == 1 ? JobStatus::succeeded : JobStatus::failed;
        result.errorMessage = statusCode == 1 ? "Task finished without any audio file." : "Task failed.";
        return result;
    }

    if (statusCode == 2)
    {
        result.status = JobStatus::failed;
        auto* failedItem = parsedResult[0].getDynamicObject();
        result.errorMessage = failedItem != nullptr ? failedItem->getProperty("error").toString()
                                                    : "Task failed.";
        if (result.errorMessage.isEmpty())
        {
            result.errorMessage = "Task failed.";
        }
        return result;
    }

    result.status = JobStatus::succeeded;
    for (int index = 0; index < juce::jmin(kResultSlotCount, parsedResult.size()); ++index)
    {
        auto* itemObject = parsedResult[index].getDynamicObject();
        if (itemObject == nullptr)
        {
            continue;
        }

        auto& slot = result.resultSlots[static_cast<size_t>(index)];
        slot.remoteFileUrl = itemObject->getProperty("url").toString();
        if (slot.remoteFileUrl.isEmpty())
        {
            slot.remoteFileUrl = itemObject->getProperty("file").toString();
        }
        slot.label = buildResultLabel(*itemObject, index);
    }

    return result;
}

PluginPreviewDownloadResult PluginBackendClient::downloadPreviewFile(const juce::String& baseUrl,
                                                                     const juce::String& remoteFileUrl,
                                                                     int slotIndex) const
{
    PluginPreviewDownloadResult result;
    result.slotIndex = slotIndex;
    if (remoteFileUrl.isEmpty())
    {
        result.errorMessage = "The backend did not return an audio file URL.";
        return result;
    }

    int statusCode = 0;
    auto stream = buildUrl(baseUrl, remoteFileUrl)
                      .createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                             .withConnectionTimeoutMs(kNetworkTimeoutMs)
                                             .withStatusCode(&statusCode)
                                             .withNumRedirectsToFollow(4)
                                             .withHttpRequestCmd("GET"));
    if (stream == nullptr || statusCode != 200)
    {
        result.errorMessage = "Could not download the generated preview audio.";
        return result;
    }

    auto previewFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getNonexistentChildFile("acestep-vst3-preview-"
                                                        + juce::String(slotIndex + 1),
                                                    chooseFileExtension(remoteFileUrl),
                                                    false);
    juce::FileOutputStream output(previewFile);
    if (!output.openedOk())
    {
        result.errorMessage = "Could not create a temporary preview file.";
        return result;
    }

    output.writeFromInputStream(*stream, -1);
    output.flush();

    result.succeeded = true;
    result.localFilePath = previewFile.getFullPathName();
    result.displayName = previewFile.getFileName();
    return result;
}

PluginLoRAStatusResult PluginBackendClient::getLoRAStatus(const juce::String& baseUrl) const
{
    juce::var response;
    juce::String errorMessage;
    if (!readJsonResponse(buildUrl(baseUrl, "/v1/lora/status"),
                          "GET",
                          {},
                          {},
                          response,
                          errorMessage))
    {
        PluginLoRAStatusResult result;
        result.errorMessage = errorMessage;
        return result;
    }

    return parseLoRAStatusResponse(response);
}

PluginLoRAOperationResult PluginBackendClient::loadLoRA(const juce::String& baseUrl,
                                                        const juce::String& loraPath) const
{
    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty("lora_path", loraPath);

    juce::var response;
    juce::String errorMessage;
    if (!readJsonResponse(buildUrl(baseUrl, "/v1/lora/load"),
                          "POST",
                          juce::JSON::toString(juce::var(payload.get())),
                          "application/json",
                          response,
                          errorMessage))
    {
        PluginLoRAOperationResult result;
        result.errorMessage = errorMessage;
        return result;
    }

    return buildLoRAOperationResult(baseUrl, response, *this);
}

PluginLoRAOperationResult PluginBackendClient::unloadLoRA(const juce::String& baseUrl) const
{
    juce::var response;
    juce::String errorMessage;
    if (!readJsonResponse(buildUrl(baseUrl, "/v1/lora/unload"),
                          "POST",
                          "{}",
                          "application/json",
                          response,
                          errorMessage))
    {
        PluginLoRAOperationResult result;
        result.errorMessage = errorMessage;
        return result;
    }

    return buildLoRAOperationResult(baseUrl, response, *this);
}

PluginLoRAOperationResult PluginBackendClient::toggleLoRA(const juce::String& baseUrl,
                                                          bool useLora) const
{
    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty("use_lora", useLora);

    juce::var response;
    juce::String errorMessage;
    if (!readJsonResponse(buildUrl(baseUrl, "/v1/lora/toggle"),
                          "POST",
                          juce::JSON::toString(juce::var(payload.get())),
                          "application/json",
                          response,
                          errorMessage))
    {
        PluginLoRAOperationResult result;
        result.errorMessage = errorMessage;
        return result;
    }

    return buildLoRAOperationResult(baseUrl, response, *this);
}

PluginLoRAOperationResult PluginBackendClient::setLoRAScale(const juce::String& baseUrl,
                                                            double scale,
                                                            const juce::String& adapterName) const
{
    juce::DynamicObject::Ptr payload = new juce::DynamicObject();
    payload->setProperty("scale", scale);
    if (adapterName.isNotEmpty())
    {
        payload->setProperty("adapter_name", adapterName);
    }

    juce::var response;
    juce::String errorMessage;
    if (!readJsonResponse(buildUrl(baseUrl, "/v1/lora/scale"),
                          "POST",
                          juce::JSON::toString(juce::var(payload.get())),
                          "application/json",
                          response,
                          errorMessage))
    {
        PluginLoRAOperationResult result;
        result.errorMessage = errorMessage;
        return result;
    }

    return buildLoRAOperationResult(baseUrl, response, *this);
}
}  // namespace acestep::vst3
