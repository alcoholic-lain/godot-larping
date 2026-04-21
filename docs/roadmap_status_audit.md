# Audit de statut de la roadmap MNMS vers Godot

Date d'audit: 2026-04-21

## Resume executif

Le depot est deja largement au-dela d'un simple prototype `classic`.

Etat confirme dans le code:

- Le runtime Godot C++ sait charger des `MnmsPackResource`, `MnmsLevelResource`, `MnmsThemeResource` et instancier un niveau complet.
- Les blocs officiels majeurs deja presents dans le runtime sont: `Block`, `ShadowBlock`, `MovingBlock`, `MovingShadowBlock`, `Fragile`, `Spikes`, `MovingSpikes`, `ShadowSpikes`, `ConveyorBelt`, `ShadowConveyorBelt`, `Swap`, `Teleporter`, `Switch`, `Button`, `Exit`, `Checkpoint`, `Collectable`, `NotificationBlock`, `Pushable`, `ShadowPushable`.
- Le flux menu principal -> selection pack/niveau -> jeu -> interlevel -> niveau suivant existe deja.
- La progression locale par pack est deja persistee dans `user://mnms_progress.cfg`.
- Le checkpoint restaure deja un etat runtime et pas seulement les positions.
- Premiers correctifs de fidelite deja appliques sur cette passe:
  - `Swap` refuse maintenant un echange si une destination est bloquee pour le corps cible.
  - `Teleporter` refuse maintenant une destination bloquee au lieu de teleporter a travers une case invalide.
  - `Fragile` progresse maintenant par nouvelle entree valide sur le bloc, au lieu de casser sur un timer continu.
  - `Switch` et `Button` distinguent maintenant les blocs "mouvement active/desactive" des blocs vraiment actives/desactives: les `MovingBlock`, `MovingSpikes` et `ConveyorBelt` s'arretent sans devenir traversables.
  - Les `Exit` combines avec un controle externe recalculent maintenant correctement leur etat ouvert/ferme avec la contrainte collectables + signal de controle.
  - Les `Pushable` sont maintenant aussi transportes par les `MovingBlock`/`MovingShadowBlock` et `ConveyorBelt` quand ils reposent dessus.
  - Les `ShadowPushable` suivent maintenant aussi correctement les supports `shadow` pour les appuis et le transport.
  - Une nouvelle passe de polish UI a retire plusieurs libelles encore trop techniques dans le menu, la selection, l'interlevel et les notifications runtime.
  - Le menu pause expose maintenant des actions directes `Reprendre / Recommencer / Checkpoint / Choisir un niveau`, en plus des raccourcis clavier.
  - Le theme de pack ne fuit plus d'un niveau a l'autre quand un override de theme niveau est applique.
  - La musique gameplay est maintenant resolue depuis le `music_list` du pack quand il est renseigne, avec fallback propre sur `default.list`.
  - Le confort replay a gagne un premier vrai lot de commandes runtime: `F5` sauvegarde, `F8` relance le replay courant, `Delete` ou `Backspace` stoppent la lecture en cours, et la pause expose aussi un bouton de sauvegarde.
  - Les retours audio de base couvrent maintenant aussi l'echec et les transitions d'enregistrement/replay.

Gaps majeurs encore confirms:

- Le perimetre runtime courant a ete volontairement recentre sur `classic` uniquement, mais les golden levels de reference n'ont pas encore ete rejoues de facon systematique.
- Le replay existe, mais la parite deterministe avec le jeu de reference n'est pas encore prouvee.
- Le runtime expose maintenant un parcours golden `classic` via `F6/F7`, ce qui rend la validation niveau par niveau beaucoup plus directe.
- `time_limit` et `recordings_count` sont lus et affiches; a la lumiere du code original, ils servent surtout de cibles de medaille / best stats sur `classic`, pas de condition d'echec gameplay directe.
- Les SFX de base sont maintenant branches (`checkpoint`, `collect`, `swap`, `achievement`, `hit`, `toggle`), mais la couverture audio complete ne l'est pas encore.
- La parite visuelle des themes reste partielle.
- L'UI complete type options/aide/credits/stats n'est pas finie.

