# CLCD multi-fenêtres — captures natives UIKit et OpenGL ES

Validation locale du 10 octobre 2026. **Exports bruts multi-fenêtres : PASS. UIKit initial et texte de victoire : observés. Composition matérielle complète : non établie, aucun composite produit.**

L’exporteur observe simultanément les quatre fenêtres du contrôleur émulé. Relic Maze fournit deux fenêtres actives : le plateau GLES en fenêtre 0 et UIKit avec alpha en fenêtre 2. Les boutons directionnels, Reset et le HUD initial sont visibles ; le HUD final affiche `Relic Maze 3/3 25 moves WON`. Les régions des boutons sont identiques entre ces deux captures. La victoire historique reste valide ; aucune donnée antérieure n’a été utilisée comme destination mutable.

## Audit et limites du système antérieur

`tools/guest_clcd_capture.h` utilise `s5l_clcd_active_window()` : première fenêtre activée selon les bits 0x40, 0x20, 0x10, 0x08 de `ctrl`. Ce choix correspond à l’adoption d’une surface par le pilote, sans démontrer l’ordre des couches d’affichage. `s5l_clcd_scanout()` convertit cette fenêtre en RGB et supprime l’alpha ; le sampler exporte une image relative à la fenêtre, sans appliquer sa position. La classification ancienne est fondée sur la profondeur et ignore les bits d’ordre. Le sampler et cette conversion restent byte-exactement inchangés.

Les quatre blocs de fenêtres commencent aux offsets 0x58, 0x70, 0x88, 0xa0, chacun sur 0x18 octets : stride, contrôle, adresse physique, géométrie, linewords, position. Géométrie : largeur bits 26:16 masqués à 0x7ff, hauteur bits 9:0 ; position : x u16 haut, y u16 bas. Format : contrôle bits 10:8 ; champ appelé ordre par le modèle : bits 17:16. Aucun sens signé ou swizzle supplémentaire n’est inventé.

Les registres globaux exportés comprennent enable/disable/ctrl/fifo, masques et état d’interruption, reg1c/preenable/backdrop/update/update2/gate, vidéo, configuration auxiliaire de fenêtres, CSC, VIDCON1/timings et les trois tables gamma. Les mots auxiliaires aux offsets 0xd8–0xec sont conservés ; 0xe8 est aussi le registre update2 dans le modèle. Le contrôleur conserve ces valeurs sans modéliser leur composition, gamma, matrice couleur ou overlay vidéo. Les captures représentent ses registres programmés et sa RAM à l’instruction indiquée, sans prétendre reproduire un état de latch matériel absent du modèle.

