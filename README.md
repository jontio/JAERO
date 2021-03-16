# JAERO

[![build](https://github.com/jontio/JAERO/actions/workflows/ci-windows-build.yml/badge.svg)](https://github.com/jontio/JAERO/actions/workflows/ci-windows-build.yml)

A SatCom ACARS demodulator and decoder for the Aero standard written in C++ Qt

![](images/screenshot-win-main.png)

This program demodulates and decodes ACARS messages sent from satellites to Aeroplanes (SatCom ACARS) commonly used when Aeroplanes are beyond VHF range. Demodulation is performed using the soundcard.
Such signals are typically around 1.5Ghz and can be received with a simple low gain antenna that can be home brewed in a few hours in conjunction with a cheap [RTL-SDR] dongle.

The 600 and 1200 bps SatCom ACARS signals are basically MSK like so the demodulator was forked from [JMSK]. The demodulator implements a coherent MSK demodulator type as seen at http://jontio.zapto.org/hda1/msk-demodulation2.html.

The 600 and 1200 bps demodulator uses the technique that treats the signal similar to [OQPSK] but with sine wave transitions rather than rectangular transitions. The BER (*Bit Error Rate*) versus EbNo (*Energy per bit to Noise power density*) performance in the presence of AWGN (*Additive White Gaussian Noise*) is the same as coherently demodulated differentially encoded [BPSK]. While designed for MSK it will also demodulate [GMSK] and some types of [BPSK]. The signal is supplied via the audio input of the computer’s soundcard.
The software implements differential decoding hence the modulator must use differential encoding. The output of the demodulator can be directed to either a built-in console or to a [UDP] network port.

An OQPSK demodulator was added and supports the faster 8400bps and 10.5k Aero signals.

Both 1200 and 10.5k burst C-band signal demodulation (From plane to ground station) are supported.

C-Channel signals at 8400bps that are for voice can be demodulated.

![](images/screenshot-win-planelog.png)

## Binaries

Precompiled binaries can be downloaded from [Releases].

## Directory structure

The [JAERO](JAERO) directory is where the Qt pro file is for the main application. The [udptextserver](udptextserver) directory is a small demo application for receiving data sent from JAERO.

## Compiling JAERO

Compiling JAERO requires the Qt framework. I would suggest using [MSYS2] to install the Qt framework and the rest of the development environment. Qt Creator can be used to compile JAERO and comes with the Qt framework. At least version 5 of the Qt framework is required. I only use MinGW and GCC for building so don't know if MSVC works.

I've now added a Windows and Linux build scripts. The Windows build script requires MSYS2, once installed run msys64 clone the repo then type `./ci-windows-build.sh`. The Linux build script is for Debian based distros and will need to be changed for others, I have only tested it on some of the Ubuntu family (Ubuntu, Kubuntu and Lubuntu). For Linux type `./ci-linux-build.sh`, it will install JAERO as a few packages so uninstalling should be clean.

I've added a continuous integration script [.github/workflows/ci-windows-build.yml](.github/workflows/ci-windows-build.yml) that will get Gihub to build JAERO for both Windows and Ubuntu. Currently the script is triggered manually in the [actions](actions) tab.

JAERO's dependencies will undoubtedly change over time, so I suspect eventually the build scripts will fail and will need fixing. Till then they work, but I have no idea how long they will work for; we shall see.

## Thanks

I'd like to thank everyone who has given their kind support for JAERO over the years. Thanks for Otti for getting the project started, John and Bev for setting up a worldwide large dish network, everyone who has donated, the people who have send feedback, people who use JAERO, Jeroen who done an excellent job programming new code for JAERO to bring some features that I’m sure will be appreciated by many, Tomasz for adding more ACARS message support, Corrosive for adding documentation, and the many other people who have written the libraries that JAERO uses.

Jonti 2021
<br>
http://jontio.zapto.org

[OQPSK]: https://en.wikipedia.org/wiki/Phase-shift_keying#Offset_QPSK_.28OQPSK.29
[GMSK]: https://en.wikipedia.org/wiki/Minimum-shift_keying#Gaussian_minimum-shift_keying
[BPSK]: https://en.wikipedia.org/wiki/Phase-shift_keying#Binary_phase-shift_keying_.28BPSK.29
[UDP]: https://en.wikipedia.org/wiki/User_Datagram_Protocol
[SSB]: https://en.wikipedia.org/wiki/Single-sideband_modulation
[Arduino]: https://www.arduino.cc/
[Varicode]: https://en.wikipedia.org/wiki/Varicode
[Spectrum Lab]: http://www.qsl.net/dl4yhf/spectra1.html
[JMSK]: https://github.com/jontio/JMSK
[RTL-SDR]: http://www.rtl-sdr.com/about-rtl-sdr/
[Releases]: https://github.com/jontio/JAERO/releases
[JAERO Wiki]: https://github.com/jontio/JAERO/wiki
[MSYS2]: https://www.msys2.org/
