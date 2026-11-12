/**
 * C-Deck — Interactive Landing Page Script
 * Powers the virtual Stream Deck simulator, FAQ accordions, audio effects, and UI helpers.
 */

(function () {
  'use strict';

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
      // AudioContext unavailable or blocked by autoplay policy
    }
  }

  // --- Interactive Stream Deck Simulator State ---
  const state = {
    volume: 72,
    muted: false,
    playing: false,
    statusTimer: null,
    slots: [
      { name: 'NOTEPAD', path: 'C:\\Windows\\System32\\notepad.exe', type: 'app', glyph: '▣' },
      { name: 'YOUTUBE', path: 'https://www.youtube.com', type: 'web', glyph: '↗' },
      { name: 'CALC', path: 'C:\\Windows\\System32\\calc.exe', type: 'app', glyph: '▣' }
    ]
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
  const configModal = document.getElementById('demo-config-modal');
  const openConfigBtn = document.getElementById('demo-open-config');
  const closeConfigBtn = document.getElementById('demo-close-config');
  const saveConfigBtn = document.getElementById('demo-save-config');
  const resetConfigBtn = document.getElementById('demo-reset-config');

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
          : 'PRONTO · C11 WIN32 NATIVO';
      }
    }, 3200);
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

  function updateSlotKeys() {
    state.slots.forEach((slot, index) => {
      const keyEl = document.getElementById(`key-app-${index + 1}`);
      if (!keyEl) return;

      const badgeEl = keyEl.querySelector('.key-badge');
      const glyphEl = keyEl.querySelector('.key-glyph');
      const labelEl = keyEl.querySelector('.key-label');

      const isWeb = slot.path.startsWith('http://') || slot.path.startsWith('https://');
      const slotType = isWeb ? 'WEB' : 'APP';
      const slotGlyph = isWeb ? '↗' : '▣';

      if (badgeEl) badgeEl.textContent = `0${index + 1} · ${slotType}`;
      if (glyphEl) {
        glyphEl.textContent = slotGlyph;
        glyphEl.className = `key-glyph key-icon ${isWeb ? 'glyph-web' : 'glyph-app'}`;
      }
      if (labelEl) labelEl.textContent = (slot.name || 'SEM NOME').toUpperCase();
    });
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
    if (playGlyph) playGlyph.textContent = state.playing ? '⏸' : '▶';
    if (playLabel) playLabel.textContent = state.playing ? 'PAUSAR' : 'PLAY / PAUSE';
    if (playBtn) playBtn.classList.toggle('key-active-media', state.playing);
    setStatus(state.playing ? 'Comando enviado: Reproduzir faixa' : 'Comando enviado: Pausar reprodução');
  }

  function triggerMediaSkip(direction) {
    playKeySound('media');
    setStatus(direction === 'next' ? 'Comando enviado: Próxima faixa »' : 'Comando enviado: « Faixa anterior');
  }

  function launchSlot(index) {
    const slot = state.slots[index];
    if (!slot || !slot.path) {
      setStatus(`Atalho ${index + 1} não configurado.`);
      return;
    }
    playKeySound('launch');

    const isWeb = slot.path.startsWith('http://') || slot.path.startsWith('https://');
    if (isWeb) {
      setStatus(`Abrindo navegador padrão: ${slot.name} (${slot.path})`);
      // Provide simulated user feedback with visual toast
      showToast(`Abrindo link no navegador: ${slot.path}`, 'web');
    } else {
      setStatus(`Iniciando aplicativo: ${slot.name} (${slot.path})`);
      showToast(`Executando: ${slot.path}`, 'app');
    }
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
    }, 2400);
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

    // Slots 1, 2, 3
    const slot1Btn = document.getElementById('key-app-1');
    const slot2Btn = document.getElementById('key-app-2');
    const slot3Btn = document.getElementById('key-app-3');
    if (slot1Btn) slot1Btn.addEventListener('click', () => launchSlot(0));
    if (slot2Btn) slot2Btn.addEventListener('click', () => launchSlot(1));
    if (slot3Btn) slot3Btn.addEventListener('click', () => launchSlot(2));

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

    // Config Modal in Mockup
    if (openConfigBtn && configModal) {
      openConfigBtn.addEventListener('click', () => {
        playKeySound('click');
        populateConfigInputs();
        configModal.classList.add('active');
        configModal.setAttribute('aria-hidden', 'false');
      });
    }

    if (closeConfigBtn && configModal) {
      closeConfigBtn.addEventListener('click', () => {
        playKeySound('click');
        configModal.classList.remove('active');
        configModal.setAttribute('aria-hidden', 'true');
      });
    }

    if (saveConfigBtn && configModal) {
      saveConfigBtn.addEventListener('click', () => {
        playKeySound('volume');
        saveConfigFromInputs();
        configModal.classList.remove('active');
        configModal.setAttribute('aria-hidden', 'true');
        updateSlotKeys();
        setStatus('Configuração salva com sucesso! Botões atualizados.');
        showToast('Atalhos salvos em config/apps.cfg', 'app');
      });
    }

    if (resetConfigBtn) {
      resetConfigBtn.addEventListener('click', () => {
        state.slots = [
          { name: 'NOTEPAD', path: 'C:\\Windows\\System32\\notepad.exe', type: 'app', glyph: '▣' },
          { name: 'YOUTUBE', path: 'https://www.youtube.com', type: 'web', glyph: '↗' },
          { name: 'CALC', path: 'C:\\Windows\\System32\\calc.exe', type: 'app', glyph: '▣' }
        ];
        populateConfigInputs();
        updateSlotKeys();
        setStatus('Atalhos restaurados para o padrão.');
      });
    }

    updateVolumeUI();
    updateSlotKeys();
  }

  function populateConfigInputs() {
    state.slots.forEach((slot, index) => {
      const nameInput = document.getElementById(`config-name-${index + 1}`);
      const pathInput = document.getElementById(`config-path-${index + 1}`);
      if (nameInput) nameInput.value = slot.name;
      if (pathInput) pathInput.value = slot.path;
    });
  }

  function saveConfigFromInputs() {
    state.slots.forEach((slot, index) => {
      const nameInput = document.getElementById(`config-name-${index + 1}`);
      const pathInput = document.getElementById(`config-path-${index + 1}`);
      if (nameInput && pathInput) {
        slot.name = nameInput.value.trim() || `Atalho ${index + 1}`;
        slot.path = pathInput.value.trim() || '';
      }
    });
  }

  // --- FAQ Accordion ---
  function initFAQ() {
    const faqItems = document.querySelectorAll('.faq-item');
    faqItems.forEach((item) => {
      const trigger = item.querySelector('.faq-question');
      if (!trigger) return;

      trigger.addEventListener('click', () => {
        const isOpen = item.classList.contains('active');
        // Close other items for single accordion behavior
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

  // --- Copy Snippets ---
  function initCopyButtons() {
    const copyButtons = document.querySelectorAll('[data-copy-target]');
    copyButtons.forEach((btn) => {
      btn.addEventListener('click', () => {
        const targetId = btn.getAttribute('data-copy-target');
        const targetEl = document.getElementById(targetId);
        if (!targetEl) return;

        const textToCopy = targetEl.textContent.trim();
        navigator.clipboard.writeText(textToCopy).then(() => {
          const originalText = btn.innerHTML;
          btn.innerHTML = '<span>✓</span> Copiado!';
          btn.classList.add('copied');
          playKeySound('click');
          setTimeout(() => {
            btn.innerHTML = originalText;
            btn.classList.remove('copied');
          }, 2200);
        }).catch(() => {
          // Fallback if clipboard API restricted
          const temp = document.createElement('textarea');
          temp.value = textToCopy;
          document.body.appendChild(temp);
          temp.select();
          document.execCommand('copy');
          document.body.removeChild(temp);
          btn.innerHTML = '<span>✓</span> Copiado!';
          setTimeout(() => {
            btn.textContent = 'Copiar';
          }, 2200);
        });
      });
    });
  }

  // --- Sticky Header and Active Links ---
  function initHeader() {
    const header = document.querySelector('.site-header');
    const mobileToggle = document.getElementById('mobile-menu-toggle');
    const navLinks = document.querySelector('.nav-links');

    if (header) {
      window.addEventListener('scroll', () => {
        if (window.scrollY > 20) {
          header.classList.add('scrolled');
        } else {
          header.classList.remove('scrolled');
        }
      }, { passive: true });
    }

    if (mobileToggle && navLinks) {
      mobileToggle.addEventListener('click', () => {
        const expanded = mobileToggle.getAttribute('aria-expanded') === 'true';
        mobileToggle.setAttribute('aria-expanded', !expanded ? 'true' : 'false');
        navLinks.classList.toggle('open', !expanded);
        mobileToggle.classList.toggle('open', !expanded);
        playKeySound('click');
      });

      // Close mobile menu on click link
      navLinks.querySelectorAll('a').forEach((link) => {
        link.addEventListener('click', () => {
          mobileToggle.setAttribute('aria-expanded', 'false');
          navLinks.classList.remove('open');
          mobileToggle.classList.remove('open');
        });
      });
    }
  }

  // --- Scroll to Top Button ---
  function initBackToTop() {
    const btn = document.getElementById('back-to-top');
    if (!btn) return;

    window.addEventListener('scroll', () => {
      if (window.scrollY > 400) {
        btn.classList.add('visible');
      } else {
        btn.classList.remove('visible');
      }
    }, { passive: true });

    btn.addEventListener('click', (e) => {
      e.preventDefault();
      window.scrollTo({ top: 0, behavior: 'smooth' });
      playKeySound('click');
    });
  }

  // Initialize all components once DOM is ready
  document.addEventListener('DOMContentLoaded', () => {
    initSimulator();
    initFAQ();
    initCopyButtons();
    initHeader();
    initBackToTop();
  });
})();
