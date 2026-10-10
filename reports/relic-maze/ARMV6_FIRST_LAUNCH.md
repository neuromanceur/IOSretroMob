# Relic Maze — construction ARMv6 et premier lancement réel

10 octobre 2026. Branche locale `codex/relic-maze-armv6-first-launch`, base `d25814b873aa528ed223651363e5993541cf6654`. Aucun commit, push, release ou GitHub Actions pendant cette mission.

**Objectif prioritaire atteint : véritable binaire ARMv6 lancé sous iPhone OS 3.1.3 ; plateau rendu par EAGL/ES1 dans le guest ; six déplacements tactiles et une relique collectée. État : affiché et jouable sur le parcours testé. La victoire et l’interface complète restent non validées dans le guest.**

## Construction et validation du bundle

La chaîne des diagnostics RMGLES1/RMTexture/RMDepth est réutilisée : LLVM de l'installation NDK r28c existante, sans installation ni headers SDK Apple. C freestanding `armv6-none-eabi`, ARM1176JZF-S, ARM, `-ffixed-r9`, softfp/VFP, `-O1 -nostdinc -Wall -Wextra -Werror`, sans PIC/PIE/stack protector. Le linker ELF et le writer Mach-O classique existant produisent un véritable exécutable destiné au dyld historique. Le jeu C original est lié dans ce binaire.

Le contrôle indépendant du binaire final vérifie header, limites/protections des segments, symboles non lazy et entrée :

| Propriété | Résultat |
|---|---|
| Format | Mach-O 32 bits `MH_EXECUTE`, CPU ARM 12, subtype V6 6 |
| Entrée | ARM `0x2000`, `LC_UNIXTHREAD` |
| Bibliothèque chargée | `/usr/lib/libSystem.B.dylib` seulement |
| Imports | `_dlopen`, `_dlsym` ; vrais frameworks invités chargés dynamiquement |
| Instructions | r9 réservé, contrôle sur les sections exécutables des quatre objets |
| API modernes | Pas de `LC_MAIN`, chained fixups, PIE ni runtime Unity |
| Binaire | 24 643 octets, SHA-256 `e23578d8dd3b7fc1d471efbf5d57dd55b982d82fe5d2f3b644023da7913242ca` |
| Payload individuel | SHA-256 `9acf1f51114ce6d0dc47124f932d5fd6566904043475b20f6ceab2b1ed6bfc9d` |
| Payload combiné utilisé | SHA-256 `3d97162d5b1f0c0eac08734244dc171bbbc909ac06df94865c165a68e8fcc6c6` |

42 symboles requis et 12 sélecteurs sélectionnés sont présents dans les images nommées du cache dyld ARMv6 canonique 7E18. Aucune bibliothèque Apple n'a été extraite ou ajoutée au projet. La présence statique ne prouve pas l'exécution ou toutes les ABI des méthodes ; les résultats invités sont séparés ci-dessous. Le premier contrôle r9 sur l'ELF lié avait pris des chaînes de `.rodata` pour des instructions : le contrôle corrigé porte sur les objets et exclut les pools `.word`.

Bundle **plat**, identifiant `org.iosretromob.relicmaze`, minimum 3.1.3, iPhone/portrait, 27 304 octets de contenu :

```text
RelicMaze.app/
  RelicMaze       24643 octets, exécutable 0755
  Info.plist       1117 octets
  Icon.png          129 octets, icône procédurale originale
  LEVEL.txt          90 octets, dérivé du niveau C original
  LICENSE.txt      1081 octets, licence originale
  RENDERER.txt      244 octets, limites de l'adaptation
```

Les ressources sont à la racine : `Resources/` avec notre plist à la racine sélectionnait une disposition CFBundle incompatible. Deux tests indépendants utilisant CFBundle natif macOS reproduisent ce défaut et valident le bundle plat. Ils n'exécutent pas le binaire ARMv6. Le contrôle HFS après arrêt compare chaque fichier installé au bundle local, octet par octet, ainsi que permissions et propriétaire root:root. Cette installation système unsigned avec bypass de signature autorisé est propre au guest de recherche ; ce n'est pas une installation IPA/App Store générale.

## Sources conservées et adaptation

