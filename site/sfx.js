// SÉANCE's sound effects, all made with the Web Audio API (no files). The browser only allows sound after a key or
// tap, so the audio wakes on the first one. M mutes.
var Sfx = (function () {
  "use strict";
  var ctx = null, out = null, muted = localStorage.getItem("seanceMuted") === "1";

  function wake() {
    if (!ctx) {
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return;
      ctx = new AC();
      out = ctx.createGain();
      out.gain.value = muted ? 0 : 0.32;
      out.connect(ctx.destination);
    }
    if (ctx.state === "suspended") ctx.resume();
  }
  ["keydown", "pointerdown", "touchstart"].forEach(function (e) { document.addEventListener(e, wake, true); });

  function tone(type, f0, f1, at, len, vol, attack) {
    var o = ctx.createOscillator(), g = ctx.createGain();
    o.type = type;
    o.frequency.setValueAtTime(f0, at);
    o.frequency.exponentialRampToValueAtTime(f1, at + len);
    g.gain.setValueAtTime(0.0001, at);
    g.gain.exponentialRampToValueAtTime(vol, at + (attack || 0.005));
    g.gain.exponentialRampToValueAtTime(0.0001, at + len);
    o.connect(g); g.connect(out);
    o.start(at); o.stop(at + len + 0.05);
    return o;
  }

  function noise(at, len, vol, freq) {
    var n = Math.floor(ctx.sampleRate * len), buf = ctx.createBuffer(1, n, ctx.sampleRate), d = buf.getChannelData(0);
    for (var i = 0; i < n; i++) d[i] = (Math.random() * 2 - 1) * (1 - i / n);
    var s = ctx.createBufferSource(), g = ctx.createGain(), f = ctx.createBiquadFilter();
    f.type = "lowpass"; f.frequency.value = freq || 2000;
    s.buffer = buf; g.gain.value = vol;
    s.connect(f); f.connect(g); g.connect(out);
    s.start(at);
  }

  function ok() { return ctx && !muted; }

  return {
    jump: function () { if (!ok()) return; var t = ctx.currentTime; tone("square", 330, 520, t, 0.09, 0.06); },
    land: function () { if (!ok()) return; noise(ctx.currentTime, 0.05, 0.12, 600); },
    plate: function (on) {
      if (!ok()) return; var t = ctx.currentTime;
      tone("triangle", on ? 220 : 180, on ? 330 : 120, t, 0.12, 0.25);
    },
    door: function () { if (!ok()) return; var t = ctx.currentTime; tone("sawtooth", 70, 55, t, 0.35, 0.08, 0.05); noise(t, 0.3, 0.06, 400); },
    lever: function () { if (!ok()) return; var t = ctx.currentTime; tone("square", 880, 660, t, 0.06, 0.08); tone("square", 990, 990, t + 0.07, 0.06, 0.06); },
    die: function () {
      if (!ok()) return; var t = ctx.currentTime;
      tone("square", 520, 60, t, 0.6, 0.18); noise(t, 0.4, 0.25, 1500);
    },
    rewind: function () {                 // a tape running backwards
      if (!ok()) return; var t = ctx.currentTime;
      tone("sawtooth", 1400, 90, t, 0.65, 0.07, 0.02); tone("sine", 700, 45, t, 0.65, 0.18, 0.02);
    },
    summon: function () {                 // a cold choir: detuned sines swelling up
      if (!ok()) return; var t = ctx.currentTime;
      [261.6, 311.1, 392, 523.3].forEach(function (f, i) {
        var o = tone("sine", f, f * 1.003, t + i * 0.06, 1.6, 0.09, 0.4);
        var l = ctx.createOscillator(), lg = ctx.createGain();
        l.frequency.value = 5 + i; lg.gain.value = f * 0.01;
        l.connect(lg); lg.connect(o.frequency); l.start(t); l.stop(t + 1.8);
      });
    },
    no: function () { if (!ok()) return; var t = ctx.currentTime; tone("triangle", 160, 150, t, 0.25, 0.25); tone("triangle", 120, 110, t + 0.22, 0.35, 0.25); },
    win: function () {
      if (!ok()) return; var t = ctx.currentTime;
      [523.3, 659.3, 784, 1046.5, 1318.5].forEach(function (f, i) { tone("triangle", f, f, t + i * 0.09, 0.5, 0.18); tone("sine", f * 2, f * 2, t + i * 0.09, 0.3, 0.05); });
    },
    toggle: function () {
      muted = !muted;
      localStorage.setItem("seanceMuted", muted ? "1" : "0");
      if (out) out.gain.value = muted ? 0 : 0.32;
      return muted;
    },
    isMuted: function () { return muted; }
  };
})();
