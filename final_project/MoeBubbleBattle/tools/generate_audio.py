"""Generate original music and sound effects for Moe Bubble Battle.

Only the Python standard library is used so every submitted audio file can be
recreated.  The melodies are original and intentionally simple, loop-friendly
chiptune/watercolor-game cues rather than adaptations of existing game music.
"""

from __future__ import annotations

import argparse
import math
import random
import struct
import wave
from array import array
from pathlib import Path


SAMPLE_RATE = 44_100
TAU = math.tau


def midi_frequency(note: float) -> float:
    return 440.0 * (2.0 ** ((note - 69.0) / 12.0))


def create_buffer(duration: float) -> array:
    return array("f", [0.0]) * int(duration * SAMPLE_RATE)


def envelope(time_value: float, duration: float, attack: float, release: float) -> float:
    if time_value < attack:
        return time_value / max(attack, 1e-6)
    if time_value > duration - release:
        return max(0.0, (duration - time_value) / max(release, 1e-6))
    return 1.0


def oscillator(phase: float, kind: str) -> float:
    sine = math.sin(phase)
    if kind == "sine":
        return sine
    if kind == "triangle":
        return 2.0 / math.pi * math.asin(sine)
    if kind == "soft_square":
        return 0.72 * math.tanh(2.2 * sine) + 0.28 * sine
    raise ValueError(f"unknown oscillator: {kind}")


def add_tone(
    target: array,
    start: float,
    duration: float,
    frequency: float,
    volume: float,
    kind: str = "sine",
    attack: float = 0.01,
    release: float = 0.08,
    end_frequency: float | None = None,
    vibrato: float = 0.0,
) -> None:
    first = max(0, int(start * SAMPLE_RATE))
    count = min(int(duration * SAMPLE_RATE), len(target) - first)
    if count <= 0:
        return

    phase = 0.0
    final_frequency = frequency if end_frequency is None else end_frequency
    for offset in range(count):
        t = offset / SAMPLE_RATE
        ratio = t / max(duration, 1e-6)
        current_frequency = frequency + (final_frequency - frequency) * ratio
        if vibrato:
            current_frequency *= 1.0 + 0.008 * math.sin(TAU * vibrato * t)
        phase += TAU * current_frequency / SAMPLE_RATE
        env = envelope(t, duration, attack, release)
        target[first + offset] += oscillator(phase, kind) * volume * env


def add_note(
    target: array,
    start: float,
    duration: float,
    midi_note: float,
    volume: float,
    kind: str = "triangle",
    **kwargs: float,
) -> None:
    add_tone(target, start, duration, midi_frequency(midi_note), volume, kind, **kwargs)


def add_noise(
    target: array,
    start: float,
    duration: float,
    volume: float,
    seed: int,
    decay: float = 5.0,
) -> None:
    random_source = random.Random(seed)
    first = max(0, int(start * SAMPLE_RATE))
    count = min(int(duration * SAMPLE_RATE), len(target) - first)
    previous = 0.0
    for offset in range(count):
        t = offset / SAMPLE_RATE
        white = random_source.uniform(-1.0, 1.0)
        previous = previous * 0.72 + white * 0.28
        target[first + offset] += previous * volume * math.exp(-decay * t)