Le cœur `core/relic_maze.c` reste inchangé, SHA-256 `c7d7eaff0ead757d5a48df6ef8a8fbec338fbc354bfd43fb94c10944120843e8` : niveau 9 × 9, caisse, dalle, porte, trois reliques, victoire et logique de collision originales. Le solveur indépendant retrouve le parcours gagnant de 25 mouvements. Le frontend Objective-C `ios/main.m` utilise effectivement UIKit/CAEAGLLayer/EAGL API 1/FBO OES/ES1, mais nécessite des headers propriétaires absents ; **il n'a pas été compilé**. Il est conservé avec le web, les tests originaux, le plist source et `unity33/`, inchangés.

Le nouveau frontend SDK-free réutilise le pont Objective-C des diagnostics et appelle les vrais frameworks invités. Pour respecter les enveloppes MBX texturées déjà mesurées, il affiche l'intérieur 7 × 7 sur un quad de six sommets, depuis une texture RGBA opaque 8 × 8 recalculée à chaque input à partir du véritable `RMGame`. Les couleurs viennent du renderer original ; la bordure solide est implicite dans la marge et les collisions. Les ornements de cellules et le tap direct sur une case sont omis. UIKit fournit le HUD, Reset et les quatre boutons directionnels. Le jeu désactive son idle timer via l'API UIKit, sans timer d'animation. Aucun depth test, blend ou dither, aucun audio. Les textures procédurales sont compilées dans les sources originales du frontend, sans asset commercial ni image fixe à présenter comme exécution.

Ajouts : `ios/armv6_main.c`, `ios/board_texture.c`, `ios/board_texture.h`, `ios/build_armv6.py`, `tests/test_board_texture.c`, `tests/test_bundle_layout.py`, sous `examples/RelicMaze/`, et ce dossier de rapport. Adaptations préexistantes : `examples/RelicMaze/CMakeLists.txt`, `README.md`, `ios/README.md`. Aucun code moteur modifié, aucun traitement graphique au nom du jeu, aucune modification de Unity 3.3. Les crédits S5LBox par j0shua-SYSON et les licences MIT restent intacts.

## Exécution invitée et preuves

Binaire moteur préservé `build/core/bootkernel`, SHA-256 `384ce2d001f1c4e12ab70f860eb5d79cc4cb5999b5dfc47123a3c877084a9678`. Kernel, DeviceTree et rootfs canoniques de 7E18 sont vérifiés par SHA-256 avant chaque campagne, puis utilisés en lecture seule. Aucun ancien état RAM n'est mélangé avec une nouvelle installation. Chaque cold boot utilise un disque frais isolé ; un seul est actif à la fois, **aucun snapshot n'est créé**.

| Essai | Résultat et diagnostic |
|---|---|
| Préfixe XNU | 1 million d'instructions, sans disque ni payload, code 0 ; ne prouve pas le jeu |
| `cold-01` | 7,65 milliards, code 0, 712,09 s ; installation byte-identique et inscription MobileInstallation, mais icône absente et journal absent : disposition `Resources/` corrigée |
| `cold-02` | CLI refusé, code 1 en 0,004 s : bouton `home` inconnu, Home est `menu` ; aucun disque ni instruction invitée |
| `cold-03` | 8,1 milliards, code 0, 752,37 s ; icône visible avec bundle plat ; premier candidat incorrect, puis auto-verrouillage avant le candidat correct ; aucun journal du jeu |
| `cold-04` | 7,45 milliards, code 0, 689,13 s ; tap correct mais durée 24 millions : captures du mode réorganisation des icônes, aucun journal du jeu |
| `cold-05` | 7,45 milliards, code 0, 700.83 s ; processus ARMv6, rendu et six mouvements confirmés par journal et captures |

La durée du bouton matériel Home ne doit pas être appliquée au tap d'icône. Le dernier essai reprend le tap court d'un million d'instructions utilisé dans les lancements historiques validés. Position `(116,60)` issue de la vraie capture SpringBoard stabilisée ; lancement à 6 900 100 000, puis six taps Down `(160,435)` à 7 190/7 230/7 270/7 310/7 350/7 390 millions, durée 1 million. Déverrouillage : drag à 4 milliards, dismissal à 6,01 milliards, changement de page à 6,5001 milliards. Tous passent par le périphérique Z2, sans appel hôte à `rm_step`. **Acceptation du périphérique ne signifie pas consommation par UIKit** : journal et pixels sont requis.