La vérification du code primaire [OpeniBoot S5L8900 LCD](https://raw.githubusercontent.com/iDroid-Project/openiBoot/master/plat-s5l8900/lcd.c) et de son [en-tête matériel](https://raw.githubusercontent.com/iDroid-Project/openiBoot/master/plat-s5l8900/includes/hardware/lcd.h) corrobore les blocs de fenêtre et les positions. Les fonctions de configuration n’établissent pas une règle complète de priorité et d’alpha pour les valeurs observées ici. Aucun code GPL de ce projet n’a été incorporé ; les ajouts sont des sources originales MIT utilisant les structures S5LBox existantes.

## Modifications génériques

- `tools/clcd_multilayer_capture.h/.c` : vue native bornée, métadonnées des quatre fenêtres, SHA-256 des données et du JSON, écriture synchrone sans avancer le guest entre fenêtres.
- `tools/guest_clcd_multilayer.h` : sampler opt-in séparé du sampler historique, période et intervalle obligatoires, limite d’octets et maximum de 128 exports distincts. Déduplication sur pixels, alpha et registres pertinents ; les seuls compteurs de frames/interruptions ne créent pas de doublons.
- `tools/bootkernel.c` : branche d’observation dans la boucle interpréteur par défaut. Aucun changement CPU, GPU, CLCD ou UIKit. La limite de 32 taps était un changement de la mission précédente ; elle est conservée.
- `tools/clcd_layers.py` : validation SHA/taille/stride, décodage fidèle des canaux, PNG RGBA et placement/clipping d’une seule fenêtre sur un panneau transparent 320 × 480. Ce placement ne mélange aucune couche.
- `core/tests/test_clcd_multilayer.c`, `core/tests/test_clcd_layers.py` : petites fixtures synthétiques et références de bytes/couleurs indépendantes.
- `core/CMakeLists.txt` : bibliothèque hôte, liaison du CLI et deux tests CTest. Python 3 est requis pour la référence indépendante.

`core/src/soc/clcd.c`, `core/include/soc.h`, le header historique de capture et toutes les sources du jeu sont inchangés. L’ancien changement de `core/tests/check_bootkernel_options.cmake` est conservé. Les snapshots gardent leur disposition binaire et leurs exécutables compatibles.

Formats strictement reconnus : **format 7 / ordre 0**, bytes BGRA8 en mémoire correspondant à l’ARGB déclaré par le pilote, et **format 3 / ordre 0**, RGB565 little-endian. Tous les autres codes ou ordres sont explicitement refusés, même si la conversion historique les assimile à une profondeur connue. Une fenêtre inconnue apparaît dans le manifeste avec son statut et ses registres, sans pixels devinés. Aucun format inconnu n’a été rencontré dans la campagne réelle.

Le natif conserve les bytes d’alpha et le padding entre lignes ; la longueur est `(height-1)*stride + width*bpp`, sans padding inutile après la dernière ligne. Les limites mémoire et le dépassement de l’espace physique 32 bits sont vérifiés avant lecture. Les fichiers sont créés exclusivement, jamais écrasés. Le JSON écrit en dernier marque un bundle complet ; un échec conserve les fichiers partiels et désactive l’export. La réserve de métadonnées est vérifiée avant toute écriture. La destination doit être préparée et contrôlée ; le sampler ne crée pas de répertoire. La borne de 8 Mio par fenêtre est une limite de cet exporteur, pas une limite matérielle.

L’activation exige les cinq variables `IOSRETROMOB_CLCD_LAYERS_DIR`, `IOSRETROMOB_CLCD_LAYERS_START`, `IOSRETROMOB_CLCD_LAYERS_END`, `IOSRETROMOB_CLCD_LAYERS_INTERVAL`, `IOSRETROMOB_CLCD_LAYERS_MAX_BYTES`. DIR désigne un dossier contrôlé existant, START/END/INTERVAL sont des instructions invitées et MAX_BYTES est la limite globale en octets. Pour inspecter un export existant sans redémarrer le guest :

```sh
python3 tools/clcd_layers.py MANIFEST.json --output NOUVEAU_DOSSIER
```

Le dossier de sortie doit être neuf ; les images produites sont des couches individuelles. La commande réelle et son budget doivent toujours être préparés avant toute nouvelle campagne.

**Limite de backend :** l’intégration automatique concerne la boucle interpréteur par défaut. Les variantes run-api/static A64 ne sont pas instrumentées. Sans les variables d’activation, le nouveau sampler reste désactivé.

## Tests effectivement exécutés

Compilation Release des objets hôte et du CLI isolé, puis intégration CMake Release des cibles `bootkernel` et `test_clcd_multilayer` avec **-j2 : PASS**. Le CLI de campagne utilise les bibliothèques moteur conservées ; la vérification CMake supplémentaire construit dans son propre dossier. Aucun ancien exécutable n’est remplacé.

**CTest nouveau : 2/2 PASS**, comprenant les assertions C et **8/8 tests Python indépendants, zéro skip**. Les références vérifient plusieurs fenêtres, positions non nulles, strides avec padding, frontières exactes de RAM, wrap physique, refus de géométrie/format/ordre, canaux alpha 0/intermédiaires/255, RGB565, clipping, corruption SHA, chemin de fuite refusé, non-écrasement et round-trip de tous les canaux PNG. Les scénarios sampler testent six configurations invalides, la borne de 128 captures et un budget insuffisant sans sortie partielle. L’ancienne sortie PPM synthétique reste exactement `P6\n3 2\n255\n` suivie des bytes RGB attendus.

Le premier essai des fixtures a échoué : `NDEBUG` de Release supprimait les expressions d’assertion contenant les appels de création de fixtures. Le test C désactive maintenant NDEBUG avant assert.h ; toutes les vérifications ont été rejouées après correction, avant la campagne. L’échec et ses logs sont conservés, sans être comptés comme une réussite.

**12/12 entrées CTest moteur ciblées PASS** : machine/CLCD, MBX 2D, MBX VGP, Z2, mapping et queue tactile, interpréteur ARM, parseur Mach-O, provisionnement HFS, payload tar, replays publics et galerie. **17 sous-tests oracle static A64 sont SKIP**, faute de handlers AArch64 signés ; ils ne sont pas présentés comme exécutés. Les suites d’options CLI (8/32/33 taps) et external-md passent avec le nouveau CLI. Aucune affirmation de réussite de la suite moteur entière n’est tirée de cette sélection.

Logs privés : `build-validation.json`, `cmake-integration.json`, `native-independent-bounded-final.log`, `new-CMake-tests.log`, `engine-regressions.log`, et les premiers résultats conservés, dans `.local/clcd-multilayer-2026-10-10/`.

## Une seule campagne invitée justifiée

Les anciens journaux et PPM prouvent déjà la victoire. Ils ne contiennent ni les bytes alpha ni la fenêtre 2, et les disques persistants ne contiennent pas la RAM du guest. Aucun snapshot RAM Relic Maze disponible ne permet un export rétrospectif. Une nouvelle exécution était donc nécessaire pour observer UIKit, avec la même application ARMv6 et le parcours validé.

Préflight : politique de stockage historique lue, `df -h`, `df -k`, `du -sk`, tailles logiques et blocs enregistrés ; aucun autre bootkernel actif. Empreintes vérifiées du firmware, payload, jeu et des neuf composants de référence. Pire allocation planifiée **573 563 225 octets**, borne exports **80 000 000**, marge preuves **20 000 000**, limite mission **600 000 000** ; plus de 5 Gio de réserve après allocation. Un seul disque neuf `work/clcd-multilayer-2026-10-10/cold-01/rootfs.img`, copié par le chemin external-md existant, sans clone APFS revendiqué et sans snapshot.

Le plan et la commande exacte, les variables d’environnement et les hash d’entrées sont conservés dans `plan.json`, `result.json`, `preflight.json`, avec les scripts dans `reproduction/`. Boot borné à **10 250 000 000 instructions**, terminé normalement en **1 070,5 s**, exit 0, aucun arrêt budget.

Échantillonnage : instructions 7 125 000 000 à 10 225 000 000, intervalle 50 000 000. Un vrai tap UIKit Reset à 7 500 000 000 produit l’état initial zéro mouvement ; le premier déplacement arrive à 7 700 000 000. Les 25 directions `DDDDDDUURRRUURUUDDRRDDDDL` reproduisent la route existante jusqu’à la victoire, sans jeu reconstruit sur l’hôte. Le total CLI comprend 28 taps (deux système, Reset, 25 déplacements) et deux drags système.

Application ARMv6 inchangée SHA-256 `e23578d8dd3b7fc1d471efbf5d57dd55b982d82fe5d2f3b644023da7913242ca` ; CLI de campagne `96a76549ef80e8f9884210f25c8eb6ab68b29e70ede8381d524e1f780f18c886`. L’extraction HFS après arrêt confirme les six fichiers du bundle, bytes et modes identiques aux originaux.

Journal invité : PID **39**, initialisation UIKit/EAGL/FBO confirmée, **26 callbacks UIKit** (Reset + 25 directions), **27 présentations GLES**, aucune erreur texture/draw/present. Les 27 états observés correspondent au parcours attendu. Final : joueur (6,7), caisse (5,5), moves 25, mask 7, plate 1, **won=1**, résultat rm_step 2. Ce résultat provient du véritable journal invité extrait du disque arrêté.

## Exports natifs et images authentiques

**27 bundles JSON / 54 exports natifs**, **32 575 659 octets** de captures avec métadonnées. Toutes les empreintes brutes et les empreintes JSON annoncées par le CLI ont été recalculées ; aucun fichier orphelin ou bundle partiel. Fenêtres 1 et 3 désactivées ; fenêtres 0 et 2 reconnues en format 7 / ordre 0, contrôle 0x700, stride 1280.

Les fichiers suivants restent privés, sous `.local/clcd-multilayer-2026-10-10/layers/` :

| État | Instruction invitée | Fenêtre | Export natif | Géométrie et placement | SHA-256 |
|---|---:|---:|---|---|---|
| initial | 7525000000 | 0 | `layers-0001-at-7525000000-win0.raw` | 320 × 460, (0,20), stride 1280 | `e827da5f93902c285df97bb5763419f1aaeea16f9417565ad642d0b34f632ac6` |
| initial | 7525000000 | 2 | `layers-0001-at-7525000000-win2.raw` | 320 × 480, (0,0), stride 1280 | `fcb1ef5ba32bd969cc735d4e32b443fe9a128c8821a0b2bb701a7e9a46c6ff0e` |
| victory | 10125000000 | 0 | `layers-0026-at-10125000000-win0.raw` | 320 × 460, (0,20), stride 1280 | `f538a189891e94c424a2ffd774830e29a2d4c9def558b60ac2fc4c60db5bd776` |
| victory | 10125000000 | 2 | `layers-0026-at-10125000000-win2.raw` | 320 × 480, (0,0), stride 1280 | `af3198ee990fd30dbdfc2382701978db1b9f0d3735f3d4cfbf822f9a946f4c02` |

- initial : `layers-0001-at-7525000000.json`, SHA-256 `a5ac30891af76c8ce312f63d61a4fc78cf433f9a127ccc1813fc3e504b564ff3`.
- victory : `layers-0026-at-10125000000.json`, SHA-256 `b356e3ef59e509be1ab21f7145a67e1e754a2c275605f4af17d72eb9f02e415a`.

Le plateau initial retenu à 7 525 000 000 est après Reset et **avant le premier déplacement**. Le plateau final et tous les états 0–25 ont été retrouvés par 441 points de palette indépendants par fenêtre. L’ensemble RGB du plateau final est byte-identique à la capture de victoire historique : SHA-256 `173800b52db030ec82fdc71f0832cc531b14b0ed39d3e459314b3b43312c2419`. Cette comparaison exclut l’alpha que l’ancien PPM n’avait pas conservé.

Fenêtre 0 : 147 200 pixels alpha 255 aux deux jalons. Fenêtre 2 initiale : **134 881 alpha 0, 16 827 alpha 255, 1 892 intermédiaires**. Finale : **134 598 alpha 0, 16 857 alpha 255, 2 145 intermédiaires**. Tous les pixels observés vérifient R/G/B ≤ A, ce qui est compatible avec des couleurs prémultipliées ; cette observation ne prouve ni les règles de mélange ni les modes matériels.

Revue des PNG natifs : boutons **Up, Down, Left, Right**, **Reset**, HUD `0/3 0 moves` puis **`3/3 25 moves WON`**. Les rectangles UIKit vérifiés et leurs hash RGBA sont dans [CLCD_EVIDENCE.json](CLCD_EVIDENCE.json). Les boutons ont 9 808 pixels alpha non nul et Reset 1 612 ; leurs bytes restent identiques entre début et victoire. Le HUD change de 899 à 1 182 pixels alpha non nul. Le point (20,100) de la fenêtre UIKit reste RGBA (0,0,0,0), laissant une zone transparente réelle, sans plateau ajouté par l’hôte.

Aperçus avec tous les canaux conservés :

- [UIKit initial, fenêtre 2, 320 × 480](clcd-multilayer/initial/window-2-native.png).
- [Plateau initial, fenêtre 0, 320 × 460](clcd-multilayer/initial/window-0-native.png).
- [UIKit victoire, fenêtre 2, 320 × 480](clcd-multilayer/victory/window-2-native.png).
- [Plateau victoire, fenêtre 0, 320 × 460](clcd-multilayer/victory/window-0-native.png).

Chaque dossier contient aussi le placement d’une fenêtre seule sur panneau transparent 320 × 480 et `PREVIEWS.json` avec hash PNG/RGBA et instant invité. Un PNG RGBA conserve les canaux natifs ; son affichage par un lecteur PNG ne valide pas l’alpha prémultiplié matériel. **La fenêtre UIKit 320 × 480 de victoire est authentique, mais elle n’est pas l’écran iPhone composé.** Aucun HUD, bouton ou texte de victoire n’a été dessiné côté hôte.

## Pourquoi la composition s’arrête ici

L’activation simultanée de 0 et 2, la position (0,20) du plateau et la présence d’alpha en UIKit ne suffisent pas à prouver que 2 est au-dessus de 0, ni à déterminer l’usage du byte alpha, d’alpha constant, de chroma-key ou des modes auxiliaires. Le tri numérique des fenêtres et un mélange standard seraient des hypothèses. Aucun compositeur approximatif ni capture prétendue complète n’est livré. Les exports incluent les données nécessaires à une future preuve ; les modes vidéo/CSC/gamma, ordres non nuls et autres formats restent non couverts.

Prochaine étape : établir la priorité et les modes d’alpha CLCD avec une référence matérielle ou une analyse documentée des registres/pilote, tester indépendamment ces règles, puis brancher un affichage interactif générique avec les mêmes captures natives. Les données de cette campagne permettent cette analyse sans nouveau boot pour réexaminer le HUD et les boutons.

## Intégrité, stockage et statut Git

Les neuf composants de référence ont été rehashés avant et après la campagne. Les 19 anciens fichiers work, 25 anciennes preuves Relic Maze, 71 fichiers du jeu et les deux dossiers privés antérieurs (415/186 fichiers) restent identiques. L’inventaire historique S5LBox est identique pour les sources, firmware, disques et preuves. **Exception de métadonnées enregistrée : deux anciennes références `.git/refs/codex/turn-diffs/` ont disparu et deux nouvelles sont apparues** pendant la session, compatible avec la maintenance interne de l’application Codex ; leur auteur exact n’est pas établi par l’inventaire. Le premier contrôle intégral signale cette différence ; le contrôle de contenu la distingue sans effacer son résultat. Aucune restauration de ces références et aucune écriture volontaire dans S5LBox n’ont été effectuées. `guest_clcd_capture.h` et les binaires de référence restent inchangés.

Au contrôle de clôture : données privées nouvelles **47,501,316 octets**, nouveau work **467,715,541 octets** ; preuves PNG/JSON/report de petite taille. Borne conservatrice incluant les fichiers source modifiés entiers : **517,163,873 octets**, avec **1 000 000 octets** réservés pour finaliser rapport/audits, soit **moins de 519 Mo**, sous 600 Mo. `work/` passe de **2 337 770 699** à **2,805,486,240 octets logiques**, environ 2,805 Go, sous 10 Go.

Somme des blocs alloués nouveaux work + privé : **487,620,608 octets** ; ce total n’est pas un coût exclusif APFS. `df -h` donne environ **69 Gio libres** en fin, contre 70 Gio avant. La variation df inclut les autres activités du Mac ; aucune économie CoW n’est revendiquée. `df -k`, `du -sh/-sk`, tailles logiques, blocs et mesures régulières sont conservés dans `storage-final.json`, `preflight.json` et `storage-monitor.json`. Aucun disque ou preuve n’a été supprimé, aucun nouveau checkpoint durable créé.

Audit tracked + **tous les objets Git : PASS**, 386 fichiers suivis, quatre commits inchangés, 444 objets / 399 blobs inspectés. HEAD reste `d25814b873aa528ed223651363e5993541cf6654`, branche locale `codex/relic-maze-armv6-first-launch`. L’audit supplémentaire couvre tous les candidats non suivis non ignorés et tous les fichiers modifiés, y compris sources et rapports précédents, avec les nouvelles images admises uniquement à leurs hash authentiques. Les données privées/firmware/SDK/checkpoints ne sont pas destinés à Git. La liste positive publique existante reste inchangée : ces développements et preuves sont **locaux, non indexés, non commités et non publiés**. Les 70 fichiers candidats passent le contrôle mécanique final, sans problème détecté. Le bilan final détaillé est dans `candidate-audit.json` et `tracked-object-audit.json` ; sa sélection positive reste une étape distincte avant une future publication.

Aucun changement Unity, traitement spécial Relic Maze dans le moteur, commit, push, release ou CI/GitHub Actions pendant cette mission. Licences MIT et attributions S5LBox/j0shua-SYSON préservées.