def write_wave(path: Path, samples: array) -> None:
    peak = max(max((abs(value) for value in samples), default=0.0), 1e-6)
    gain = min(0.90 / peak, 1.0)
    path.parent.mkdir(parents=True, exist_ok=True)

    with wave.open(str(path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(SAMPLE_RATE)
        block = bytearray()
        for value in samples:
            clipped = max(-1.0, min(1.0, value * gain))
            block.extend(struct.pack("<h", int(clipped * 32767.0)))
            if len(block) >= 131_072:
                output.writeframesraw(block)
                block.clear()
        if block:
            output.writeframesraw(block)


def compose_menu_music() -> array:
    tempo = 118.0
    beat = 60.0 / tempo
    bars = 12
    target = create_buffer(bars * 4.0 * beat)
    chords = [
        (60, 64, 67),  # C
        (57, 60, 64),  # Am
        (53, 57, 60),  # F
        (55, 59, 62),  # G
    ]
    melody_patterns = [
        (0, 1, 2, 1, 0, 2, 1, 2),
        (2, 1, 0, 1, 2, 1, 0, 1),
        (0, 2, 1, 2, 0, 1, 2, 1),
    ]

    for bar in range(bars):
        chord = chords[bar % len(chords)]
        bar_start = bar * 4.0 * beat
        pattern = melody_patterns[bar % len(melody_patterns)]

        for step, chord_index in enumerate(pattern):
            note_start = bar_start + step * 0.5 * beat
            add_note(target, note_start, 0.42 * beat, chord[chord_index] + 12,
                     0.12, "triangle", attack=0.012, release=0.09, vibrato=5.0)

        for pulse in range(4):
            add_note(target, bar_start + pulse * beat, 0.72 * beat, chord[0] - 12,
                     0.11, "sine", attack=0.02, release=0.15)

        for step in range(8):
            add_note(target, bar_start + step * 0.5 * beat, 0.34 * beat,
                     chord[step % 3], 0.045, "soft_square", attack=0.015, release=0.08)

        if bar % 2 == 1:
            add_tone(target, bar_start + 3.35 * beat, 0.28 * beat, 780.0, 0.055,
                     "sine", attack=0.005, release=0.08, end_frequency=1120.0)

    return target


def compose_game_music() -> array:
    tempo = 148.0
    beat = 60.0 / tempo
    bars = 16
    target = create_buffer(bars * 4.0 * beat)
    chords = [
        (60, 64, 67),
        (55, 59, 62),
        (57, 60, 64),
        (53, 57, 60),
    ]
    melody = (0, 1, 2, 1, 2, 1, 0, 2)

    for bar in range(bars):
        chord = chords[bar % 4]
        bar_start = bar * 4.0 * beat

        for step in range(8):
            note = chord[melody[(step + bar) % len(melody)]] + 12
            if bar % 4 == 3 and step >= 6:
                note += 12
            add_note(target, bar_start + step * 0.5 * beat, 0.34 * beat,
                     note, 0.105, "soft_square", attack=0.008, release=0.07)

            arpeggio_note = chord[step % 3]
            add_note(target, bar_start + step * 0.5 * beat, 0.30 * beat,
                     arpeggio_note, 0.052, "triangle", attack=0.005, release=0.05)

        for pulse in range(4):
            bass_note = chord[0] - 12 if pulse != 3 else chord[2] - 12
            add_note(target, bar_start + pulse * beat, 0.62 * beat,
                     bass_note, 0.14, "sine", attack=0.008, release=0.12)

            add_tone(target, bar_start + pulse * beat, 0.10, 92.0, 0.13,
                     "sine", attack=0.002, release=0.08, end_frequency=54.0)

        for snare in (1, 3):
            add_noise(target, bar_start + snare * beat, 0.11, 0.09,
                      seed=8000 + bar * 10 + snare, decay=22.0)

        if bar % 2 == 0:
            add_tone(target, bar_start + 3.65 * beat, 0.22 * beat, 920.0, 0.045,
                     "sine", attack=0.002, release=0.06, end_frequency=1380.0)

    return target


def sound_menu_move() -> array:
    target = create_buffer(0.13)
    add_tone(target, 0.0, 0.12, 620.0, 0.28, "sine", release=0.08,
             end_frequency=760.0)
    return target


def sound_menu_confirm() -> array:
    target = create_buffer(0.32)
    for index, note in enumerate((72, 76, 79)):
        add_note(target, index * 0.045, 0.24, note, 0.16, "triangle", release=0.12)
    return target


def sound_bubble_place() -> array:
    target = create_buffer(0.38)
    add_tone(target, 0.0, 0.32, 280.0, 0.25, "sine", attack=0.003,
             release=0.12, end_frequency=640.0)
    add_tone(target, 0.07, 0.22, 760.0, 0.10, "sine", release=0.10,
             end_frequency=980.0)
    return target


def sound_bubble_explode() -> array:
    target = create_buffer(0.62)
    add_tone(target, 0.0, 0.52, 160.0, 0.33, "sine", attack=0.002,
             release=0.25, end_frequency=58.0)
    add_noise(target, 0.0, 0.48, 0.30, seed=2718, decay=6.0)
    add_tone(target, 0.08, 0.30, 720.0, 0.11, "triangle", release=0.20,
             end_frequency=260.0)
    return target


def sound_power_up() -> array:
    target = create_buffer(0.78)
    for index, note in enumerate((72, 76, 79, 84)):
        add_note(target, index * 0.12, 0.30, note, 0.20, "triangle",
                 attack=0.004, release=0.16, vibrato=6.0)
    return target


def sound_hit() -> array:
    target = create_buffer(0.42)
    add_tone(target, 0.0, 0.38, 430.0, 0.28, "soft_square", attack=0.003,
             release=0.18, end_frequency=115.0)
    add_noise(target, 0.02, 0.20, 0.12, seed=1618, decay=13.0)
    return target


def sound_level_clear() -> array:
    target = create_buffer(2.25)
    notes = (60, 64, 67, 72, 76, 79, 84)
    for index, note in enumerate(notes):
        start = index * 0.19
        add_note(target, start, 0.48, note, 0.18, "triangle",
                 attack=0.008, release=0.24, vibrato=5.0)
    for note in (72, 76, 79):
        add_note(target, 1.45, 0.72, note, 0.13, "sine",
                 attack=0.02, release=0.34)
    return target


def inspect_wave(path: Path) -> tuple[float, int, int]:
    with wave.open(str(path), "rb") as source:
        duration = source.getnframes() / source.getframerate()
        return duration, source.getframerate(), source.getnchannels()


def generate(output_directory: Path) -> None:
    assets = {
        "bgm_menu.wav": compose_menu_music(),
        "bgm_game.wav": compose_game_music(),
        "sfx_menu_move.wav": sound_menu_move(),
        "sfx_menu_confirm.wav": sound_menu_confirm(),
        "sfx_bubble_place.wav": sound_bubble_place(),
        "sfx_bubble_explode.wav": sound_bubble_explode(),
        "sfx_power_up.wav": sound_power_up(),
        "sfx_hit.wav": sound_hit(),
        "sfx_level_clear.wav": sound_level_clear(),
    }
    for filename, samples in assets.items():
        write_wave(output_directory / filename, samples)

    for filename in assets:
        duration, sample_rate, channels = inspect_wave(output_directory / filename)
        print(f"{filename:26s} {duration:6.2f}s  {sample_rate}Hz  {channels}ch")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    generate(arguments.output)


if __name__ == "__main__":
    main()
