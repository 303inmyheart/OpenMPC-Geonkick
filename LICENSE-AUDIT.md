# License review — 2026-11-13

Reviewed upstream quamplex/geonkick commit
462fed9bd8c48579a8819e7b360f8a2999b246fc (3.9.0), including nested
licenses and README files inside all nine factory preset archives.

## Code

Root LICENSE contains GPLv3. Representative source headers in the application,
DSP, Redkite GUI and Quamplex DSP Tools expressly permit version 3 **or any later
version**. The MPC adaptation uses GPL-3.0-or-later; original notices remain.
The OpenMPC wrapper is MIT and stays external; its copyright/license is included
in binary packages. DejaVu font notices remain in art/FONT-LICENSE.txt.

Forking, modifying, redistributing and charging for GPL builds are permitted
under the license terms. Publishing an MPC binary requires the corresponding
modified source, headers, required vendored components and build/install scripts
under the applicable GPL terms. A link to unmodified upstream is insufficient.
For public GitHub releases, pin/tag the exact source used for each binary and
link it beside the installer; preserve attribution, mark changes and dates,
include GPL text and the necessary third-party notices. Do not impose additional
restrictions on recipients' GPL rights. Source availability must be real before
a public binary release; the proposed repository URL is not yet published.

References: https://www.gnu.org/licenses/gpl-3.0.html (sections 4–6),
https://www.gnu.org/licenses/gpl-faq.en.html#DoesTheGPLAllowMoney .

## Images

Upstream data/README and src/resources/README dedicate their images to CC0 1.0.
Our MPC interface and browser tile are newly drawn; no upstream GUI artwork or
Akai artwork is currently bundled. A source fork should retain these notices.

## Bundled factory sound packs (included since0.0.29)

| Archive | Declared license | Attribution in archive README |
|---|---|---|
| GeonkickBells | CC0 1.0 (spelled C0, full CC0 name supplied) | Geonkick |
| SPKickPresetsVol1 | CC0 1.0 | Oleh Bardiuk |
| HaraldReveryKicks | CC0 1.0 (spelled C0, full CC0 name supplied) | Harald Revery |
| DSoS | CC-BY 4.0 | Dark Side of Synth |
| ToniLinkTechnoKicks | CC-BY 4.0 | Toni Link |
| TimeKit | CC-BY 4.0 | Iurie |
| UnfaTutorial | CC-BY 4.0 | Unfa |
| DigitalSamba | CC-BY-SA 4.0 | Bart Brouns |
| AVL-Drumkit | CC-BY-SA 3.0 | Iurie; based on AVL Drumkits samples |

Retain the README, author/source and license link for each pack actually shipped.
BY requires attribution and identification of modifications; BY-SA also requires
adapted pack content to retain its share-alike terms (or a permitted compatible
license). Do not relabel all presets/samples GPL merely because the code is GPL.
For AVL samples, preserve and review the referenced sample-source terms as well
before bundling. The eight MPC starting sounds are separate parameter recipes. Since0.0.29
the original factory packs and embedded samples are also included, under the
individual terms and conversion notices described below.

CC license references: https://creativecommons.org/licenses/by/4.0/ ,
https://creativecommons.org/licenses/by-sa/4.0/ ,
https://creativecommons.org/licenses/by-sa/3.0/ .

This records the licenses supplied in the inspected source; it does not establish
ownership of every contributor's third-party sample or grant trademark rights.


## Factory-bank conversion0.0.29 (2026-11-21)

All nine original archives are now supplied in factory/original with immutable
SHA256 provenance.102 individual presets and7 kits are converted to native MPC
states. Individual sound content keeps the licences in the table above; the
software adapter remains GPL-3.0-or-later. Every installed package includes all
original pack READMEs, attribution, source and licence URLs, plus an explicit
conversion/change notice. BY-SA sound conversions retain their respective terms.

AVL sample-source notice additionally verified at the author's official PDF:
https://bandshed.net/pdf/AVL-Drumkits%20CC-BY-SA%20License.pdf
Credit Glen MacArthur,2015, alongside Iurie's Geonkick adaptation. Retain the
CC BY-SA3.0 sample-library terms and the author's musical-production exception.
The adapted kit is called MPC AVL-derived Kit to distinguish the conversion.
See factory/NOTICE.txt for the distributed attribution and change information.
CC licence deeds reviewed: https://creativecommons.org/licenses/by/4.0/ ,
https://creativecommons.org/licenses/by-sa/4.0/ ,
https://creativecommons.org/licenses/by-sa/3.0/ .
