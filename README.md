# Note: This firmware is currently compatible with v1 hardware only. Support for other hardware versions is planned for the future.

This firmware is based on the LOSEHU132E firmware (see: https://github.com/losehu/uv-k5-firmware-custom), with several changes made to differentiate it from the original 132E build.
Please refer to the releases for the changelog.

Firmware flasher used for flashing radios: https://www.universirius.com/SirioArchive/Materiel_pr_site/Firmware-IJV/K5prog_IJV_V3.zip
Thanks IJV! If you use his firmware, please support him.


CHIRP support is included; the CHIRP module for this repository can be found in the CHIRP Module folder.

Build artifacts: run `make full FIRMWARE_VERSION=01` to create `firmware.bin` and the UVTools-compatible `TrueMDC.Gen01.packed.bin`. The version is embedded in the packed firmware metadata and included in the filename. Upload that packed file to a public GitHub release or repository, then use its raw URL with UVTools, for example: `https://egzumer.github.io/uvtools/?firmwareURL=https://raw.githubusercontent.com/USER/REPOSITORY/main/TrueMDC.Gen01.packed.bin`. The default version is `Gen00`.

Custom features:
- Custom-length MDC preamble (menus 26 and 27)
- 6 different Roger beeps + MDC modes (menu 28)
- UI tones: power-on beep and talk-permit tone (F + Down)
- Screen inversion (F + Menu)
- SmartSquelch: pre-arms a long tail for fluttering signals, then switches to the shortest tail if stable reception is detected
- Call End Tone (C.End): optional FM call-end tone after the dual-watch inactivity delay, with red and green LEDs combined as a yellow indicator while pending, and also Call screen UI.
- Talk Permit Tone (TPT): selectable XTS, TRBO, HYT and TETRA talk-permit tones after pre-ID signaling.
- C.End and TPT settings are saved in EEPROM; F + Down UI-tone mute overrides playback without changing their saved settings
- Power calibration menu added for easier power calibration.

I would like to thank everyone who trusts my work and those who have tested my builds. Special thanks to Sara Sinn for early testing, 9W2BIL for extensive beta testing of new features, 9W3MIG, 9W3KKW, and 9W3JJJ for supporting this firmware, and 9W2DSL and 9W2ESR for their ideas and feedback. Special thanks to BI7CZK on his professional advice on Hytera Radio audio UI. Thanks also to everyone else who has used my firmware, and to those who have helped keep the spirit of this project lively and forward-moving despite a few unnecessary detours and the occasional dramatic performance along the way.

I hope this work makes your K5 a little better. Enjoy the firmware!

73 DE 9M2RTX
