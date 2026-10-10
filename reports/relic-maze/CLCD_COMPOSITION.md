# CLCD — composition expérimentale à partir des couches existantes

Validation hors ligne du 10 octobre 2026. **Calcul et intégrité : PASS. Composition matérielle S5L8900 : NON VALIDÉE.** Un prototype générique compare quatre hypothèses et produit des panneaux 320 × 480 avec plateau, commandes UIKit et victoire. Leur présentation porte explicitement la mention **HYPOTHESIS ONLY**. La priorité matérielle et les équations du contrôleur restent insuffisamment démontrées ; aucun fichier nommé `relic-maze-initial-composed.png` ou `relic-maze-victory-composed.png` n’est présenté comme capture fidèle.

La victoire invitée précédemment prouvée reste acquise. Aucun nouvel iPhone OS exécuté, disque créé, capture native régénérée, firmware ou SDK acquis. Toutes les images du jeu et de son interface proviennent des pixels déjà capturés ; le seul dessin hôte ajouté est une bannière diagnostique placée **hors du panneau**, au-dessus de celui-ci.

## Entrées authentiques préservées

Les **27 manifestes / 54 exports natifs** de `.local/clcd-multilayer-2026-10-10/layers/` ont été vérifiés avant traitement : SHA-256 des JSON comparés aux empreintes annoncées dans le journal historique, SHA-256 et tailles des fichiers bruts comparés à chaque manifeste. Aucun export manquant, supplémentaire ou partiel. Les descripteurs sont également confrontés aux registres bruts par le nouvel outil ; une divergence provoque un refus.

| État | Instruction invitée | SHA-256 du manifeste |
|---|---:|---|
| Initial, après Reset et avant le premier mouvement | 7525000000 | `a5ac30891af76c8ce312f63d61a4fc78cf433f9a127ccc1813fc3e504b564ff3` |
| Victoire | 10125000000 | `b356e3ef59e509be1ab21f7145a67e1e754a2c275605f4af17d72eb9f02e415a` |

Les quatre empreintes des exports utilisés figurent dans les deux `HYPOTHESES.json` livrés et dans [le rapport précédent](CLCD_MULTILAYER_CAPTURE.md). Chaque comparaison assemble exclusivement des couches **du même manifeste et du même instant invité**. Aucun mélange de début et de fin, aucune ressource du jeu utilisée pour redessiner l’interface.

Les 54 exports vérifient R/G/B ≤ A et RGB = 0 lorsque A = 0. La fenêtre 0 conserve 320 × 460 pixels opaques à (0,20) ; la fenêtre 2 conserve 320 × 480 pixels à (0,0). Contrôle 0x700, format 7, champ dit `order_code` égal à 0, stride 1280. Fenêtres 1 et 3 désactivées. Les décompositions BGRA8 natives restent celles de l’exporteur précédent.

## Sources examinées et portée de leurs preuves

