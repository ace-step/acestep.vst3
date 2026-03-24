/**
 * Waveform renderer — draws peak data on <canvas> with played/unplayed coloring.
 */

class WaveformRenderer {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext("2d");
    this.peaks = [];
    this.progress = 0;
    this._resizeObserver = new ResizeObserver(() => this._fitCanvas());
    this._resizeObserver.observe(canvas.parentElement);
    this._fitCanvas();
  }

  setPeaks(peaks) {
    this.peaks = peaks;
    this.draw();
  }

  setProgress(progress) {
    this.progress = Math.max(0, Math.min(1, progress));
    this.draw();
  }

  destroy() {
    this._resizeObserver.disconnect();
  }

  _fitCanvas() {
    const rect = this.canvas.parentElement.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    this.canvas.width = rect.width * dpr;
    this.canvas.height = rect.height * dpr;
    this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    this.draw();
  }

  draw() {
    const { ctx, canvas, peaks, progress } = this;
    const w = canvas.width / (window.devicePixelRatio || 1);
    const h = canvas.height / (window.devicePixelRatio || 1);
    const style = getComputedStyle(document.documentElement);
    const playedColor = style.getPropertyValue("--waveform-played").trim() || "#2ea043";
    const unplayedColor = style.getPropertyValue("--waveform-unplayed").trim() || "#b8dbbe";
    const cursorColor = style.getPropertyValue("--waveform-cursor").trim() || "#18a0fb";

    ctx.clearRect(0, 0, w, h);

    if (peaks.length === 0) {
      ctx.fillStyle = unplayedColor;
      ctx.fillRect(0, h / 2 - 0.5, w, 1);
      return;
    }

    const barWidth = Math.max(1, w / peaks.length);
    const midY = h / 2;
    const cursorX = progress * w;

    for (let i = 0; i < peaks.length; i++) {
      const x = (i / peaks.length) * w;
      const amp = Math.abs(peaks[i]);
      const barH = Math.max(1, amp * midY * 0.9);
      ctx.fillStyle = x < cursorX ? playedColor : unplayedColor;
      ctx.fillRect(x, midY - barH, barWidth - 0.5, barH * 2);
    }

    if (progress > 0 && progress < 1) {
      ctx.fillStyle = cursorColor;
      ctx.fillRect(cursorX - 0.5, 0, 1.5, h);
    }
  }

  positionFromEvent(event) {
    const rect = this.canvas.getBoundingClientRect();
    return Math.max(0, Math.min(1, (event.clientX - rect.left) / rect.width));
  }
}

/**
 * Extract peaks from an AudioBuffer for waveform visualization.
 */
function extractPeaks(audioBuffer, count = 300) {
  const channel = audioBuffer.getChannelData(0);
  const peaks = new Array(count);
  const samplesPerPeak = Math.floor(channel.length / count);
  for (let i = 0; i < count; i++) {
    let max = 0;
    const start = i * samplesPerPeak;
    for (let j = start; j < start + samplesPerPeak && j < channel.length; j++) {
      const abs = Math.abs(channel[j]);
      if (abs > max) max = abs;
    }
    peaks[i] = max;
  }
  return peaks;
}

window.WaveformRenderer = WaveformRenderer;
window.extractPeaks = extractPeaks;