## Correctif structurel applique pendant cet audit

Le runtime ne relance plus l'import des `.map` / `.lst` au boot du jeu.

Avant cet audit, `MnmsGameDirector::_bootstrap_pipeline()` appelait `MnmsImporter::import_all(...)` a chaque lancement, ce qui contredisait le contrat cible "runtime = ressources converties uniquement".

Desormais:

- le jeu lit seulement `res://mnms_converted` au runtime;
- une scene utilitaire dediee a la conversion hors gameplay a ete ajoutee:
  - `res://tools/rebuild_mnms_converted.tscn`
  - script: `res://tools/rebuild_mnms_converted.gd`
- la source de rebuild actuellement utilisee par le projet pointe sur `res://mnms_source_data`, volontairement limitee au pack `classic`.

## Statut par iteration

### Iteration 1 - Audit de parite jouable et durcissement du socle

Statut: partiellement fait

Deja fait:

- Flux `Importer -> Pack/Level/Theme resources -> GameDirector -> LevelRunner` en place.
- Theme de pack charge depuis les ressources converties.
- Spawn player/shadow, exit, restart, checkpoint, selection pack/niveau et progression locale en place.
- Contrat runtime corrige pendant cet audit pour ne plus parser les sources originales au lancement.

Reste a faire:

- Repasser un lot de golden levels sur `classic`.
- Verifier les details de comportement des blocs deja implantes sur niveaux de reference.

### Iteration 2 - Determinisme Player/Shadow

Statut: partiellement fait

Deja fait:

- `MnmsReplaySystem`, `MnmsShadow`, `MnmsPlayerShadowSystem` existent.
- L'enregistrement se fait en frames d'input sur `_physics_process`.
- Restart, checkpoint et reset shadow existent.
- Le restart preserve maintenant la disponibilite du checkpoint, ce qui colle mieux au comportement du jeu original.
- Les golden levels `classic` sont maintenant atteignables rapidement via `F6/F7` pour les passes de validation.
- Le switch de camera player/shadow existe.

Reste a faire:

- Prouver la non-divergence sur restart/checkpoint/replay.
- Verifier si la boucle actuelle colle a la reference officielle sur tous les cas limites.
- Continuer a verifier la parite de presentation/medaille autour de `time_limit` et `recordings_count`, mais sans les traiter comme une contrainte de fail-state pour `classic` tant que la reference ne montre pas autre chose.

### Iteration 3 - Fermeture des gaps de blocs officiels

Statut: largement avancee mais non close

Deja fait:

- `Pushable`, `ShadowPushable`, `NotificationBlock` sont deja implementes dans `MnmsBlockSystem`.
- Les blocs cites dans la roadmap sont tous presents sous une forme exploitable dans le runtime.
- Les proprietes de comportement restent centralisees dans `MnmsTileResource.properties`.
- Les pushables suivent maintenant mieux les plateformes mouvantes et tapis, ce qui reduit un ecart de fidelite sur les interactions de support.

Reste a faire:

- Valider la fidelite, pas seulement la presence.
- Continuer a durcir en particulier `MovingBlock`, `MovingShadowBlock`, `Switch`, `Button`, `Collectable`, `Checkpoint` et la parite fine des `Exit`.
- Rejouer des cas `ShadowPushable` sur supports `shadow` pour confirmer la fermeture de cet ecart.
- Ajouter des validations niveau par niveau sur les packs officiels.

### Iteration 4 - Progression packs et UI de jeu

Statut: bien avancee mais partielle

Deja fait:

- Menu principal en dur.
- Selection pack/niveau.
- Ecran interlevel.
- Niveau suivant.
- Restart.
- Navigation clavier fonctionnelle.
- Persistance locale de progression.

Reste a faire:

- Remplacer les derniers libelles/debug text encore techniques.
- Continuer le polish UX maintenant que la pause a des actions directes.
- Finition UX et parcours complet comparable au jeu original.

