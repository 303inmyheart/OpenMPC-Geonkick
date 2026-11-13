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

## Bundled factory sound packs (not used in the current MPC build)

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
before bundling. Current eight MPC starting sounds are parameter configurations
written in src/engine.c; no archived factory samples or presets are copied.

CC license references: https://creativecommons.org/licenses/by/4.0/ ,
https://creativecommons.org/licenses/by-sa/4.0/ ,
https://creativecommons.org/licenses/by-sa/3.0/ .

This records the licenses supplied in the inspected source; it does not establish
ownership of every contributor's third-party sample or grant trademark rights.
