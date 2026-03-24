/**
 * ACE-Step VST3 Web UI — compatible with acestep.cpp ace-server API.
 *
 * Endpoints used:
 *   GET  /props          — server health + defaults
 *   POST /lm             — enrich caption → full AceRequest[]
 *   POST /synth[?wav=1]  — AceRequest[] → audio blob(s)
 */

(function () {
  "use strict";

  // ── Config ───────────────────────────────────────────────────────────

  const HEALTH_INTERVAL_MS = 10000;
  const HEALTH_TIMEOUT_MS = 2000;

  // ── DOM refs ─────────────────────────────────────────────────────────

  const $ = (sel) => document.querySelector(sel);
  const trackListEl = $("#trackList");
  const emptyStateEl = $("#emptyState");
  const statusDot = $("#statusDot");
  const statusText = $("#statusText");
  const toastEl = $("#toast");
  const btnGenerate = $("#btnGenerate");
  const darkToggle = $("#darkToggle");
  const formatSelect = $("#formatSelect");
  const volumeSlider = $("#volumeSlider");

  // ── State ────────────────────────────────────────────────────────────

  let tracks = [];
  let audioCtx = null;
  let currentSource = null;
  let currentTrackIndex = -1;
  let playbackStartTime = 0;
  let playbackOffset = 0;
  let animFrame = null;
  const renderers = new Map();

  // ── Toast ────────────────────────────────────────────────────────────

  let toastTimer = null;
  function showToast(msg) {
    toastEl.textContent = msg;
    toastEl.classList.add("visible");
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => toastEl.classList.remove("visible"), 3000);
  }

  // ── Theme ────────────────────────────────────────────────────────────

  darkToggle.addEventListener("change", () => {
    document.documentElement.dataset.theme = darkToggle.checked ? "dark" : "";
    localStorage.setItem("ace-vst3-dark", darkToggle.checked ? "1" : "0");
    renderers.forEach((r) => r.draw());
  });

  if (localStorage.getItem("ace-vst3-dark") === "1") {
    darkToggle.checked = true;
    document.documentElement.dataset.theme = "dark";
  }

  // ── Health polling ───────────────────────────────────────────────────

  let serverDefaults = null;

  async function pollHealth() {
    try {
      const res = await fetch("props", {
        signal: AbortSignal.timeout(HEALTH_TIMEOUT_MS),
      });
      if (!res.ok) throw new Error("not ok");
      const data = await res.json();
      serverDefaults = data.default || null;

      const lm = data.status?.lm || "disabled";
      const synth = data.status?.synth || "disabled";

      if (lm === "ok" && synth === "ok") {
        statusDot.className = "status-dot ok";
        statusText.textContent = "ready";
      } else if (lm === "sleeping" || synth === "sleeping") {
        statusDot.className = "status-dot sleeping";
        statusText.textContent = "sleeping";
      } else {
        statusDot.className = "status-dot";
        statusText.textContent = lm + " / " + synth;
      }
    } catch {
      statusDot.className = "status-dot";
      statusText.textContent = "offline";
      serverDefaults = null;
    }
  }

  pollHealth();
  setInterval(pollHealth, HEALTH_INTERVAL_MS);

  // ── Build AceRequest from form ───────────────────────────────────────

  function buildRequest() {
    const req = {};

    const caption = $("#captionInput").value.trim();
    if (!caption) return null;
    req.caption = caption;

    const lyrics = $("#lyricsInput").value.trim();
    if (lyrics) req.lyrics = lyrics;

    const lang = $("#metaLanguage").value.trim();
    if (lang) req.vocal_language = lang;

    const bpm = parseInt($("#metaBpm").value, 10);
    if (bpm > 0) req.bpm = bpm;

    const dur = parseFloat($("#metaDuration").value);
    if (dur > 0) req.duration = dur;

    const key = $("#metaKey").value.trim();
    if (key) req.keyscale = key;

    const ts = $("#metaTimeSig").value.trim();
    if (ts) req.timesignature = ts;

    const seed = parseInt($("#metaSeed").value, 10);
    if (!isNaN(seed)) req.seed = seed;

    const batch = parseInt($("#metaBatch").value, 10);
    if (batch > 1) req.batch_size = batch;

    const temp = parseFloat($("#metaLmTemp").value);
    if (!isNaN(temp)) req.lm_temperature = temp;

    const cfg = parseFloat($("#metaCfg").value);
    if (!isNaN(cfg)) req.lm_cfg_scale = cfg;

    req.use_cot_caption = $("#metaCot").value === "true";

    return req;
  }

  // ── Generate ─────────────────────────────────────────────────────────

  btnGenerate.addEventListener("click", async () => {
    const req = buildRequest();
    if (!req) {
      showToast("Caption is required");
      return;
    }

    btnGenerate.disabled = true;
    btnGenerate.textContent = "LM ENRICHING...";

    try {
      // Step 1: POST /lm — enrich with metadata + audio codes
      const lmRes = await fetch("lm", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(req),
      });

      if (lmRes.status === 503) throw new Error("Server busy (LM)");
      if (lmRes.status === 501) throw new Error("LM pipeline not loaded");
      if (!lmRes.ok) {
        const err = await lmRes.json().catch(() => ({ error: lmRes.statusText }));
        throw new Error(err.error || lmRes.statusText);
      }

      const enrichedRequests = await lmRes.json();

      // Step 2: POST /synth — generate audio
      btnGenerate.textContent = "SYNTHESIZING...";

      const format = formatSelect.value;
      const synthUrl = format === "wav" ? "synth?wav=1" : "synth";
      const synthBody =
        enrichedRequests.length === 1
          ? JSON.stringify(enrichedRequests[0])
          : JSON.stringify(enrichedRequests);

      const synthRes = await fetch(synthUrl, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: synthBody,
      });

      if (synthRes.status === 503) throw new Error("Server busy (Synth)");
      if (synthRes.status === 501) throw new Error("Synth pipeline not loaded");
      if (!synthRes.ok) {
        const err = await synthRes.json().catch(() => ({ error: synthRes.statusText }));
        throw new Error(err.error || synthRes.statusText);
      }

      // Parse audio blobs
      const blobs = await parseAudioResponse(synthRes, format);

      // Add tracks
      for (let i = 0; i < blobs.length; i++) {
        const enriched = enrichedRequests[i] || enrichedRequests[0];
        const name = req.caption.substring(0, 40) + (blobs.length > 1 ? ` #${i + 1}` : "");
        const dur = enriched.duration || req.duration || 30;
        const seed = enriched.seed || req.seed || -1;
        await addTrackFromBlob(name, format, dur, seed, blobs[i], enriched);
      }

      showToast("Generated " + blobs.length + " track(s)");
    } catch (err) {
      showToast("Error: " + err.message);
      console.error("[generate]", err);
    } finally {
      btnGenerate.disabled = false;
      btnGenerate.textContent = "GENERATE";
    }
  });

  // ── Parse multipart/mixed or single audio response ───────────────────

  async function parseAudioResponse(res, format) {
    const ct = res.headers.get("Content-Type") || "";
    const mime = format === "wav" ? "audio/wav" : "audio/mpeg";

    if (!ct.startsWith("multipart/")) {
      return [await res.blob()];
    }

    const match = ct.match(/boundary=([^\s;]+)/);
    if (!match) throw new Error("Missing boundary");

    const buf = new Uint8Array(await res.arrayBuffer());
    const enc = new TextEncoder();
    const delim = enc.encode("--" + match[1]);
    const positions = [];

    for (let i = 0; i <= buf.length - delim.length; i++) {
      let ok = true;
      for (let j = 0; j < delim.length; j++) {
        if (buf[i + j] !== delim[j]) { ok = false; break; }
      }
      if (ok) positions.push(i);
    }

    const results = [];
    for (let p = 0; p < positions.length - 1; p++) {
      const partStart = positions[p] + delim.length + 2;
      const partEnd = positions[p + 1] - 2;
      if (partStart >= partEnd) continue;

      let splitAt = -1;
      for (let i = partStart; i < partEnd - 3; i++) {
        if (buf[i] === 13 && buf[i+1] === 10 && buf[i+2] === 13 && buf[i+3] === 10) {
          splitAt = i;
          break;
        }
      }
      if (splitAt < 0) continue;
      results.push(new Blob([buf.slice(splitAt + 4, partEnd)], { type: mime }));
    }

    return results;
  }

  // ── Track management ─────────────────────────────────────────────────

  async function addTrackFromBlob(name, format, durationSec, seed, blob, request) {
    if (!audioCtx) audioCtx = new AudioContext();

    const arrayBuf = await blob.arrayBuffer();
    let peaks;
    try {
      const audioBuf = await audioCtx.decodeAudioData(arrayBuf.slice(0));
      peaks = extractPeaks(audioBuf, 300);
      durationSec = audioBuf.duration;
    } catch {
      peaks = [];
    }

    const index = tracks.length;
    tracks.push({
      name, format, durationSec, seed, peaks,
      blob, request,
      currentTime: 0, playing: false,
      objectUrl: URL.createObjectURL(blob),
    });

    renderTrackList();
    return index;
  }

  function removeTrack(index) {
    if (currentTrackIndex === index) stopPlayback();
    const t = tracks[index];
    if (t && t.objectUrl) URL.revokeObjectURL(t.objectUrl);
    renderers.get(index)?.destroy();
    renderers.delete(index);
    tracks.splice(index, 1);
    if (currentTrackIndex > index) currentTrackIndex--;
    renderTrackList();
  }

  function formatTime(seconds) {
    const m = Math.floor(seconds / 60).toString().padStart(2, "0");
    const s = Math.floor(seconds % 60).toString().padStart(2, "0");
    const ms = Math.floor((seconds % 1) * 100).toString().padStart(2, "0");
    return m + ":" + s + ":" + ms;
  }

  function escapeHtml(str) {
    const div = document.createElement("div");
    div.textContent = str;
    return div.innerHTML;
  }

  function renderTrackList() {
    renderers.forEach((r) => r.destroy());
    renderers.clear();
    trackListEl.querySelectorAll(".track-card").forEach((el) => el.remove());

    if (tracks.length === 0) {
      emptyStateEl.style.display = "";
      return;
    }
    emptyStateEl.style.display = "none";

    tracks.forEach((track, i) => {
      const card = document.createElement("div");
      card.className = "track-card";
      card.innerHTML = `
        <div class="track-header">
          <div class="track-left">
            <button class="track-play-btn" data-action="play" data-index="${i}">
              <svg viewBox="0 0 24 24" width="14" height="14">
                <path fill="currentColor" d="${track.playing ? "M6 19h4V5H6v14zm8-14v14h4V5h-4z" : "M8 5v14l11-7z"}"/>
              </svg>
            </button>
            <span class="track-name">${escapeHtml(track.name)}</span>
          </div>
          <div class="track-right">
            <span class="format-badge ${track.format}">${track.format.toUpperCase()}</span>
            <span class="track-time" data-time-index="${i}">${formatTime(track.currentTime)} / ${formatTime(track.durationSec)}</span>
            <div class="track-actions">
              <button class="track-action-btn" data-action="download" data-index="${i}" title="Download">
                <svg viewBox="0 0 24 24" width="13" height="13"><path fill="currentColor" d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z"/></svg>
              </button>
              <button class="track-action-btn" data-action="delete" data-index="${i}" title="Delete">
                <svg viewBox="0 0 24 24" width="13" height="13"><path fill="currentColor" d="M6 19c0 1.1.9 2 2 2h8a2 2 0 0 0 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/></svg>
              </button>
            </div>
          </div>
        </div>
        <div class="waveform-container" data-waveform-index="${i}">
          <canvas></canvas>
        </div>
      `;
      trackListEl.appendChild(card);

      const canvas = card.querySelector("canvas");
      const renderer = new WaveformRenderer(canvas);
      renderer.setPeaks(track.peaks || []);
      renderer.setProgress(track.durationSec > 0 ? track.currentTime / track.durationSec : 0);
      renderers.set(i, renderer);

      card.querySelector(".waveform-container").addEventListener("click", (e) => {
        const pos = renderer.positionFromEvent(e);
        seekTrack(i, pos * track.durationSec);
      });
    });
  }

  function updateTimeDisplay(index) {
    const el = document.querySelector(`[data-time-index="${index}"]`);
    if (el && tracks[index]) {
      el.textContent = formatTime(tracks[index].currentTime) + " / " + formatTime(tracks[index].durationSec);
    }
  }

  // ── Playback (Web Audio API) ─────────────────────────────────────────

  async function playTrack(index) {
    if (!audioCtx) audioCtx = new AudioContext();
    if (currentTrackIndex === index && tracks[index].playing) {
      stopPlayback();
      return;
    }

    stopPlayback();

    const track = tracks[index];
    if (!track) return;

    try {
      const arrayBuf = await track.blob.arrayBuffer();
      const audioBuf = await audioCtx.decodeAudioData(arrayBuf);
      const source = audioCtx.createBufferSource();
      const gain = audioCtx.createGain();
      gain.gain.value = parseInt(volumeSlider.value, 10) / 100;
      source.buffer = audioBuf;
      source.connect(gain);
      gain.connect(audioCtx.destination);

      const offset = track.currentTime || 0;
      source.start(0, offset);

      currentSource = source;
      currentTrackIndex = index;
      playbackStartTime = audioCtx.currentTime;
      playbackOffset = offset;
      track.playing = true;
      track.durationSec = audioBuf.duration;

      source.onended = () => {
        track.playing = false;
        track.currentTime = 0;
        currentSource = null;
        currentTrackIndex = -1;
        cancelAnimationFrame(animFrame);
        renderers.get(index)?.setProgress(0);
        updateTimeDisplay(index);
        renderTrackList();
      };

      renderTrackList();
      animatePlayback();
    } catch (err) {
      showToast("Playback error: " + err.message);
    }
  }

  function stopPlayback() {
    if (currentSource) {
      try { currentSource.stop(); } catch {}
      currentSource = null;
    }
    if (currentTrackIndex >= 0 && tracks[currentTrackIndex]) {
      tracks[currentTrackIndex].playing = false;
    }
    currentTrackIndex = -1;
    cancelAnimationFrame(animFrame);
  }

  function seekTrack(index, time) {
    const track = tracks[index];
    if (!track) return;
    track.currentTime = Math.max(0, Math.min(time, track.durationSec));
    renderers.get(index)?.setProgress(track.currentTime / track.durationSec);
    updateTimeDisplay(index);

    if (track.playing) {
      playTrack(index);
    }
  }

  function animatePlayback() {
    if (currentTrackIndex < 0 || !tracks[currentTrackIndex]?.playing) return;
    const track = tracks[currentTrackIndex];
    track.currentTime = playbackOffset + (audioCtx.currentTime - playbackStartTime);
    if (track.currentTime > track.durationSec) track.currentTime = track.durationSec;
    renderers.get(currentTrackIndex)?.setProgress(track.currentTime / track.durationSec);
    updateTimeDisplay(currentTrackIndex);
    animFrame = requestAnimationFrame(animatePlayback);
  }

  volumeSlider.addEventListener("input", () => {
    // Volume changes apply on next play
  });

  // ── Track action delegation ──────────────────────────────────────────

  trackListEl.addEventListener("click", (e) => {
    const btn = e.target.closest("[data-action]");
    if (!btn) return;
    const action = btn.dataset.action;
    const index = parseInt(btn.dataset.index, 10);

    switch (action) {
      case "play":
        playTrack(index);
        break;
      case "download":
        if (tracks[index]) {
          const a = document.createElement("a");
          a.href = tracks[index].objectUrl;
          a.download = tracks[index].name + "." + tracks[index].format;
          a.click();
        }
        break;
      case "delete":
        removeTrack(index);
        break;
    }
  });

  // ── Toolbar ──────────────────────────────────────────────────────────

  $("#btnReset").addEventListener("click", () => {
    $("#captionInput").value = "";
    $("#lyricsInput").value = "";
    $("#metaLanguage").value = "";
    $("#metaBpm").value = "";
    $("#metaDuration").value = "30";
    $("#metaKey").value = "";
    $("#metaTimeSig").value = "";
    $("#metaSeed").value = "-1";
    $("#metaBatch").value = "1";
    $("#metaLmTemp").value = "0.85";
    $("#metaCfg").value = "2.0";
    showToast("Form reset");
  });

  $("#btnSave").addEventListener("click", () => {
    const state = {
      caption: $("#captionInput").value,
      lyrics: $("#lyricsInput").value,
      language: $("#metaLanguage").value,
      bpm: $("#metaBpm").value,
      duration: $("#metaDuration").value,
      key: $("#metaKey").value,
      timeSig: $("#metaTimeSig").value,
      seed: $("#metaSeed").value,
    };
    localStorage.setItem("ace-vst3-preset", JSON.stringify(state));
    showToast("Preset saved");
  });

  $("#btnOpen").addEventListener("click", () => {
    try {
      const state = JSON.parse(localStorage.getItem("ace-vst3-preset") || "{}");
      if (state.caption) $("#captionInput").value = state.caption;
      if (state.lyrics) $("#lyricsInput").value = state.lyrics;
      if (state.language) $("#metaLanguage").value = state.language;
      if (state.bpm) $("#metaBpm").value = state.bpm;
      if (state.duration) $("#metaDuration").value = state.duration;
      if (state.key) $("#metaKey").value = state.key;
      if (state.timeSig) $("#metaTimeSig").value = state.timeSig;
      if (state.seed) $("#metaSeed").value = state.seed;
      showToast("Preset loaded");
    } catch {
      showToast("No saved preset found");
    }
  });

  // ── Public API for JUCE embedding ────────────────────────────────────

  window.aceStepUI = {
    setState(state) {
      if (state.caption) $("#captionInput").value = state.caption;
      if (state.lyrics) $("#lyricsInput").value = state.lyrics;
      if (state.theme === "dark") {
        darkToggle.checked = true;
        document.documentElement.dataset.theme = "dark";
      }
    },
    addTrackFromBlob,
    showToast,
  };
})();
