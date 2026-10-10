# Intégration locale de Relic Maze et du laboratoire CLCD

10 octobre 2026. **Sources sélectionnées, compilation Release, 81 entrées CTest et audit pré-commit : PASS.** Intégration locale sur `codex/relic-maze-armv6-first-launch`, base publiée `d25814b873aa528ed223651363e5993541cf6654`. Les quatre commits historiques restent intacts. Aucun nouveau boot iPhone OS, média VM, snapshot, push, release, PR ou CI. Le checkout S5LBox historique n’est pas utilisé comme destination d’écriture.

Relic Maze est un **jeu natif ARMv6 réellement terminé dans iPhone OS 3.1.3**, sans Unity : PID 39, 25 callbacks UIKit et déplacements, 26 présentations ES1 réussies, trois reliques et `won=1`. Ce résultat provient du journal invité et des captures historiques. La fluidité temps réel, la composition complète fidèle et un runtime Android ne sont pas démontrés.

## Commits et état de publication

| Groupe local | Identité |
|---|---|
| Jeu C original, frontend ARMv6, navigateur, prototype Unity source et tests | `75c2ff4fa2920eb2546b51b739827057a0b94d99` — Integrate original Relic Maze ARMv6 game and firmware-free host tests |
| 32 touches et laboratoire natif CLCD générique | `0590f4f36cceedab49c4ef2c841ffd76f9a3767f` — Add bounded touch campaigns and native CLCD capture laboratory |
| Documentation, preuves sélectionnées et audit renforcé | Commit contenant ce rapport : `Document guest victory and curate verifiable CLCD integration evidence` |

L’identité du troisième commit se résout à partir du rapport, sans y incorporer une empreinte circulaire :

```sh
git log -1 --format=%H -- reports/relic-maze/INTEGRATION_AUDIT.md
git log --reverse --format='%H %s' d25814b873aa528ed223651363e5993541cf6654..HEAD
```

Le relevé de clôture local `.local/relic-maze-integration-2026-10-10/final-closure.json` contient les trois identités complètes, les mesures finales et la vérification du clone du HEAD. Aucun tag ou nouvel objet historique S5LBox n’est importé. La vérification distante est une lecture `git ls-remote` : main était à la base publiée avant les commits ; la même empreinte est vérifiée après. Aucun push n’est autorisé par cette consolidation.

## Sélection et conservation

Inventaire initial : **386 fichiers suivis, 89 fichiers non suivis, trois fichiers suivis modifiés**, HEAD à la base publiée. Les SHA-256, tailles et modes sont conservés dans le baseline privé. L’ensemble intégré contient **475 chemins sélectionnés positivement** ; aucune indexation globale `git add .`.

Le jeu inclut le moteur C original, les tests, l’atlas procédural, `armv6_main.c`, le builder Mach-O, les ressources originales, le solveur indépendant et la version navigateur. `ios/main.m` est conservé comme frontend Objective-C classique **non compilé**. `unity33/` conserve le script original sans modification : aucun éditeur 3.3, player iPhone ou export Unity n’a été validé. La logique C, le frontend invité exécuté et les sources Unity ne sont pas réécrits par cette mission.

Les changements génériques comprennent les limites CLI 8/32/33, le sampler multi-fenêtres opt-in, la conservation des canaux alpha, les registres et formats bornés, le décodeur/vérificateur, les fixtures indépendantes et le prototype de composition explicitement expérimental. Aucune branche par nom de jeu dans le matériel émulé ; `core/src/soc/clcd.c`, le sampler RGB historique et le GPU restent inchangés.

### Preuves retenues

- Les quatre rapports datés : [ARMv6](ARMV6_FIRST_LAUNCH.md), [partie complète](FULL_GUEST_PLAYTHROUGH.md), [couches natives](CLCD_MULTILAYER_CAPTURE.md), [composition expérimentale](CLCD_COMPOSITION.md). Leurs textes restent identiques aux versions historiques ; leurs phrases « non suivi » ou « pas de victoire » décrivent la mission de leur date, et sont ensuite complétées par les preuves suivantes.
- `ARMV6_BUNDLE_VALIDATION.json`, `FIRST_LAUNCH_EVIDENCE.json`, `FULL_TOUCH_PLAN.json` : ABI, identité du véritable binaire invité, entrées et attentes séparées des observations.
- Les deux journaux invités, dont `full-playthrough/GUEST_EXECUTION.log`, les registres finaux et `full-playthrough/EVIDENCE.json` : 26 états, 25 callbacks, transitions, victoire et hashes. Ces journaux de preuve originaux sont expressément sélectionnés ; les stdout/UART/build/logs de campagne privés restent exclus.
- **32 PNG originaux ou diagnostiques identifiés** : huit captures RGB du jeu, quatre couches natives initiales/finales, quatre placements d’une seule couche, huit panneaux hypothétiques et huit aperçus avec bannière. Les placements et hypothèses ne sont jamais classés comme scanout matériel composé.
- Deux `CAPTURE.json` natifs, byte-identiques aux manifestes d’origine, leurs `PREVIEWS.json`, `CLCD_EVIDENCE.json`, les `HYPOTHESES.json` et `COMPARISON.json`. Chaque paire provient du même instant invité.
- [PUBLIC_EVIDENCE_MANIFEST.json](PUBLIC_EVIDENCE_MANIFEST.json) : admission précise par chemin, SHA-256 et catégorie. Les hypothèses exigent préfixe de fichier, métadonnée PNG et `hardware_validated:false`.

