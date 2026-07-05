# Note: This firmware is currently compatible with v1 hardware only. Support for other hardware versions is planned for the future.

This firmware is based on the LOSEHU132E firmware (see: https://github.com/losehu/uv-k5-firmware-custom), with several changes made to differentiate it from the original 132E build.
Please refer to the releases for the changelog.

Firmware flasher used for flashing radios: https://www.universirius.com/SirioArchive/Materiel_pr_site/Firmware-IJV/K5prog_IJV_V3.zip
Thanks IJV! If you use his firmware, please support him.

This firmware now uses an in-house microphone AGC implementation with a feedback suppressor.

CHIRP support is included; the CHIRP module for this repository can be found in the CHIRP Module folder.

Custom features:
- Custom-length MDC preamble (menus 26 and 27)
- 6 different Roger beeps + MDC modes (menu 28)
- UI tones: power-on beep and talk-permit tone (F + Down)
- Screen inversion (F + Menu)
- Mic AGC with Feedback suppressor

I would like to thank everyone who trusts my work and those who have tested my builds. Special thanks to Sara Sinn for early testing, 9W2BIL for extensive beta testing of new features, 9W3MIG, 9W3KKW, and 9W3JJJ for supporting this firmware, and 9W2DSL and 9W2ESR for their ideas and feedback. Thanks also to everyone else who has used my firmware, and to those who have helped keep the spirit of this project lively and forward-moving despite a few unnecessary detours and the occasional dramatic performance along the way.

In these few months of developing this firmware, I want to share this:
Amateur radio is all about sharing, curiosity, and respect for each other. When that spirit turns into hostility, legal threats, and unnecessary drama, the community suffers, and newcomers get pushed away. I’d rather keep this project focused on learning, teamwork, and improving the hobby for everyone.

I hope this work makes your K5 a little better. Enjoy the firmware!

73 DE 9M2RTX
