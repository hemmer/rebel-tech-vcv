[![workflow](https://github.com/hemmer/rebel-tech-vcv/actions/workflows/build-plugin.yml/badge.svg)](https://github.com/hemmer/rebel-tech-vcv/actions/workflows/build-plugin.yml)
[![License](https://img.shields.io/pypi/l/fpvgcc.svg)](https://opensource.org/licenses/GPL-3.0)

This repo contains VCV ports of Rebel Technology Eurorack modules. See: https://www.rebeltech.org/ and https://github.com/pingdynasty/.

License: GPL-3.0-or-later 

## Screenshots

<img src="./img/plugin.png" style="max-width: 100%;">


## Differences with hardware

We have tried to make the VCV implementations as authentic as possible, however there are some minor changes that have been made (either for usuability or to bring modules in line with VCV rack conventions and standards).

* CV inputs accept range -10V to +10V, and sum with the knob. E.g. if the knob is at zero, +10V corresonds to the equivalent state where the knob is 100%.

* CLK has the option to output gates (50% duty) or triggers, and allows x1 or x16 output varients - see context menu.

* Bidirectional jacks aren't supported, so where clock multis are present on hardware, one is designated as Clock Thru instead.

* Mode switches can also act as a reset on hardware; this is not really possible with the way VCV components are implemented so is not implemented.

## Source repos for hardware versions

* [Stoicheia/Klasmata](https://github.com/pingdynasty/EuclideanSequencer)
* [Phoreo](https://github.com/pingdynasty/ClockMultiplier)
* [Logoi](https://github.com/pingdynasty/ClockDelay)
* [CLK](https://github.com/pingdynasty/CLK)
* [Tonic](https://github.com/pingdynasty/Tonic)

