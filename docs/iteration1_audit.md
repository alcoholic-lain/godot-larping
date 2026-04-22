# Audit Iteration 1

Pack audite: `classic`

Le runtime Godot charge aujourd'hui des ressources converties `.tres` uniquement. L'audit ci-dessous fige le sous-ensemble de niveaux `classic` a rejouer en boucle pour durcir le socle et verifier la fidelite gameplay sur le perimetre actuel du projet.

## Golden levels proposes

- `classic/0_babysteps.tres`: smoke test de chargement, spawn, sortie et interlevel.
- `classic/1_shadowblocks.tres`: base `Player/Shadow`, shadow blocks, premier usage clair du record/replay.
- `classic/8_control.tres`: verifie la boucle record -> replay -> sortie dans un niveau plus structure.
- `classic/10_jumping.tres`: collisions de base, saut et `Spikes`.
- `classic/11_updown.tres`: checkpoint, restart, load checkpoint et coherence de reprise.
- `classic/15_timing.tres`: niveau de reference pour timing, replay et recordings cibles.
- `classic/20_shadow.tres`: stress simple sur le comportement de l'ombre en phase avancee du pack.
- `classic/22_end.tres`: verifie progression de fin de pack et ecran interlevel final.

## Axes de validation

Raccourcis utiles pendant l'audit:

- `F6/F7`: naviguer entre les golden levels `classic`
- `R`: recommencer
- `L`: charger le checkpoint courant ou recommencer
- `Espace`: enregistrer / lancer le replay
- `Backspace`: annuler l'enregistrement en cours

- Chargement:
  - le niveau s'ouvre sans crash ni ressource manquante
  - le theme et les sprites de personnages se chargent proprement
- Gameplay:
  - spawn `Player` / `Shadow` correct
  - collisions et sortie coherentes
  - `R` restart et `L` checkpoint/load fiables
- Replay:
  - `Espace` demarre puis termine l'enregistrement
  - le shadow rejoue sans divergence evidente
  - `Backspace` annule l'enregistrement en cours proprement
- Progression:
  - best time / best recordings / medaille restent coherents
  - l'interlevel affiche bien le resultat attendu

## Note de reference sur les objectifs

Dans le code original local de Me and My Shadow, `time_limit` et `recordings_count` servent sur le pack `classic` de cibles de medaille, d'affichage et de best stats; ils ne sont pas traites comme des conditions d'echec gameplay directes. Pour cette raison, les playthroughs de validation doivent surtout verifier la coherence des stats, de l'interlevel et de la medaille, pas un game over automatique quand la cible est depassee.

## Conclusion

Pour le runtime actuellement livre, l'iteration 1 doit maintenant se fermer par des validations manuelles sur ces niveaux `classic`, puis par correction des derniers ecarts de feeling ou de determinisme observes pendant ces playthroughs.