`GUEST_EXECUTION.log` est extrait du fichier HFS `/private/var/tmp/relic-maze.log` après arrêt du guest. Il confirme :

- `RM_MAIN_ENTER`, PID invité **39** (`0x27`), delegate enregistré, `UIApplicationMain` et `RELIC_UIKIT_LAUNCHED`.
- Contexte EAGL courant, OpenGL ES-CM 1.1 (48), PowerVR MBXLite with VGPLite ; renderbuffer **320 × 460**, FBO `0x8cd5` complet.
- Sept renders : présentations réussies, erreurs texture/draw/present à zéro, idle timer désactivé.
- Six callbacks UIKit Down (`direction=2`), six `rm_step` réussis : `(1,1)` → `(1,7)`, compteur `0` → `6`, masque de reliques `0` → `2` (**une relique**). Caisse `(3,5)`, dalle inactive et victoire `0`.

Les cinq captures du plateau correspondent aux états après **1, 2, 3, 4 et 6 mouvements** : 441 points intérieurs indépendants par capture, zéro différence de canal RGB. Les états de ce comparateur sont recoupés avec les sept états du journal, pas déduits de `present()` seul. La première capture disponible est après le premier déplacement ; l’état initial est attesté par le journal, sans capture indépendante de cet état.

**Portée exacte : fenêtre CLCD 0 brute 320 × 460, position d’affichage `(0,20)`**, format 7, stride 1280. La fenêtre UIKit 2 reste active en 320 × 480 mais n’est pas exportée par le sampler existant. Les images ne montrent donc pas les compteurs/boutons superposés ; elles ne constituent pas une composition complète de l’écran 320 × 480. Aucune superposition reconstruite ou image retouchée n’est présentée. Le premier comparateur avait supposé 480 lignes et a refusé la capture 460 lignes ; son origine verticale a été corrigée d’après le registre CLCD, sans modifier les pixels.

| Capture conservée | Instructions | État | SHA-256 RGB |
|---|---:|---|---|
| [FIRST_GAME_FRAME.png](FIRST_GAME_FRAME.png), PPM original associé | 7 200 000 000 | joueur `(1,2)`, 1 mouvement, 0 relique | `f3d1565268f5773746822a49345188aafc62a3ae757e184d772c7829f088f116` |
| [AFTER_SIX_MOVES.png](AFTER_SIX_MOVES.png), PPM original associé | 7 400 000 000 | joueur `(1,7)`, 6 mouvements, 1 relique | `a71b92953d1bf9e7d3dc677a0ca1309ab390d0800fc1da622e3110194b9e35ab` |

Les hashes des PNG/PPM, dimensions, origine, états et identité du binaire sont dans `FIRST_LAUNCH_EVIDENCE.json`. Les deux PNG sont redécodés et comparés octet par octet aux RGB des PPM. `ARMV6_BUNDLE_VALIDATION.json` contient le manifeste du bundle sans chemin personnel. Le reste des prélèvements et le comparateur indépendant sont privés.

Les captures viennent du scanout CLCD P6 actif (320 × 480 avant lancement, 320 × 460 pour la fenêtre GL), intervalle 50 millions d'instructions pour le dernier essai. Conversion PNG sans changement des octets RGB. Les anciens screenshots et les images synthétiques ne servent pas de preuves de cette campagne. Les captures SpringBoard/lockscreen des essais échoués restent privées.

## Commandes et journaux

Depuis la racine du dépôt :

```sh
cmake -S examples/RelicMaze -B .local/relic-maze-2026-10-10/build-host -DCMAKE_BUILD_TYPE=Release
cmake --build .local/relic-maze-2026-10-10/build-host -j2
ctest --test-dir .local/relic-maze-2026-10-10/build-host --output-on-failure
python3 examples/RelicMaze/tests/verify_level.py
export IOSRETROMOB_LLVM_BIN=/path/to/existing/ndk/llvm/bin
python3 examples/RelicMaze/ios/build_armv6.py .local/relic-maze-armv6
python3 -m unittest discover -s tests/guest-apps -p 'test_*.py'
python3 examples/RelicMaze/tests/test_bundle_layout.py -v
ctest --test-dir build -R '^(arm_interp|vfp|power_vr_mbx2d|power_vr_mbx_vgp|multitouch_z2|macho_parser|hfs_catalog_provisioning|jailbreak_payload_tar|external_md_cli_preflight|boot_option_resolution|app_touch_map|app_touch_queue)$' -V
```

