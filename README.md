# Note: This firmware is currently compatible with v1 hardware only. Support for other hardware versions is planned for the future.

This firmware is based on the LOSEHU132E firmware (see: https://github.com/losehu/uv-k5-firmware-custom), with several changes made to differentiate it from the original 132E build.
Please refer to the releases for the changelog.

Firmware flasher used for flashing radios: https://www.universirius.com/SirioArchive/Materiel_pr_site/Firmware-IJV/K5prog_IJV_V3.zip
Thanks IJV! If you use his firmware, please support him.


CHIRP support is included; the CHIRP module for this repository can be found in the CHIRP Module folder.


Custom features and ongoing enhancements:
- FleetSync II support with separate Fleet and Unit IDs, including validation and clamping to safe limits.
- FleetSync audio/Roger behavior aligned to the active family selection so the decoder and transmit path follow the same mode.
- MDC1200 support with user-configurable ID and signaling timing options.
- Talk Permit Tone (TPT) selection, including XTS, TRBO, HYT and TETRA variants with Auto behavior and Roger-family aware operation.
- Roger mode selection for Off, Pre, Post and Both for both MDC and FleetSync signaling.
- Call End Tone (C.End) and UI tones with EEPROM persistence; F + Down can mute UI tones without changing saved settings.
- Power calibration entries and menu-level adjustments for easier field tuning.
- CHIRP module support for import/export of the updated settings and limits.

FleetSync notes:
- FleetSync uses a separate Fleet ID and Unit ID.
- Valid Fleet ID range in the current implementation: 100 to 349.
- Valid Unit ID range in the current implementation: 100 to 4999.
- Values outside these limits are clamped to the nearest valid bound so the radio stays within the supported protocol range.
- The FleetSync ID is handled independently from the MDC ID, and the active Roger mode determines whether the FleetSync path is used for pre/post signaling.

Menu count note:
- The menu total is currently kept at 54 active entries in the current build after the removed menu item, matching the active menu list in this firmware revision.

I would like to thank everyone who trusts my work and those who have tested my builds. Special thanks to Sara Sinn for early testing, 9W2BIL for extensive beta testing of new features, 9W3MIG, 9W3KKW, and 9W3JJJ for supporting this firmware, and 9W2DSL and 9W2ESR for their ideas and feedback. Special thanks to BI7CZK on his professional advice on Hytera Radio audio UI. Thanks also to everyone else who has used my firmware, and to those who have helped keep the spirit of this project lively and forward-moving!

I hope this work makes your K5 a little better. Enjoy the firmware!

73 DE 9M2RTX