**Hors Git et conservés sans suppression :** les huit PPM de Relic Maze, les 54 exports natifs et 27 manifestes privés d’origine, exécutables ARM/host, bundles installés, ELF/objets, payloads, disques, snapshots, SDK, bibliothèques Apple et logs de campagne. Les huit PPM disposent désormais de huit exclusions exactes dans `.gitignore` ; ils ne deviennent pas des données perdues. Tous les nouveaux builds/tests sont isolés dans `.local/` ignoré.

Les PNG RGB permettent de reconstruire les huit PPM et d’en retrouver les hashes originaux. Les quatre PNG natifs RGBA permettent de reconstruire exactement les quatre spans BGRA sélectionnés, avec leur alpha et leurs hashes. Aucun padding n’est inventé : ces quatre fenêtres ont stride = largeur × 4. Les autres exports restent privés. Cela conserve des preuves vérifiables sans versionner 32,6 Mo de dumps natifs.

## Documentation et licences

README anglais/français, COMPATIBILITY, UPSTREAM, ARCHITECTURE, BUILDING et ROADMAP indiquent le niveau exact : jeu C natif terminé, pas Unity, frontend Objective-C non compilé, couches séparées authentiques, composition hypothétique, temps réel non démontré, Android ARM64 futur. La provenance S5LBox de j0shua-SYSON reste explicite. La comparaison amont datée reste historique ; aucune nouvelle promesse sur son état actuel n’est ajoutée.

Le `LICENSE` MIT S5LBox et ses notices restent byte-identiques. Le jeu original dispose de sa propre licence MIT IOSretroMob contributors. Les nouveaux outils/tests sont originaux MIT. Aucune implémentation GPL OpeniBoot/iDroid, donnée commerciale, firmware, framework Apple ou SDK Unity n’est incorporé. Les références de recherche dans les anciens rapports servent à documenter les limites.

## Validation sans firmware ni anciens builds

Un clone local neuf de la base, sans hardlinks, firmware, work ou build, reçoit uniquement les fichiers destinés à Git pour la validation préalable. La configuration Release et compilation `-j2` passent. La validation finale rattache ce checkout au HEAD intégré à partir du dépôt local, vérifie l’identité de l’arbre et la propreté Git, puis relance configure/build/CTest et l’audit complet. Les dépendances sont le compilateur C local, CMake, Python standard, Node pour le navigateur et LLVM installé pour les builds ARMv6 ; aucune dépendance volumineuse n’est copiée ou téléchargée.

| Contrôle exécuté avant commits | Résultat |
|---|---|
| CMake Release / compilation -j2 | PASS ; compilation neuve, environ 19,2 s |
| Suite CTest complète | **81/81 entrées passent**, environ 33,9 s |
| Logique C et texture Relic Maze | 2/2 ; route complète et atlas dynamique |
| Solveur indépendant | PASS ; cartes C/JS/C# identiques, 25 mouvements |
| Frontend/builder Relic Maze | **4/4 tests Python**, LLVM installé ; ABI ARMv6, imports classiques, r9 réservé, ressources, CFBundle plat |
| Diagnostics ARMv6 / package / TA | 8/8 tests Python ; les compilations LLVM sont réellement exercées |
| Logique navigateur avec Node | PASS ; victoire, reset, collisions |
| CLCD natif C et référence Python | PASS ; 8/8 oracles Python, bornes mémoire/strides/formats/alpha |
| Composition expérimentale | 10/10 tests analytiques ; aucune acceptation matérielle implicite |
| Vérification des preuves | PASS ; 32 PNG, huit identités PPM, quatre spans natifs, 26 états et 25 callbacks |
| Contrôles négatifs de l’audit | 7/7 ; sélection, payloads, secrets, hashes, structure CRC PNG et marquage des hypothèses |
| Fichiers indexés et tous objets Git | PASS ; zéro anomalie d’admission ou de secret |