L'installation LLVM doit contenir `clang`, `ld.lld` et `llvm-objdump`. Le builder refuse de remplacer un répertoire de sortie existant. Le moteur n'a pas été recompilé pendant cette mission.

Commande du dernier guest, chemins firmware explicites fournis localement :

```sh
build/core/bootkernel "$FIRMWARE/kernel.macho" -d "$FIRMWARE/devicetree.bin" \
  -c 'debug=0x8 serial=1 nand-enable-adm=0' \
  --external-md "$FIRMWARE/rootfs.img" "$NEW_WORK/rootfs.img" \
  -R 128 -n 7450000000 -Z 100000000 \
  --framebuffer --iomfb-display --multitouch --activate \
  --mbx --no-ca-software-render --fast --jb-codesign --jb-payload \
  --jailbreak-payload "$PAYLOAD" \
  --drag 4000000000:57:431:305:431:24:165000000 \
  --touch 6010000000:160:334:24000000 \
  --drag 6500100000:275:220:40:220:24:165000000 \
  --touch 6900100000:116:60:1000000 \
  --touch 7190000000:160:435:1000000 --touch 7230000000:160:435:1000000 \
  --touch 7270000000:160:435:1000000 --touch 7310000000:160:435:1000000 \
  --touch 7350000000:160:435:1000000 --touch 7390000000:160:435:1000000
```

Lancer depuis un nouveau dossier de campagne contenant `firmware/` pour les sorties du CLI. Définir `S5LBOX_CLCD_CAPTURE_DIR` sur un dossier privé vide et `S5LBOX_CLCD_CAPTURE_INTERVAL=50000000`. Ne pas rejouer avec une destination disque existante. Les préflights, commandes absolues exactes, environnement, durée, erreurs, stdout/stderr, UART, inventaires HFS, replays de contrôle et scripts de reproduction sont dans `.local/relic-maze-2026-10-10/` ignoré. Construction finale : `armv6-04/`. Résultat final : `cold-05-result.json`, contrôles : `validation-final-04.json`, `guest-api-validation.json`, `installed-bundle-05.json`, `final-bundle-summary.json`. Les logs des refus sont conservés.

## Tests et limites

| Validation locale | Résultat |
|---|---|
| CTest Relic Maze Release, `-j2` | 2/2 : logique originale + atlas opaque/transitions jusqu'à victoire |
| Solveur indépendant | Carte C/web/Unity identique et solution 25 mouvements ; Unity seulement lu |
| CTest moteur ciblé | 12/12, aucun sous-cas sauté annoncé : ARM/VFP/MBX/VGP/Z2/Mach-O/HFS/payload/tactile/CLI |
| Outils invités Python | 8/8, LLVM effectivement exécuté, aucune classe sautée |
| Régression CFBundle Python | 2/2 sur macOS, rejouée avec le frontend final, aucune classe sautée |
| Audit public fichiers suivis + tous objets Git | PASS, 386 fichiers, 444 objets/399 blobs, zéro anomalie |

Pas de suite moteur complète nouvellement exécutée ou revendiquée. Les tests hôtes ne prouvent ni le frontend Objective-C original, ni Unity 3.3, ni un jeu commercial. Les nouvelles sources et ce rapport restent non suivis ; l'audit public porte sur l'historique existant. Les bundles, exécutables, caches et disques ne sont pas ajoutés à Git ; aucun firmware/SDK/IPA/clé ajouté au projet.

## Stockage et intégrité

Mesures avant campagne : `df -h` **72 Gio libres**, `df -k` **75 348 120 Kio** ; `S5LBox/work` 4,401,139,574 octets logiques et 4,356,079,616 octets de somme de blocs, inchangés. Chaque essai valide SHA-256/ancêtres sans symlinks, budget et réserve ≥5 Gio avant production. Allocation additionnelle maximale annoncée : **550 000 000 octets par cold boot** ; budget cumulatif final des médias/preuves privées **2 250 000 000 octets**, sous le plafond work de 10 milliards. Il s’agit d’une campagne de lancement locale, pas du budget par défaut de 1 Gio d’une nouvelle publication.

