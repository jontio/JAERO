# JMSK
An MSK demodulator written in C++ Qt

This program implements a coherent MSK demodulator type as seen at http://jontio.zapto.org/hda1/msk-demodulation2.html.

![](https://github.com/jontio/JMSK/blob/gh-pages/jmsk-screenshot-win.png)

The demodulator uses the technique that treats the signal similar to OQPSK but with sine wave transitions rather than rectangular transitions. The BER versus EbNo performance in the presence of AWGN is the same as coherently demodulated differentially encoded BPSK. While designed for MSK it will also demodulate GMSK. The signal is supplied via the audio input of the computer’s soundcard.
The software implements differential decoding hence the modulator must use differential encoding. The demodulator can decode varicode encoded text. The output of the demodulator can be directed to either a built-in console or to a UDP network port.

The JMSK directory is where the Qt pro file is for the main application. The udptextserver directory is a small demo application for receiving data sent from JMSK.

Jonti 2015
http://jontio.zapto.org

