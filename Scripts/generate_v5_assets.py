import asyncio
import os
import wave
import struct
import math
import numpy as np
import edge_tts
import miniaudio

OUT_DIR = r"E:\Projekte\Velocitytower\RawAssets\Audio"
os.makedirs(OUT_DIR, exist_ok=True)

SR = 44100

def save_stereo_wav(filepath: str, left: np.ndarray, right: np.ndarray):
    left = np.clip(left, -0.98, 0.98)
    right = np.clip(right, -0.98, 0.98)
    interleaved = np.empty(left.size * 2, dtype=np.int16)
    interleaved[0::2] = (left * 32767.0).astype(np.int16)
    interleaved[1::2] = (right * 32767.0).astype(np.int16)
    with wave.open(filepath, "wb") as wf:
        wf.setnchannels(2)
        wf.setsampwidth(2)
        wf.setframerate(SR)
        wf.writeframes(interleaved.tobytes())
    dur = len(left) / SR
    print(f"[OK] Saved {filepath} ({dur:.2f}s)")

def apply_quake_dsp(pcm: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    # 1. Warm saturation
    sat = np.tanh(1.9 * pcm)
    
    # 2. Sub-octave generator
    sub_phase = 0.0
    sub = np.zeros_like(sat)
    for i in range(len(sat)):
        sub[i] = math.sin(sub_phase) * abs(sat[i])
        sub_phase += 2.0 * math.pi * 65.0 / SR
    
    bass_heavy = 0.72 * sat + 0.38 * sub
    
    # 3. Stereo Arena Reverb
    delays_l = [int(0.027 * SR), int(0.049 * SR), int(0.083 * SR), int(0.125 * SR)]
    delays_r = [int(0.033 * SR), int(0.057 * SR), int(0.091 * SR), int(0.138 * SR)]
    gains = [0.32, 0.24, 0.18, 0.12]
    
    out_l = bass_heavy.copy()
    out_r = bass_heavy.copy()
    
    for d, g in zip(delays_l, gains):
        out_l[d:] += bass_heavy[:-d] * g
    for d, g in zip(delays_r, gains):
        out_r[d:] += bass_heavy[:-d] * g
        
    peak = max(np.max(np.abs(out_l)), np.max(np.abs(out_r)), 0.001)
    return (out_l / peak) * 0.95, (out_r / peak) * 0.95

async def generate_quake_voice(key: str, text: str, pitch="-14Hz", rate="-2%"):
    tmp_path = os.path.join(OUT_DIR, f"{key}_tmp.mp3")
    comm = edge_tts.Communicate(text, "en-US-ChristopherNeural", pitch=pitch, rate=rate)
    await comm.save(tmp_path)
    
    decoded = miniaudio.decode_file(tmp_path)
    os.remove(tmp_path)
    
    raw_pcm = np.array(decoded.samples, dtype=np.float32) / 32768.0
    if decoded.nchannels == 2:
        mono_pcm = (raw_pcm[0::2] + raw_pcm[1::2]) * 0.5
    else:
        mono_pcm = raw_pcm
        
    l, r = apply_quake_dsp(mono_pcm)
    out_wav = os.path.join(OUT_DIR, f"{key}.wav")
    save_stereo_wav(out_wav, l, r)

async def generate_falling_scream():
    # Character falling into abyss scream
    tmp_path = os.path.join(OUT_DIR, "scream_tmp.mp3")
    comm = edge_tts.Communicate("Waaaaaaaaaaaaaaahhhhh!", "en-US-GuyNeural", pitch="+14Hz", rate="+22%")
    await comm.save(tmp_path)
    
    decoded = miniaudio.decode_file(tmp_path)
    os.remove(tmp_path)
    
    raw_pcm = np.array(decoded.samples, dtype=np.float32) / 32768.0
    if decoded.nchannels == 2:
        mono = (raw_pcm[0::2] + raw_pcm[1::2]) * 0.5
    else:
        mono = raw_pcm
        
    N = len(mono)
    # Pitch bend down (Doppler effect) & distance fade
    t = np.linspace(0.0, 1.0, N)
    
    # Resample with dropping speed (time warping)
    speed_curve = 1.15 - 0.55 * (t ** 1.4) # drops from 1.15 down to 0.60
    indices = np.cumsum(speed_curve)
    indices = (indices / indices[-1]) * (N - 1)
    
    warped = np.interp(np.arange(N), indices, mono)
    
    # Distance attenuation: exponential volume fade
    distance_fade = np.exp(-1.8 * t)
    warped = warped * distance_fade
    
    # Add deep chasm reverberation
    rev_delay = int(0.085 * SR)
    rev_tail = np.zeros(N + rev_delay)
    rev_tail[:N] = warped
    rev_tail[rev_delay:] += warped * 0.45
    
    l = rev_tail.copy()
    r = rev_tail.copy()
    peak = max(np.max(np.abs(l)), 0.001)
    l = (l / peak) * 0.95
    r = (r / peak) * 0.95
    
    out_wav = os.path.join(OUT_DIR, "SFX_Character_Scream.wav")
    save_stereo_wav(out_wav, l, r)

def generate_dead_impact_sfx():
    # Heavy 808 cyber sub-impact + metallic glitch crunch
    duration = 1.35
    total_samples = int(duration * SR)
    l = np.zeros(total_samples, dtype=np.float32)
    r = np.zeros(total_samples, dtype=np.float32)
    
    for i in range(total_samples):
        t = i / SR
        # Pitch drops rapidly from 160Hz down to 36Hz
        freq = 36.0 + 124.0 * math.exp(-t * 9.0)
        phase = 2.0 * math.pi * freq * t
        
        # Sub boom
        sub = math.sin(phase) * math.exp(-t * 2.8)
        
        # Metal click/crunch transient at start
        if t < 0.08:
            noise = (math.sin(t * 14000.0) * 0.6 + math.sin(t * 8800.0) * 0.4) * math.exp(-t * 60.0)
        else:
            noise = 0.0
            
        sample = sub * 0.85 + noise * 0.45
        sample = math.tanh(sample * 1.6)
        
        l[i] = sample
        r[i] = sample * 0.98 + 0.02 * math.sin(phase * 1.5)
        
    peak = max(np.max(np.abs(l)), 0.001)
    l = (l / peak) * 0.96
    r = (r / peak) * 0.96
    
    out_wav = os.path.join(OUT_DIR, "SFX_Dead_Impact.wav")
    save_stereo_wav(out_wav, l, r)

async def main():
    print("Generating Quake 'New High Score!'...")
    await generate_quake_voice("VO_HighScore", "New High Score!", pitch="-14Hz", rate="-2%")
    
    print("Generating Character Fall Scream...")
    await generate_falling_scream()
    
    print("Generating DEAD Screen Impact SFX...")
    generate_dead_impact_sfx()
    print("All audio assets generated successfully!")

if __name__ == "__main__":
    asyncio.run(main())
