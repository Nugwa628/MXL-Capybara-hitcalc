# MXL Capybara Calculator

Shows your chance to hit every Median XL monster, with the attack rating you'd need for 95%.

Download **`MXL Capybara Calculator.exe`** (Windows x64). It's a single file and needs no install, no game files and no other files.


## How to use
- **Attack Rating**, **Character Level**, **Difficulty**: your numbers.
- **Monster level (area)**: optional. Non-boss monsters use this level, and bosses keep their own. If you leave it empty, each monster uses its table level.
- **Search** / **Bosses only** filter the list. Click a column header to sort.

## Formula (D2Game 1.13c; Median XL only clamps both levels to at least 1)
```
def = monlvl.AC[difficulty][monster level] * monstats.AC% / 100
hit = 100 * AR / (AR + def)
hit = hit * 2 * clvl / (clvl + mlvl)
clamp 5 .. 95
```
The monster data comes from Median XL's `monstats.bin` and `monlvl.bin` (`source/mondata.h`).

## Source
Freestanding Win32 C with no CRT and no Windows SDK. See `source/BUILD.txt` for build steps.
- `source/hitcalc.c`: the calculator
- `source/splash.h`: the loading screen
- `source/splash_data.h`: the generated, embedded frames and title (`source/tools/build_data.py`)

Diablo II art and fonts © Blizzard Entertainment. Median XL by the Median XL team. This is a fan-made tool.
