# ChangeLog

## Version 1.1.0 (2019-01-18):
* Incompatible API change: so far `LA_VERSION` was a preprocessor macro,
  which was expanded during compilation of any program using it. As a result,
  its value contained the version of the C header, while the intention was
  to show the version of the currently running library. `LA_VERSION` has
  therefore been changed to a `char * const * const` variable. As a result,
  it is no longer possible to refer to it in preprocessor constructs
  (like `#ifdef`s or gluing of static strings together).
* Added decoder for Media Advisory messages (ACARS label SA) and an example
  program `media_advisory`. Contributed by Fabrice Crohas.
* Fixed decoding of ADS-C messages containing multiple contract requests.
* A few small fixes.

## Version 1.0.0 (2018-12-10):
* First stable release.
