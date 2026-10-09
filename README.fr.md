# IOSretroMob

**Historical iOS Game Preservation & Compatibility**

IOSretroMob est une plateforme expérimentale de préservation des anciens jeux iOS, fondée sur **S5LBox de j0shua-SYSON**, sous licence MIT. Le CPU ARMv6, le système S5L8900 et le démarrage d’iPhone OS proviennent de S5LBox.

Nos apports sont un pipeline VGP limité aux formes mesurées, des diagnostics originaux et un laboratoire public vérifiant géométrie, couleurs et textures sans firmware Apple. Les six captures du [README anglais](README.md) sont de véritables scanouts invités historiques. Les images produites par les replays publics sont explicitement synthétiques.

UIKit, le tactile, des triangles et des textures nearest/linear ont été observés localement. La profondeur reste non résolue. Aucun jeu Unity, Temple Trouble, APK Android ni fonctionnement sur Samsung S23 n’est validé. Les neuf cibles Android compilées prouvent uniquement la compilation croisée.

```sh
./iosretromob doctor
./iosretromob test
./iosretromob replay lab/fixtures/square-linear.json --output .local/demo
./iosretromob verify-gallery
```

Les tests publics ne nécessitent ni firmware ni SDK Apple. Pour le démarrage invité, fournir séparément des composants autorisés. Ne jamais publier de clés, disques ou snapshots. Notre snapshot v33 diffère du v33 amont et ne doit pas être mélangé avec lui.

Voir [BUILDING.md](BUILDING.md), [COMPATIBILITY.md](COMPATIBILITY.md), [UPSTREAM.md](UPSTREAM.md) et l’[audit de publication](reports/PUBLIC_REPOSITORY_AUDIT.md). Le dépôt historique est conservé sans modification ; la publication du nouveau dépôt attend l’autorisation finale de l’utilisateur.