**Sauts distincts : 28 annonces**, toutes conservées dans les journaux : **17 oracles static A64** faute de handlers signés, et **11 contrôles/branches dépendant de données privées absentes** (noyau réel, trois cas app restore/poweroff/seam, paquet Debian, real-package-plan, trois cas stockage/Cydia/install-build, DeviceTree et firmware boot). Certaines entrées CTest renvoient succès après un skip interne : 81 entrées passées ne signifient pas que ces 28 portions ont été exécutées. Aucun saut LLVM ou Node dans cette validation. Le « working C compiler skipped » de CMake est une optimisation de configuration, pas un test ignoré supplémentaire.

Les seules mises en garde de linkage sont des bibliothèques dupliquées ignorées par ld. Une alerte whitespace identifie une ligne vide finale dans le journal de registres historique : elle est traitée par un attribut **limité à ce fichier**, sans modifier ses bytes. Les journaux invités et deux manifestes natifs sont `-text` pour préserver leurs empreintes au checkout. `git diff --check` passe après cette mesure.

Reproduire les validations à partir du HEAD local intégré :

```sh
git clone --no-local --no-hardlinks --branch codex/relic-maze-armv6-first-launch CHEMIN_DEPOT_LOCAL checkout
cd checkout
export IOSRETROMOB_LLVM_BIN=/path/to/complete/llvm/bin
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build -V
python3 tools/verify_relic_maze_evidence.py
python3 tools/audit_public.py
```

Les rapports historiques décrivent des commandes de campagne privée : ne pas les rejouer pendant cette consolidation. Les sorties publiques des replays sont explicitement synthétiques. Un test du builder n’exécute jamais l’ARMv6 sur le Mac.

## Volume et audit de sécurité

La limite est **20 Mio = 20 971 520 octets**, contrôlée séparément sur les fichiers suivis, tous les objets Git non compressés (y compris inaccessibles) et les blocs alloués aux métadonnées Git. Le contrôle pré-commit mesure **14,144,644 octets suivis**, **18,647,658 octets d’objets** et **4,530,176 octets de blocs Git**. Le rapport lui-même et les trois petits objets de commit ne font pas approcher la limite. Les mesures finales exactes sont dans `final-closure.json` ; bornes finales vérifiées : **moins de 14 Mio suivis, moins de 19 Mio d’objets, moins de 6 Mio de blocs Git**, toutes sous 20 Mio.

L’allowlist passe de 386 à 475 chemins exacts. Les contrôles originaux des chemins positifs, de tous les blobs et commits, secrets, modes historiques et captures de la galerie demeurent. S’ajoutent des exclusions de payloads/dépendances/builds, des tests négatifs et la validation stricte des nouvelles images avec hashes et métadonnées. Un fichier ignoré déjà versionné serait toujours examiné. Aucun `git add .`, import de gros objet, history rewrite, force push ou workflow.

Budget de cette mission : 512 Mio de nouvelles données et au moins 5 Gio de réserve, vérifiés avant allocation. Le clone et les compilations restent de l’ordre de quelques dizaines de Mo ; `work/` reste à **2 805 486 240 octets**, aucun média nouveau. Les tailles logiques, blocs alloués, `df -h`, `df -k` et `du -sk work` sont conservés ; la somme de blocs APFS n’est pas une mesure exclusive de consommation physique.

## Clôture et différences par rapport à main

Les sources/proofs nouveaux et les documents décrits sont intégrés sur la branche locale, trois commits devant main. Le registre de sélection donne chaque taille/hash. Les anciens rapports et images, les sources C invitées, Unity et les anciens fichiers privés sont comparés au baseline ; seules les adaptations documentaires/CMake/exclusions autorisées changent. Aucun disque ou autre fichier préexistant n’est supprimé. Les huit PPM et médias/builds demeurent **ignorés**. L’état attendu à clôture est un index et un working tree propres, aucun fichier non suivi non ignoré ; les éventuels écarts arrêtent la clôture et sont rapportés.

L’historique amont local et `origin/main` restent à la base publiée ; main distante est revérifiée sans push. Le projet distant ne contient donc pas ces nouveaux développements. Les valeurs exactes et `git status` final sont conservés dans `final-closure.json`, ainsi que les logs séparés du clone final et des audits.

Prochaine étape Unity : identifier un runtime historique légalement utilisable, auditer ses vrais imports/ABI, puis démontrer initialisation, trames et entrées dans un guest autorisé avec budget borné. Relic Maze fournit une référence native utile ; sa réussite ne valide pas Unity. Établir les règles CLCD manquantes avant d’annoncer un écran complet fidèle, puis mesurer séparément la cadence et préparer le host Android ARM64.
