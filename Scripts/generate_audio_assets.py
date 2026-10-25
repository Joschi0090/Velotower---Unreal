import os
import wave
import struct
import math
import random

OUT_DIR = r"E:\Projekte\Velocitytower\RawAssets\Audio"
os.makedirs(OUT_DIR, exist_ok=True)

SAMPLE_RATE = 44100

def write_wav(filename, samples_left, samples_right=None):
    if samples_right is None:
        samples_right = samples_left
    filepath = os.path.join(OUT_DIR, filename)
    with wave.open(filepath, 'w') as wf:
        wf.setnchannels(2)
        wf.setsampwidth(2)
        wf.setframerate(SAMPLE_RATE)
        frames = bytearray()
        for l, r in zip(samples_left, samples_right):
            cl = max(-32767, min(32767, int(l * 32767.0)))
            cr = max(-32767, min(32767, int(r * 32767.0)))
            frames.extend(struct.pack('<hh', cl, cr))
        wf.writeframes(frames)
    print(f"Saved {filepath} ({len(samples_left)/SAMPLE_RATE:.2f}s)")

def note_freq(midi_note):
    return 440.0 * (2.0 ** ((midi_note - 69.0) / 12.0))

def generate_music_loop():
    # 124 BPM, 8 bars (32 beats, 128 16th-notes) driving Cyberpunk Synthwave / Electro loop
    bpm = 124.0
    beat_sec = 60.0 / bpm
    step_sec = beat_sec / 4.0
    total_steps = 128
    total_samples = int(total_steps * step_sec * SAMPLE_RATE)
    left = [0.0] * total_samples
    right = [0.0] * total_samples

    # Bass progression (A minor -> F major -> C major -> G major -> D minor -> E7)
    bass_roots = [45, 45, 41, 41, 48, 48, 43, 40] # 8 bars
    arp_offsets = [0, 3, 7, 12, 7, 3, 12, 15]
    lead_melody = [
        69, 0, 72, 76, 74, 0, 72, 69,  71, 0, 72, 74, 76, 0, 79, 76,
        65, 0, 69, 72, 76, 0, 74, 72,  67, 0, 71, 74, 79, 0, 76, 74,
        72, 0, 76, 79, 81, 0, 79, 76,  74, 0, 76, 79, 84, 0, 81, 79,
        67, 0, 71, 74, 79, 0, 83, 79,  76, 0, 74, 71, 69, 72, 76, 81
    ]

    rng = random.Random(42)
    phase_bass = 0.0
    phase_arp = 0.0
    phase_lead = 0.0

    for i in range(total_samples):
        t = i / SAMPLE_RATE
        step_idx = min(total_steps - 1, int(t / step_sec))
        step_frac = (t - step_idx * step_sec) / step_sec
        bar_idx = (step_idx // 16) % len(bass_roots)

        # 1. Punchy Four-on-the-floor Kick (steps 0, 4, 8, 12)
        kick = 0.0
        if step_idx % 4 == 0:
            kt = step_frac * step_sec
            if kt < 0.18:
                k_freq = 150.0 * math.exp(-kt * 28.0) + 46.0
                k_env = (1.0 - kt / 0.18) ** 1.6
                kick = math.sin(2.0 * math.pi * k_freq * kt) * k_env * 0.55

        # 2. Snare / Clap on steps 4, 12
        snare = 0.0
        if step_idx % 8 == 4:
            st = step_frac * step_sec
            if st < 0.16:
                s_env = (1.0 - st / 0.16) ** 2.0
                noise = (rng.random() * 2.0 - 1.0)
                body = math.sin(2.0 * math.pi * 190.0 * st) * math.exp(-st * 25.0)
                snare = (noise * 0.7 + body * 0.4) * s_env * 0.35

        # 3. Crisp Hi-Hat on off-beats
        hat = 0.0
        if step_idx % 2 == 1:
            ht = step_frac * step_sec
            if ht < 0.07:
                h_env = (1.0 - ht / 0.07) ** 2.5
                hat = (rng.random() * 2.0 - 1.0) * h_env * 0.14

        # 4. Rolling Synthwave Octave Saw Bass (sidechained to kick)
        root_note = bass_roots[bar_idx] + (12 if (step_idx % 2 == 1) else 0)
        b_freq = note_freq(root_note - 12)
        phase_bass = (phase_bass + b_freq / SAMPLE_RATE) % 1.0
        saw = 2.0 * phase_bass - 1.0
        sub = math.sin(2.0 * math.pi * phase_bass)
        sidechain = 0.35 + 0.65 * min(1.0, (step_idx % 4 + step_frac) / 1.5)
        b_env = math.exp(-step_frac * 3.2)
        bass = (saw * 0.55 + sub * 0.65) * b_env * sidechain * 0.32

        # 5. Stereo Shimmer Arpeggio
        arp_note = bass_roots[bar_idx] + 12 + arp_offsets[step_idx % 8]
        a_freq = note_freq(arp_note)
        phase_arp = (phase_arp + a_freq / SAMPLE_RATE) % 1.0
        pulse = 1.0 if phase_arp < 0.35 else -1.0
        a_env = math.exp(-step_frac * 5.0)
        arp_l = pulse * a_env * 0.14 * (0.7 + 0.3 * math.sin(t * 3.0))
        arp_r = pulse * a_env * 0.14 * (0.7 + 0.3 * math.cos(t * 3.0))

        # 6. Cyberpunk Lead Synth
        m_note = lead_melody[step_idx % len(lead_melody)]
        lead_l = 0.0
        lead_r = 0.0
        if m_note > 0:
            l_freq = note_freq(m_note)
            phase_lead = (phase_lead + l_freq / SAMPLE_RATE) % 1.0
            l_env = math.exp(-step_frac * 2.2)
            l_wave = (2.0 * phase_lead - 1.0) * 0.6 + math.sin(2.0 * math.pi * phase_lead * 2.0) * 0.4
            lead_l = l_wave * l_env * 0.18
            lead_r = l_wave * l_env * 0.16

        left[i] = max(-0.98, min(0.98, kick + snare + hat + bass + arp_l + lead_l))
        right[i] = max(-0.98, min(0.98, kick + snare + hat * 0.85 + bass + arp_r + lead_r))

    write_wav("MUS_CyberTower_Loop.wav", left, right)

def generate_sfx_jump():
    dur = 0.22
    n = int(dur * SAMPLE_RATE)
    s = []
    phase = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        u = t / dur
        freq = 240.0 + 580.0 * (u ** 1.35)
        phase += freq / SAMPLE_RATE
        env = (1.0 - u) ** 1.3
        val = (math.sin(2.0 * math.pi * phase) * 0.75 + (1.0 if (phase % 1.0) < 0.5 else -1.0) * 0.25) * env * 0.65
        s.append(val)
    write_wav("SFX_Jump.wav", s)

def generate_sfx_super_launch():
    dur = 0.55
    n = int(dur * SAMPLE_RATE)
    l, r = [], []
    p1, p2 = 0.0, 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        u = t / dur
        f1 = 220.0 * (2.0 ** (u * 2.4))
        f2 = f1 * 1.5
        p1 += f1 / SAMPLE_RATE
        p2 += f2 / SAMPLE_RATE
        env = (1.0 - u) ** 0.9
        w1 = math.sin(2.0 * math.pi * p1) * 0.6 + (2.0 * (p1 % 1.0) - 1.0) * 0.4
        w2 = math.sin(2.0 * math.pi * p2) * 0.5
        l.append((w1 + w2 * 0.7) * env * 0.6)
        r.append((w1 * 0.8 + w2) * env * 0.6)
    write_wav("SFX_SuperLaunch.wav", l, r)

def generate_sfx_wall_ricochet():
    dur = 0.28
    n = int(dur * SAMPLE_RATE)
    s = []
    p = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        u = t / dur
        # Metallic cyber ricochet zing
        freq = 980.0 - 420.0 * u + 180.0 * math.sin(t * 95.0)
        p += freq / SAMPLE_RATE
        env = (1.0 - u) ** 1.6
        s.append(math.sin(2.0 * math.pi * p) * env * 0.65)
    write_wav("SFX_WallRicochet.wav", s)

def generate_sfx_core_pickup():
    # Crisp 2-note coin/core chime (B5 -> E6)
    dur = 0.34
    n = int(dur * SAMPLE_RATE)
    l, r = [], []
    p1, p2 = 0.0, 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 987.77 if t < 0.085 else 1318.51
        p1 += freq / SAMPLE_RATE
        p2 += (freq * 2.0) / SAMPLE_RATE
        local_t = t if t < 0.085 else (t - 0.085)
        env = math.exp(-local_t * 11.0)
        val = (math.sin(2.0 * math.pi * p1) * 0.75 + math.sin(2.0 * math.pi * p2) * 0.25) * env * 0.65
        l.append(val)
        r.append(val * 0.95)
    write_wav("SFX_CorePickup.wav", l, r)

def generate_sfx_combo_up():
    # Juicy 3-note rising triad chime (C6 -> E6 -> G6 -> C7)
    notes = [1046.50, 1318.51, 1567.98, 2093.00]
    dur = 0.42
    n = int(dur * SAMPLE_RATE)
    l, r = [], []
    p = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        idx = min(3, int(t / 0.085))
        freq = notes[idx]
        p += freq / SAMPLE_RATE
        local_t = t - idx * 0.085
        env = math.exp(-local_t * 10.0) * (1.0 - t / dur * 0.4)
        val = (math.sin(2.0 * math.pi * p) * 0.7 + (2.0 * (p % 1.0) - 1.0) * 0.3) * env * 0.65
        l.append(val)
        r.append(val)
    write_wav("SFX_ComboUp.wav", l, r)

def generate_sfx_level_up_fanfare():
    # Triumphant AAA Level Up fanfare chord arpeggio (A4 - C#5 - E5 - A5 - C#6 - E6)
    notes = [523.25, 659.25, 783.99, 1046.50, 1318.51, 1567.98, 2093.00]
    dur = 1.15
    n = int(dur * SAMPLE_RATE)
    l, r = [0.0] * n, [0.0] * n
    for idx, freq in enumerate(notes):
        start_t = idx * 0.09
        p = 0.0
        for i in range(int(start_t * SAMPLE_RATE), n):
            t = i / SAMPLE_RATE - start_t
            p += freq / SAMPLE_RATE
            env = math.exp(-t * 3.8)
            wave_v = (math.sin(2.0 * math.pi * p) * 0.65 + (2.0 * (p % 1.0) - 1.0) * 0.35) * env * 0.25
            l[i] = max(-0.98, min(0.98, l[i] + wave_v))
            r[i] = max(-0.98, min(0.98, r[i] + wave_v * 0.95))
    write_wav("SFX_LevelUp_Fanfare.wav", l, r)

def generate_sfx_ice_shatter():
    dur = 0.36
    n = int(dur * SAMPLE_RATE)
    rng = random.Random(99)
    s = []
    p = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        u = t / dur
        noise = rng.random() * 2.0 - 1.0
        freq = 1800.0 + 900.0 * math.sin(t * 210.0)
        p += freq / SAMPLE_RATE
        env = math.exp(-t * 11.0)
        s.append((noise * 0.55 + math.sin(2.0 * math.pi * p) * 0.45) * env * 0.6)
    write_wav("SFX_IceShatter.wav", s)

def generate_sfx_button_click():
    dur = 0.09
    n = int(dur * SAMPLE_RATE)
    s = []
    p = 0.0
    for i in range(n):
        t = i / SAMPLE_RATE
        freq = 1480.0 if t < 0.035 else 1980.0
        p += freq / SAMPLE_RATE
        env = math.exp(-t * 38.0)
        s.append(math.sin(2.0 * math.pi * p) * env * 0.55)
    write_wav("SFX_ButtonClick.wav", s)

if __name__ == "__main__":
    generate_music_loop()
    generate_sfx_jump()
    generate_sfx_super_launch()
    generate_sfx_wall_ricochet()
    generate_sfx_core_pickup()
    generate_sfx_combo_up()
    generate_sfx_level_up_fanfare()
    generate_sfx_ice_shatter()
    generate_sfx_button_click()
    print("All audio files generated successfully!")