Le fichier [OpeniBoot S5L8900 lcd.c, révision 866562f](https://raw.githubusercontent.com/iDroid-Project/openiBoot/866562fdb1cfd019bcd77885c80fbf0af65d5c15/plat-s5l8900/lcd.c) et [son en-tête matériel](https://raw.githubusercontent.com/iDroid-Project/openiBoot/866562fdb1cfd019bcd77885c80fbf0af65d5c15/plat-s5l8900/includes/hardware/lcd.h) permettent de reconnaître les blocs, adresses, dimensions, pas mémoire et placements. `setLayer` encode deux coordonnées ; son nom ne prouve pas un ordre Z. `setWindow` place `wordSetting` dans les bits [18:16], avec des valeurs différentes pour RGB565 et RGB888. Cela ne démontre pas une priorité des fenêtres. `configWindow` écrit des options non nommées aux offsets D8/E0/E8 ; `configLCD` modifie des champs globaux sans documenter leur équation de mélange. Le tracé RGB888 opaque d’OpeniBoot ne fournit pas de test d’alpha par pixel.

Les pilotes framebuffer historiques [kernel_common, révision ec0e7d0](https://raw.githubusercontent.com/iDroid-Project/kernel_common/ec0e7d06381f90c9ad7eee1b12e17644472938c4/drivers/video/iphone-fb.c) et [iDroid-kernel, révision 3f971a6](https://raw.githubusercontent.com/iDroid-Project/iDroid-kernel/3f971a676096c37472aec5e139b2720245e509e1/drivers/video/iphone-fb.c) sélectionnent une surface et son adresse. Ils déclarent les composantes RGB en mode quatre octets, mais ne spécifient pas ici une composition multi-fenêtres avec alpha. Ces sources ne tranchent donc pas les équations recherchées. Leurs copies et licences restent dans le dossier de recherche privé ignoré ; aucun code GPL n’a été importé dans les outils MIT.

Une inspection statique bornée d’AppleH1CLCD, à partir du noyau **déjà disponible**, confirme les écritures de placement et l’association des couches RGB logiques 0/1 aux fenêtres physiques 0/2. Elle confirme aussi l’écriture de 2 à D4 et de 0x50001000 à E8. Repères : routine à 0xc0704940, activations à 0xc0704ab4/0xc0704ac0, position à 0xc0704c7c–0xc0704c84, incrément de bloc à 0xc0705170. Le littéral 0xc0705264 a été relu dans le segment Mach-O. SHA-256 du noyau inchangé : `0d8cdb339d37cf37a1db2638fff79272ecd63a17764bf7666efa1618725df70c`. Ces faits ne donnent pas le comportement électrique du mélangeur. L’adoption de la première fenêtre activée à 0xc0705f00, et la « priorité » mentionnée dans les commentaires du modèle existant, concernent la **sélection d’une surface**, sans preuve d’ordre de composition.

La documentation [Quartz 2D sur les contextes bitmap](https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/drawingwithquartz2d/dq_context/dq_context.html) distingue les représentations prémultipliées et non prémultipliées ; elle ne décrit pas le CLCD S5L8900 ni le `bitmapInfo` actif de cette surface. Le [guide des vues UIKit](https://developer.apple.com/library/archive/documentation/WindowsViews/Conceptual/ViewPG_iPhoneOS/CreatingViews/CreatingViews.html) décrit l’ajout tardif d’une vue sœur au-dessus des précédentes. Il permet d’établir l’intention d’affichage du jeu, sans prouver le chemin matériel effectivement employé.

Les URL, révisions complètes et SHA-256 des six fichiers documentaires sont enregistrés dans `.local/clcd-composition-2026-10-10/research/SOURCES.json`. Aucun comportement n’est emprunté à un autre contrôleur Samsung.

## Ce qui est établi, observé ou encore hypothétique

| Question | Résultat et niveau de preuve |
|---|---|
| Adresses, pas, taille, placement | Registres capturés et écritures du pilote concordent ; 0 à (0,20), 2 à (0,0). |
| Priorité 0 contre 2 | **Non établie matériellement.** L’intention UIKit favorise 2 au-dessus de 0 ; la sélection d’une surface par le pilote n’est pas un ordre Z. |
| Bits dits d’ordre | Valeur observée 0. Le nom du champ logiciel n’en prouve pas la fonction ; priorité, permutation et fetch ne sont pas déduits. |
| Alpha par pixel | Octets alpha réellement présents ; le stockage contient des valeurs 0, 255 et intermédiaires. Leur consommation par le contrôleur n’est pas prouvée. |
| Prémultiplication stockée | **Fortement étayée**, conditionnellement au texte blanc UIKit sur fond transparent : les 844 pixels intermédiaires du HUD initial et les 1 097 du HUD final vérifient R = G = B = A. La source utilise `whiteColor` et un fond transparent. Cela distingue le stockage des couleurs blanches d’un stockage blanc non prémultiplié ; RGB ≤ A seul ne suffirait pas. |
| Équation matérielle et alpha constant | **Inconnus.** Aucun coefficient n’est décodé avec une sémantique démontrée. Ni alpha constant 1, ni source-over matériel ne sont affirmés. |
| Clipping et hors-fenêtre | Intersection géométrique testée dans le prototype. Pas de preuve de tous les cas de débordement matériel, de coordonnées signées ou de scaling. Backdrop RGB noir opaque imposé explicitement, compatible avec 0xff000000 mais non prouvé comme sortie physique. |
| Arrondi, colorimétrie, gamma, CSC, vidéo, key, latch | Non couverts ; aucune valeur de registre stockée ne suffit à prouver leur effet ou leur absence d’effet. |

Les registres globaux sont **identiques aux 27 instants** : notamment ctrl 0x01110051, D4 = 2, E8 = 0x50001000 et champs auxiliaires [0x1000,0,0x1000,0,0]. Formats, placements et champs d’ordre sont également fixes. Les changements de pixels ne constituent donc pas une expérience faisant varier les coefficients ou la priorité. Il manque une interprétation démontrée de ces champs, ou une référence indépendante de sortie matérielle pour ce mode précis.

## Prototype expérimental générique

Ajouts : `tools/clcd_composition_experimental.py`, `core/tests/test_clcd_composition.py`, et quatre lignes de déclaration CTest dans `core/CMakeLists.txt`. Le prototype ne connaît ni le nom du jeu, ni ses ressources. Aucun changement du moteur CLCD, du sampler historique ou de l’exporteur natif. Les modifications préexistantes de `bootkernel.c`, des tests d’options et de CMake restent distinctes de cette mission.

Le calcul réutilisable accepte jusqu’à quatre couches RGBA, des dimensions bornées, positions, clipping et un fond RGB opaque explicite. Le comparateur CLI exige exactement les deux fenêtres actives et examine **les deux ordres × deux conventions alpha**, sans ordre préféré. Entrées : BGRA8/format 7/ordre 0 et RGB565 little-endian/format 3/ordre 0, via le décodeur natif existant ; alpha 255 synthétique explicitement défini pour RGB565. Formats, permutations et contrôles non pris en charge sont refusés. Les pas mémoire et tailles sont vérifiés, les hashes requis, les sorties créées dans un dossier neuf sans écrasement.

Pour une composante source Cs, une composante destination Cd et A sur 0–255, les **hypothèses mathématiques** sont :

- prémultiplié : `floor((255*Cs + (255-A)*Cd + 127)/255)` ;
- non prémultiplié : `floor((A*Cs + (255-A)*Cd + 127)/255)`.

L’arrondi intervient après chaque couche. Le fond et la sortie sont opaques. Ce choix d’arrondi n’est pas attribué au matériel. Les couleurs incompatibles avec une représentation prémultipliée bornée sont refusées. Aucun mode alpha constant, color-key ou saturation supplémentaire n’est inventé.

## Résultats des comparaisons

| Observation | Initial | Victoire |
|---|---:|---:|
| Pixels fenêtre 2 alpha 0 / 255 / intermédiaire | 134881 / 16827 / 1892 | 134598 / 16857 / 2145 |
| Pixels différents entre les deux équations, fenêtre 2 au-dessus | 1892 | 2145 |
| Écart maximal de composante entre ces équations | 64 | 64 |
| Pixels différents entre les deux ordres, mode prémultiplié | 12319 | 12602 |
| Écart maximal de composante entre ces ordres | 240 | 240 |

Avec 2 au-dessus de 0, tous les pixels UIKit opaques sont copiés exactement ; les pixels alpha 0 laissent exactement passer le plateau placé ou le fond explicite. Le HUD, Reset et les quatre commandes directionnelles apparaissent. La victoire affiche **`3/3 25 moves WON` uniquement à partir des pixels invités**. Les deux équations diffèrent exclusivement aux zones intermédiaires ; leur plausibilité visuelle ne démontre pas le matériel.

Avec 0 au-dessus de 2, le plateau opaque cache le HUD et les commandes : **aucun** des 55/85 pixels opaques du HUD initial/final, des 1428 pixels opaques Reset et des 2236 pixels opaques de chacun des quatre boutons ne conserve sa valeur UIKit dans ces zones. Il reste 6400 pixels UIKit opaques identiques, dans les vingt lignes noires situées hors de la fenêtre 0. Les deux équations sont alors identiques. Cet ordre peut être rejeté **comme reproduction du contrat d’affichage voulu**, sous l’hypothèse que le chemin UIKit/pilote est correct ; il n’est pas formellement réfuté comme comportement matériel par une photographie de référence inexistante.

Huit panneaux hypothétiques 320 × 480 et leurs huit versions **320 × 504 avec bannière extérieure** sont livrés. Tous ont un préfixe `HYPOTHESIS`, un chunk PNG `CLCD_Status` et `hardware_validated:false` dans leurs manifestes. Les panneaux des versions étiquetées restent byte-identiques aux panneaux sans bannière.

| Hypothèse 0 sous 2, prémultipliée | PNG 320 × 480 : SHA-256 |
|---|---|
| Initial | `7939e3f8d0724ac56c8a36abdd615d64191b269cdaba2f070bbfa026f78962a6` |
| Victoire | `8a8f83a2d59eb9a9e44c51c962fdf1df1e6419ffc5b208181ee1c4afa5f1b4e9` |

Aperçus **hypothétiques**, à examiner avec leur bannière :

- [Initial : plateau et UIKit](clcd-composition-hypotheses/initial/HYPOTHESIS-w0-under-w2-premultiplied-labelled.png).
- [Victoire : plateau et UIKit](clcd-composition-hypotheses/victory/HYPOTHESIS-w0-under-w2-premultiplied-labelled.png).
- [Ordre inversé : UIKit masqué](clcd-composition-hypotheses/victory/HYPOTHESIS-w2-under-w0-premultiplied-labelled.png).

Les SHA-256 de toutes les PNG, leurs dimensions, le hash RGBA de chaque panneau, les deux empreintes d’entrée et les six comparaisons par paire sont dans les `initial/HYPOTHESES.json`, `victory/HYPOTHESES.json` et [COMPARISON.json](clcd-composition-hypotheses/COMPARISON.json).

## Tests et régressions

**10/10 tests synthétiques** du prototype passent, sans saut : exemple analytique distinguant les équations, identités alpha 0/255, 256 valeurs d’alpha contre `Fraction`, quatre couches, positions non nulles/négatives/hors-panneau dans l’aide géométrique, clipping, ordre, fond hors-fenêtre, erreurs de dimensions/modes, descripteurs malformés, hashes, contrôles inconnus, RGB565 et padding, CRC PNG, marquage et absence d’écrasement. Les fixtures sont originales et explicitement synthétiques.

Une référence indépendante parcourt chaque pixel du panneau, inverse les coordonnées des couches et utilise des fractions rationnelles exactes : **les 153600 pixels de chacune des huit hypothèses initiales/finales concordent**, soit 1228800 comparaisons de pixels. Les zones opaques et transparentes ont en plus des vérifications d’identité ; le HUD intermédiaire et les rectangles des boutons ont des contrôles séparés. Cela valide les calculs choisis, sans constituer un oracle matériel.

**8/8 tests indépendants de l’export natif**, plus les fixtures C natives existantes, passent. **8/8 entrées CTest moteur ciblées** passent : `s5l8900_machine`, `power_vr_mbx2d`, `power_vr_mbx_vgp`, `multitouch_z2`, `app_touch_map`, `app_touch_queue`, `iosretromob_public_replays`, `iosretromob_gallery`. **17 sous-tests static A64 sont SKIP**, faute de handlers signés ; ils n’ont pas été exécutés. Ce résultat ne prétend pas couvrir toute la suite moteur ni un nouveau boot.

Une configuration CMake Release dans un petit dossier neuf confirme l’enregistrement de `clcd_experimental_composition_reference`, puis **1/1 CTest passe**, contenant les dix tests Python. Pas de recompilation de l’émulateur nécessaire : aucune source de son exécutable n’a changé dans cette mission. Logs exacts et scripts de reproduction dans `.local/clcd-composition-2026-10-10/`, notamment `tests.json`, `*-final.log`, `engine-replays-touch-CLCD.log`, `analysis.json`, `observations.json`, `input-verification.json`, `reproduction/`.

Depuis la racine du projet, reproduction hors ligne dans un **nouveau** dossier :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/clcd_composition_experimental.py \
  .local/clcd-multilayer-2026-10-10/layers/layers-0026-at-10125000000.json \
  --manifest-sha256 b356e3ef59e509be1ab21f7145a67e1e754a2c275605f4af17d72eb9f02e415a \
  --windows 0,2 --backdrop-rgb 0,0,0 --output .local/victory-hypotheses-review
PYTHONDONTWRITEBYTECODE=1 python3 core/tests/test_clcd_composition.py
```

## Intégrité, audit et stockage

Le contrôle de clôture confronte les anciens fichiers au manifeste initial : **1093 fichiers de la capture précédente, 51 rapports/preuves, 23 fichiers work, 71 fichiers du jeu, 127 fichiers core et 72 outils**. Seul `core/CMakeLists.txt` reçoit les quatre lignes annoncées ; son préfixe historique reste identique. Les anciens exports, images, rapports, sources du jeu et disques restent inchangés. Le sampler historique et les binaires précédents restent inchangés. Aucun autre fichier historique n’est supprimé ou remplacé.

`tools/audit_public.py` vérifie les fichiers suivis et tous les objets Git ; un contrôle complémentaire couvre **tous les fichiers non suivis non ignorés et les fichiers modifiés**, y compris les sources/rapports des missions précédentes. Les nouvelles images sont admises uniquement comme hypothèses identifiées et hashées ; les anciennes captures par leurs empreintes de preuves. Aucun firmware, SDK, disque, snapshot, secret ou chemin personnel dans ces candidats. Résultat : **386 fichiers suivis, 444 objets Git / 399 blobs, 92 candidats locaux examinés, zéro anomalie détectée**. La sélection positive publique reste inchangée : **ces nouveaux travaux ne sont ni commités ni publiés**. Licences et attributions S5LBox conservées ; pas de code GPL importé.

Budget : **50 000 000 octets maximum**. Bilan exact dans `closure.json` : nouveaux fichiers et croissance des fichiers existants mesurés, avec leurs blocs alloués. Le volume total mesuré est d’environ **3,10 Mo logiques**, avec **6,20 Mo de blocs alloués cumulés**, sans assimilation à une consommation physique exclusive APFS. Les bornes de clôture vérifiées sont 4 Mo logiques et 8 Mo de blocs. Le volume APFS affiche 69 Gio disponibles avant et après ; ses variations globales dans `df -k` incluent aussi les écritures de fond et ne sont pas attribuées exclusivement à cette mission. Le préflight estimait 5 Mo logiques et disposait d’environ 74,2 Go libres. `work/` reste à **2 805 486 240 octets logiques**, sans aucun nouveau média. `df -h`, `df -k` et `du -sk work` avant/après sont conservés. Aucun nettoyage effectué.

Branche locale inchangée `codex/relic-maze-armv6-first-launch`, HEAD inchangé `d25814b873aa528ed223651363e5993541cf6654`. Aucun commit, push, release ou CI, aucune écriture volontaire dans S5LBox, aucun changement Unity.

## Prochaine étape

Continuer l’analyse statique des champs globaux/auxiliaires et du contrat des couches du pilote, ou obtenir une mesure indépendante du scanout S5L8900 avec des couleurs/alphas analytiques distinguant les coefficients et les ordres. Une validation doit aussi établir arrondi, backdrop, gamma/CSC et latching pour le mode employé. Ces informations permettraient de remplacer les hypothèses par un compositeur générique fidèle, puis d’alimenter un affichage interactif. Aucun démarrage invité supplémentaire n’est nécessaire pour réexaminer les données de cette mission.
