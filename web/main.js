/**
 * C-Deck — Interactive landing page simulator.
 * Demonstrates the same six actions available in the native Windows app.
 */
(function () {
  'use strict';

  const state = { volume: 72, muted: false, micMuted: false, statusTimer: null };
  const $ = (id) => document.getElementById(id);
  const volumeValue = $('demo-volume-value');
  const volumeTrack = $('demo-volume-track');
  const volumeSlider = $('demo-volume-slider');
  const status = $('demo-status-text');
  const muteButton = $('key-mute');
  const micButton = $('key-mic');
  const soundButton = $('demo-sound-toggle');
  const resetButton = $('demo-reset');
  let soundEnabled = true;
  let audioContext = null;

  function playTone(type) {
    if (!soundEnabled) return;
    try {
      const Audio = window.AudioContext || window.webkitAudioContext;
      if (!Audio) return;
      if (!audioContext) audioContext = new Audio();
      if (audioContext.state === 'suspended') audioContext.resume().catch(() => {});
      const now = audioContext.currentTime;
      const oscillator = audioContext.createOscillator();
      const gain = audioContext.createGain();
      oscillator.type = type === 'mute' ? 'triangle' : 'sine';
      oscillator.frequency.setValueAtTime(type === 'volume' ? 560 : 360, now);
      gain.gain.setValueAtTime(0.035, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + 0.06);
      oscillator.connect(gain);
      gain.connect(audioContext.destination);
      oscillator.start(now);
      oscillator.stop(now + 0.06);
    } catch (_) { /* Audio is optional; controls remain functional. */ }
  }

  function idleMessage() {
    const flags = [];
    if (state.muted) flags.push('SAÍDA MUTADA');
    if (state.micMuted) flags.push('MICROFONE MUTADO');
    return flags.length ? 'PRONTO · ' + flags.join(' · ') + ' · DEMONSTRAÇÃO' : 'PRONTO · MODO DEMONSTRAÇÃO';
  }

  function setStatus(message) {
    if (!status) return;
    status.textContent = message;
    status.classList.remove('flash');
    void status.offsetWidth;
    status.classList.add('flash');
    if (state.statusTimer) clearTimeout(state.statusTimer);
    state.statusTimer = setTimeout(() => { status.textContent = idleMessage(); }, 2600);
  }

  function updateVolume() {
    if (volumeValue) {
      volumeValue.textContent = state.muted ? 'MUTADO · ' + state.volume + '%' : state.volume + '%';
    }
    if (volumeTrack) {
      volumeTrack.style.width = state.volume + '%';
      volumeTrack.classList.toggle('is-muted', state.muted);
    }
    if (muteButton) {
      muteButton.classList.toggle('key-active-mute', state.muted);
      muteButton.setAttribute('aria-pressed', String(state.muted));
      muteButton.setAttribute('aria-label', state.muted ? 'Ativar áudio (simulação)' : 'Silenciar áudio (simulação)');
    }
    if (volumeSlider) {
      if (document.activeElement !== volumeSlider) volumeSlider.value = String(state.volume);
      volumeSlider.setAttribute('aria-valuetext', state.volume + '%' + (state.muted ? ', sem áudio' : ''));
    }
  }

  function updateMic() {
    if (!micButton) return;
    micButton.classList.toggle('key-active-mute', state.micMuted);
    micButton.setAttribute('aria-pressed', String(state.micMuted));
    micButton.setAttribute('aria-label', state.micMuted ? 'Ativar microfone (simulação)' : 'Silenciar microfone (simulação)');
    const label = micButton.querySelector('.key-label');
    if (label) label.textContent = state.micMuted ? 'MIC ATIVO OFF' : 'MIC MUTE';
  }

  function adjustVolume(delta) {
    playTone('volume');
    state.muted = false;
    state.volume = Math.max(0, Math.min(100, state.volume + delta));
    updateVolume();
    setStatus('Simulação: volume ajustado para ' + state.volume + '%');
  }

  function toggleOutputMute() {
    playTone('mute');
    state.muted = !state.muted;
    updateVolume();
    setStatus(state.muted ? 'Simulação: áudio de saída silenciado' : 'Simulação: áudio de saída restaurado');
  }

  function toggleMicMute() {
    playTone('mute');
    state.micMuted = !state.micMuted;
    updateMic();
    setStatus(state.micMuted ? 'Simulação: microfone silenciado' : 'Simulação: microfone ativado');
  }

  function showToast(message, type) {
    let toast = $('demo-toast');
    if (!toast) {
      toast = document.createElement('div');
      toast.id = 'demo-toast';
      toast.className = 'demo-toast';
      const windowElement = document.querySelector('.app-window');
      if (windowElement) windowElement.appendChild(toast);
    }
    toast.textContent = message;
    toast.setAttribute('data-type', type || 'info');
    toast.classList.add('visible');
    clearTimeout(showToast.timer);
    showToast.timer = setTimeout(() => toast.classList.remove('visible'), 2200);
  }

  function resetDemo() {
    state.volume = 72;
    state.muted = false;
    state.micMuted = false;
    updateVolume();
    updateMic();
    setStatus('Demonstração reiniciada · valores iniciais restaurados');
  }

  function initGridNavigation() {
    const ids = ['key-vol-down', 'key-mute', 'key-vol-up', 'key-capture', 'key-calc', 'key-mic'];
    const buttons = ids.map($);
    if (buttons.some((button) => !button)) return;
    buttons.forEach((button, index) => {
      button.tabIndex = index === 0 ? 0 : -1;
      button.addEventListener('focus', () => {
        buttons.forEach((item, i) => { item.tabIndex = i === index ? 0 : -1; });
      });
      button.addEventListener('keydown', (event) => {
        const row = Math.floor(index / 3);
        const col = index % 3;
        let next = index;
        if (event.key === 'ArrowLeft' && col > 0) next--;
        else if (event.key === 'ArrowRight' && col < 2) next++;
        else if (event.key === 'ArrowUp' && row > 0) next -= 3;
        else if (event.key === 'ArrowDown' && row < 1) next += 3;
        else if (event.key === 'Home') next = event.ctrlKey || event.metaKey ? 0 : row * 3;
        else if (event.key === 'End') next = event.ctrlKey || event.metaKey ? 5 : row * 3 + 2;
        else return;
        event.preventDefault();
        buttons[next].focus();
      });
    });
  }

  function init() {
    const bind = (id, handler) => { const element = $(id); if (element) element.addEventListener('click', handler); };
    bind('key-vol-down', () => adjustVolume(-5));
    bind('key-vol-up', () => adjustVolume(5));
    bind('key-mute', toggleOutputMute);
    bind('key-mic', toggleMicMute);
    bind('key-capture', () => {
      playTone('launch');
      setStatus('Simulação: captura de tela · atalho Win + Shift + S');
      showToast('Demonstração local · Captura de tela', 'app');
    });
    bind('key-calc', () => {
      playTone('launch');
      setStatus('Simulação: abrir Calculadora do Windows');
      showToast('Demonstração local · Calculadora', 'app');
    });
    bind('demo-reset', resetDemo);
    bind('demo-sound-toggle', () => {
      soundEnabled = !soundEnabled;
      soundButton.textContent = soundEnabled ? 'SOM: ON' : 'SOM: OFF';
      soundButton.setAttribute('aria-label', soundEnabled ? 'Desativar efeitos sonoros' : 'Ativar efeitos sonoros');
      soundButton.setAttribute('aria-pressed', String(soundEnabled));
      soundButton.classList.toggle('sound-muted', !soundEnabled);
      if (soundEnabled) playTone('click');
    });
    if (volumeSlider) {
      volumeSlider.addEventListener('input', (event) => {
        state.volume = Number.parseInt(event.target.value, 10) || 0;
        state.muted = false;
        updateVolume();
        setStatus('Simulação: volume ' + state.volume + '%');
      });
      volumeSlider.addEventListener('change', () => playTone('volume'));
    }
    initGridNavigation();
    updateVolume();
    updateMic();
    document.querySelectorAll('.faq-item').forEach((item) => {
      const trigger = item.querySelector('.faq-question');
      const answer = item.querySelector('.faq-answer');
      if (!trigger || !answer) return;
      trigger.addEventListener('click', () => {
        const opening = !item.classList.contains('active');
        document.querySelectorAll('.faq-item').forEach((other) => {
          other.classList.remove('active');
          const button = other.querySelector('.faq-question');
          const panel = other.querySelector('.faq-answer');
          if (button) button.setAttribute('aria-expanded', 'false');
          if (panel) panel.hidden = true;
        });
        if (opening) {
          item.classList.add('active');
          trigger.setAttribute('aria-expanded', 'true');
          answer.hidden = false;
        }
        playTone('click');
      });
    });
  }

  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', init);
  else init();
})();