Après arrêt : `df -h` **70 Gio libres**, `df -k` **73 449 424 Kio** lors de la première mesure finale ; les sorties finales exactes sont dans `storage-after.json` et `storage-final-command.log`.

| Ensemble | Taille logique | Somme de blocs alloués |
|---|---:|---:|
| Nouveau `work/` | 1,870,055,158 octets | 1,742,049,280 octets |
| Logs, builds et captures privés de la mission | 100,822,745 octets | 104,423,424 octets |
| `work/` historique inchangé | 4,401,139,574 octets | 4,356,079,616 octets |

`du -sh` : nouveau work **1,6 Gio**, preuves privées environ **100 Mio**. Les deux PPM, PNG et petits rapports ajoutent moins de 1 Mio au dossier de rapport. La consommation logique nouvelle est d’environ **1,97 Go** hors rapport. La somme des blocs APFS n’est pas une occupation exclusive ; la variation `df` du Mac n’est pas entièrement imputable à cette mission. Aucune économie CoW/sparse/compression revendiquée.

Quatre disques de 466 825 216 octets sont conservés dans `cold-01`, `cold-03`, `cold-04`, `cold-05`, pas de disque pour le refus `cold-02`. Le CLI crée obligatoirement une destination neuve à chaque cold boot. Ces reprises étaient nécessaires après le défaut de bundle puis les corrections de geste ; elles ne constituent pas une politique de snapshot par expérience. **Zéro snapshot créé**, un seul disque actif, aucune suppression de données préexistantes ou de médias VM. Les disques des essais terminés ne sont pas des checkpoints indispensables et devront faire l’objet d’un nettoyage explicitement autorisé avant une autre campagne ; les petites preuves et la construction reproductible suffisent à conserver ce jalon.

L’inventaire préalable de 15 263 fichiers du dépôt historique est comparé après la campagne par SHA-256, longueur, mode et cible des liens. **Neuf composants des trois références : inchangés**, avec leurs `.mdimage`/`.mdstate` exacts. Kernel/DeviceTree/rootfs, sources, patches, rapports, traces et captures historiques : identiques. Les fichiers préexistants de Relic Maze, dont son ancien build, sont comparés : seules les trois adaptations documentaires/CMake prévues changent parmi les sources. Core, `main.m`, plist source, tests originaux et Unity sont inchangés.

L’inventaire strict a néanmoins détecté des différences de métadonnées : deux références internes `.git/refs/codex/turn-diffs/` modifiées et deux ajoutées dans le dépôt historique, ainsi qu’un `.DS_Store` du jeu modifié. Le contrôle brut a donc échoué avec code 1 ; ces écarts sont conservés dans `integrity-after.json`, puis explicitement classés dans `integrity-assessment.json`. Aucun changement des données historiques ou de leurs références VM ; aucune restauration/suppression de ces métadonnées. Ne pas présenter l’ensemble du dépôt historique comme byte-identique jusque dans les métadonnées internes de Codex. Aucun fichier historique n’est écrit par les commandes de développement/campagne de cette mission.

Le dépôt moteur conserve HEAD `d25814b...`, les quatre commits et zéro modification suivie. Les sources du jeu étaient déjà non suivies ; elles le restent avec ce dossier de rapport. `.local/`, `work/` et le build imbriqué du jeu restent ignorés.

## Premier point restant

Aucun blocage ne reste pour le jalon « premier plateau réel et progression tactile ». Le prochain manque de preuve est la capture complète des fenêtres CLCD composées, comprenant le HUD et les boutons UIKit. Les directions Up/Left/Right, Reset, la poussée de caisse, l’ouverture de porte et les 25 mouvements jusqu’à victoire sont couverts par les tests hôtes mais **pas encore par cette exécution invitée**. La variante Objective-C originale, Unity 3.3, l’audio, la performance et l’interaction humaine en temps réel restent hors validation. Aucun développement supplémentaire n’est entrepris après ce jalon.
