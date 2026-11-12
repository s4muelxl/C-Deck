/**
 * C-Deck — Interactive Landing Page Script
 * Powers the virtual Stream Deck simulator, FAQ accordions, and audio haptics.
 */

(function () {
  'use strict';

  // --- SVG Icon Templates ---
  const SVG_ICONS = {
    play: '<svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M8 5v14l11-7z"/></svg>',
    pause: '<svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M6 19h4V5H6v14zm8-14v14h4V5h-4z"/></svg>',
    app: '<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="3"/><path d="M3 9h18"/><circle cx="6" cy="6" r="1" fill="currentColor"/><circle cx="9" cy="6" r="1" fill="currentColor"/></svg>',
    web: '<svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"/><line x1="2" y1="12" x2="22" y2="12"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/></svg>'
  };

  // --- Web Audio Context for Subtle Haptic Sound ---
  let audioCtx = null;
  let soundEnabled = true;

  function playKeySound(type = 'click') {
    if (!soundEnabled) return;
    try {
      const AudioContextClass = window.AudioContext || window.webkitAudioContext;
      if (!AudioContextClass) return;
      if (!audioCtx) audioCtx = new AudioContextClass();
      if (audioCtx.state === 'suspended') audioCtx.resume();

      const now = audioCtx.currentTime;
      const osc = audioCtx.createOscillator();
      const gain = audioCtx.createGain();

      if (type === 'volume') {
        osc.type = 'sine';
        osc.frequency.setValueAtTime(520, now);
        osc.frequency.exponentialRampToValueAtTime(740, now + 0.04);
        gain.gain.setValueAtTime(0.04, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.05);
        osc.start(now);
        osc.stop(now + 0.05);
      } else if (type === 'mute') {
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(320, now);
        gain.gain.setValueAtTime(0.05, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.07);
        osc.start(now);
        osc.stop(now + 0.07);
      } else if (type === 'media') {
        osc.type = 'sine';
        osc.frequency.setValueAtTime(440, now);
        osc.frequency.exponentialRampToValueAtTime(660, now + 0.06);
        gain.gain.setValueAtTime(0.04, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.06);
        osc.start(now);
        osc.stop(now + 0.06);
      } else if (type === 'launch') {
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(380, now);
        osc.frequency.exponentialRampToValueAtTime(840, now + 0.08);
        gain.gain.setValueAtTime(0.05, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.09);
        osc.start(now);
        osc.stop(now + 0.09);
      } else {
        osc.type = 'triangle';
        osc.frequency.setValueAtTime(400, now);
        gain.gain.setValueAtTime(0.03, now);
        gain.gain.exponentialRampToValueAtTime(0.001, now + 0.04);
        osc.start(now);
        osc.stop(now + 0.04);
      }

      osc.connect(gain);
      gain.connect(audioCtx.destination);
    } catch (e) {
      // AudioContext unavailable or blocked
    }
  }

  // --- Interactive Stream Deck Simulator State ---
  const state = {
    volume: 72,
    muted: false,
    playing: false,
    statusTimer: null
  };

  // DOM Elements
  const volumeValueEl = document.getElementById('demo-volume-value');
  const volumeTrackEl = document.getElementById('demo-volume-track');
  const volumeSliderEl = document.getElementById('demo-volume-slider');
  const statusEl = document.getElementById('demo-status-text');
  const muteBtn = document.getElementById('key-mute');
  const playBtn = document.getElementById('key-play');
  const playGlyph = document.getElementById('play-glyph');
  const playLabel = document.getElementById('play-label');
  const soundToggleBtn = document.getElementById('demo-sound-toggle');

  function setStatus(message) {
    if (!statusEl) return;
    statusEl.textContent = message;
    statusEl.classList.remove('flash');
    void statusEl.offsetWidth; // trigger reflow
    statusEl.classList.add('flash');

    if (state.statusTimer) clearTimeout(state.statusTimer);
    state.statusTimer = setTimeout(() => {
      if (statusEl) {
        statusEl.textContent = state.muted
          ? 'PRONTO · [ MUDO ATIVO ]'
          : 'PRONTO · C++ WIN32 NATIVO';
      }
    }, 2800);
  }

  function updateVolumeUI() {
    if (!volumeValueEl || !volumeTrackEl) return;

    if (state.muted) {
      volumeValueEl.innerHTML = `<span class="mute-flag">MUTADO</span> ${state.volume}%`;
      volumeTrackEl.style.width = `${state.volume}%`;
      volumeTrackEl.classList.add('is-muted');
      if (muteBtn) muteBtn.classList.add('key-active-mute');
    } else {
      volumeValueEl.textContent = `${state.volume}%`;
      volumeTrackEl.style.width = `${state.volume}%`;
      volumeTrackEl.classList.remove('is-muted');
      if (muteBtn) muteBtn.classList.remove('key-active-mute');
    }

    if (volumeSliderEl && document.activeElement !== volumeSliderEl) {
      volumeSliderEl.value = state.volume;
    }
  }

  // Simulator Actions
  function adjustVolume(delta) {
    playKeySound('volume');
    if (state.muted) {
      state.muted = false;
    }
    state.volume = Math.max(0, Math.min(100, state.volume + delta));
    updateVolumeUI();
    setStatus(`Volume ajustado para ${state.volume}%`);
  }

  function toggleMute() {
    playKeySound('mute');
    state.muted = !state.muted;
    updateVolumeUI();
    setStatus(state.muted ? 'Áudio silenciado (Mudo ativado)' : `Áudio restaurado em ${state.volume}%`);
  }

  function togglePlayPause() {
    playKeySound('media');
    state.playing = !state.playing;
    if (playGlyph) playGlyph.innerHTML = state.playing ? SVG_ICONS.pause : SVG_ICONS.play;
    if (playLabel) playLabel.textContent = state.playing ? 'PAUSAR' : 'PLAY / PAUSE';
    if (playBtn) playBtn.classList.toggle('key-active-media', state.playing);
    setStatus(state.playing ? 'Comando enviado: Reproduzir faixa' : 'Comando enviado: Pausar reprodução');
  }

  function triggerMediaSkip(direction) {
    playKeySound('media');
    setStatus(direction === 'next' ? 'Comando enviado: Próxima faixa »' : 'Comando enviado: « Faixa anterior');
  }

  function launchApp(name, target, isWeb) {
    playKeySound('launch');
    setStatus(isWeb ? `Abrindo navegador padrão: ${name}` : `Iniciando aplicativo: ${name}`);
    showToast(isWeb ? `Abrindo: ${target}` : `Executando: ${target}`, isWeb ? 'web' : 'app');
  }

  // Interactive Mini Toast inside Demo
  function showToast(text, type) {
    let toast = document.getElementById('demo-toast');
    if (!toast) {
      toast = document.createElement('div');
      toast.id = 'demo-toast';
      toast.className = 'demo-toast';
      const windowEl = document.querySelector('.app-window');
      if (windowEl) windowEl.appendChild(toast);
    }
    toast.textContent = text;
    toast.setAttribute('data-type', type || 'info');
    toast.classList.add('visible');
    setTimeout(() => {
      toast.classList.remove('visible');
    }, 2200);
  }

  // --- Setup Simulator Listeners ---
  function initSimulator() {
    const volDownBtn = document.getElementById('key-vol-down');
    const volUpBtn = document.getElementById('key-vol-up');
    const prevBtn = document.getElementById('key-prev');
    const nextBtn = document.getElementById('key-next');

    if (volDownBtn) volDownBtn.addEventListener('click', () => adjustVolume(-5));
    if (volUpBtn) volUpBtn.addEventListener('click', () => adjustVolume(5));
    if (muteBtn) muteBtn.addEventListener('click', toggleMute);
    if (playBtn) playBtn.addEventListener('click', togglePlayPause);
    if (prevBtn) prevBtn.addEventListener('click', () => triggerMediaSkip('prev'));
    if (nextBtn) nextBtn.addEventListener('click', () => triggerMediaSkip('next'));

    // Slots
    const slot1Btn = document.getElementById('key-app-1');
    const slot2Btn = document.getElementById('key-app-2');
    const slot3Btn = document.getElementById('key-app-3');
    if (slot1Btn) slot1Btn.addEventListener('click', () => launchApp('Bloco de Notas', 'C:\\Windows\\notepad.exe', false));
    if (slot2Btn) slot2Btn.addEventListener('click', () => launchApp('YouTube', 'https://www.youtube.com', true));
    if (slot3Btn) slot3Btn.addEventListener('click', () => launchApp('Calculadora', 'calc.exe', false));

    // Volume Slider interaction
    if (volumeSliderEl) {
      volumeSliderEl.addEventListener('input', (e) => {
        state.volume = parseInt(e.target.value, 10) || 0;
        if (state.muted) state.muted = false;
        updateVolumeUI();
        setStatus(`Volume: ${state.volume}%`);
      });
      volumeSliderEl.addEventListener('change', () => {
        playKeySound('volume');
      });
    }

    // Sound effect toggle
    if (soundToggleBtn) {
      soundToggleBtn.addEventListener('click', () => {
        soundEnabled = !soundEnabled;
        soundToggleBtn.textContent = soundEnabled ? '🔊 SOM: ON' : '🔇 SOM: OFF';
        soundToggleBtn.setAttribute('aria-pressed', soundEnabled ? 'true' : 'false');
        soundToggleBtn.classList.toggle('sound-muted', !soundEnabled);
        if (soundEnabled) playKeySound('click');
      });
    }

    updateVolumeUI();
  }

  // --- FAQ Accordion ---
  function initFAQ() {
    const faqItems = document.querySelectorAll('.faq-item');
    faqItems.forEach((item) => {
      const trigger = item.querySelector('.faq-question');
      if (!trigger) return;

      trigger.addEventListener('click', () => {
        const isOpen = item.classList.contains('active');
        faqItems.forEach((other) => {
          if (other !== item) {
            other.classList.remove('active');
            const otherBtn = other.querySelector('.faq-question');
            if (otherBtn) otherBtn.setAttribute('aria-expanded', 'false');
          }
        });

        item.classList.toggle('active', !isOpen);
        trigger.setAttribute('aria-expanded', !isOpen ? 'true' : 'false');
        playKeySound('click');
      });
    });
  }

  // Initialize once DOM is ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => {
      initSimulator();
      initFAQ();
    });
  } else {
    initSimulator();
    initFAQ();
  }
})();