### Iteration 5 - Themes et audio de base

Statut: partiel

Deja fait:

- Conversion de themes `.mnmstheme` vers `MnmsThemeResource`.
- Mapping des textures de blocs et des etats/frames de personnage.
- Application du theme par pack et surcharge eventuelle par niveau.
- Le theme de pack est reapplique proprement a chaque chargement de niveau, meme si un niveau precedent utilisait un override local.
- La musique gameplay lit maintenant le `music_list` du pack quand il existe, avec fallback sur `default.list`.
- SFX de base branches sur les evenements de gameplay clefs et l'interlevel affiche maintenant la medaille visuelle.

Reste a faire:

- Couvrir toute la parite visuelle attendue.
- Importer et brancher la musique et les SFX encore manquants.
- Remplacer les fallback visuels restants par les bons assets/themes.

### Iteration 6 - Replay et confort de jeu

Statut: majoritairement faite

Deja fait:

- Sauvegarde locale de replay ajoutee depuis l'ecran interlevel vers `user://replays`.
- Sauvegarde rapide du replay courant disponible aussi via `F5` et depuis le menu pause.
- Le replay courant peut maintenant etre relance via `F8`, et arrete proprement via `Delete` ou `Backspace`.
- Le bandeau runtime expose maintenant des aides contextuelles selon l'etat `record/replay`.
- Le runtime peut maintenant recharger le dernier replay sauvegarde depuis l'UI.
- Le HUD clarifie mieux `checkpoint -> reprise` vs `pas de checkpoint -> restart`.
- Les transitions replay/restart/checkpoint conservent mieux un etat UI/runtime propre.
- Des options minimales persistantes existent maintenant: musique, SFX, bandeau runtime, notifications.

Reste a faire:

- Continuer a clarifier l'UX checkpoint/replay/restart sur les niveaux de reference.
- Valider humainement les flux `replay charge -> restart -> checkpoint -> niveau suivant`.

### Iteration 7 - Parite UI complete

Statut: largement engagee, non close

Deja fait:

- Menu principal et pause exposent maintenant des ecrans `Options`, `Aide`, `Statistiques` et `Credits`.
- Les controles principaux sont reconfigurables depuis l'UI, avec persistance locale.
- Les statistiques locales visibles couvrent progression, replay, checkpoints, swaps, collectables, echecs et temps de jeu.
- Une premiere couche d'achievements locaux visibles est maintenant reliee a ces statistiques.
- Le perimetre reste volontairement limite au runtime de jeu, sans retour de l'editeur dans le scope.

Reste a faire:

- Continuer a enrichir la presentation visuelle de ces ecrans pour se rapprocher encore du jeu d'origine.
- Elargir la couverture achievements/statistiques si la parite gameplay continue de se stabiliser.

### Iteration 8 - Phase dediee editeur

Statut: non commencee

Conforme a la roadmap: a garder hors perimetre tant que gameplay + UI officielles ne sont pas valides.

## Points de verification concrets encore ouverts

- `mnms_converted/packs` ne garde plus que `classic_pack.tres` dans le runtime actuel.
- `mnms_converted/levels` ne garde plus que `classic`.
- `source_data_path` pointe sur `mnms_source_data`, qui ne contient que `classic` pour les levelpacks du projet courant.
- Les donnees officielles completes existent toujours sous `res://meandmyshadow-master/meandmyshadow-master/data`, en lecture seule, comme reference de fidelite.
- Le code source original local confirme que `time_limit` / `recordings_count` alimentent surtout medailles, best stats et achievements, pas une logique de sanction gameplay immediate sur `classic`.

## Prochaine livraison recommandee

1. Rejouer les golden levels de `docs/iteration1_audit.md`, en priorite ceux qui couvrent `Switch`/`Button`, moving blocks et conveyors.
2. Verifier ensuite les cas `checkpoint -> restart -> replay` sur ces memes niveaux.
3. Utiliser `docs/classic_validation_matrix.md` comme protocole de fermeture avant de poursuivre l'UI ou l'audio.
