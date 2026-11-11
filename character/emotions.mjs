// eye openness, brow angle (deg), mouth curvature, mouth openness, tempo (Hz), tone (Hz)
const profiles = {
  happy:       [1.0, -8,  0.8, 0.20, 1.2, 620],
  sad:         [0.7, 17, -0.8, 0.10, 0.4, 280],
  angry:       [0.7,-27, -0.7, 0.20, 1.1, 170],
  sleepy:      [0.2,  2,  0.0, 0.25, 0.3, 210],
  surprised:   [1.2, 18,  0.0, 0.86, 1.5, 780],
  excited:     [1.1,-12,  0.9, 0.65, 3.0, 740],
  curious:     [1.0, 12,  0.3, 0.12, 0.8, 540],
  confused:    [0.8,-16, -0.3, 0.16, 1.0, 360],
  shy:         [0.6, 10,  0.4, 0.08, 0.5, 490],
  laughing:    [0.2, -6,  1.0, 0.80, 4.0, 660],
  crying:      [0.3, 22, -1.0, 0.60, 2.5, 250],
  thinking:    [0.8, -4,  0.0, 0.06, 0.4, 310],
  listening:   [1.1,  5,  0.1, 0.05, 0.7, 520],
  speaking:    [0.9,  3,  0.3, 0.24, 2.0, 570],
  dancing:     [0.9,-12,  0.8, 0.32, 2.8, 690],
  blinking:    [0.04, 0,  0.1, 0.08, 1.0, 390],
  idle:        [0.9,  0,  0.2, 0.08, 0.6, 420],
  charging:    [0.7, -4,  0.5, 0.08, 0.5, 610],
  low_battery: [0.4, 10, -0.5, 0.12, 0.3, 190],
  hungry:      [0.8,  8, -0.2, 0.42, 1.4, 300],
  playful:     [1.0,-15,  0.7, 0.42, 2.2, 720],
  proud:       [0.7, -6,  0.9, 0.10, 0.7, 580],
  worried:     [0.9, 25, -0.6, 0.20, 1.0, 330],
  love:        [0.8,  4,  1.0, 0.16, 1.3, 640],
  dizzy:       [0.7,  0,  0.0, 0.45, 3.4, 260],
  bored:       [0.4, -2, -0.1, 0.06, 0.2, 220],
  focused:     [0.6,-19,  0.0, 0.05, 0.3, 440],
  greeting:    [1.0,  8,  0.8, 0.42, 1.8, 630],
  goodbye:     [0.7,  7,  0.4, 0.22, 1.1, 410],
  celebration: [1.1, -9,  1.0, 0.72, 3.6, 820],
};

export const emotions = Object.freeze(Object.keys(profiles));

const clamp = (value, min, max) => Math.max(min, Math.min(value, max));
const blend = (from, to, progress) => from + (to - from) * progress;
const cycle = (ms, hz) => Math.sin(ms * hz * Math.PI / 500);

export class Character {
  #emotion = 'idle';
  #start = 0;
  #from = profiles.idle;

  #profile(now) {
    const progress = clamp((now - this.#start) / 220, 0, 1);
    const eased = progress * progress * (3 - 2 * progress);
    return profiles[this.#emotion].map((value, index) => blend(this.#from[index], value, eased));
  }

  select(emotion, now) {
    if (!Object.hasOwn(profiles, emotion)) throw new RangeError('Unknown emotion');
    this.#from = this.#profile(now);
    this.#start = now;
    this.#emotion = emotion;
    return { frequency: profiles[emotion][5], durationMs: 150 };
  }

  touch(now) {
    const reaction = { sleepy: 'greeting', sad: 'love', playful: 'laughing', listening: 'speaking' };
    return this.select(reaction[this.#emotion] ?? 'playful', now);
  }

  frame(now, { gazeX = 0, gazeY = 0, audioLevel = 0 } = {}) {
    const [eyes, brows, smile, mouth, tempo] = this.#profile(now);
    const blinkPhase = (now + 180) % 3700;
    const blink = 1 - 0.94 * clamp(1 - Math.abs(blinkPhase - 80) / 80, 0, 1);
    const wave = cycle(now, tempo);
    const lipSync = this.#emotion === 'speaking' ? clamp(audioLevel, 0, 1) * 0.70 : 0;
    return {
      emotion: this.#emotion,
      eyes: eyes * blink,
      brows,
      smile,
      mouth: clamp(mouth + wave * mouth * 0.22 + lipSync, 0, 1),
      gazeX: clamp(gazeX + cycle(now, tempo * 0.31) * 0.12, -1, 1),
      gazeY: clamp(gazeY + wave * 0.05, -1, 1),
    };
  }
}
