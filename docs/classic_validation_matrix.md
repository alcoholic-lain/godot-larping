# Validation Matrix Classic

Date: 2026-04-21

Perimetre: pack `classic` uniquement.

Objectif: fermer les iterations 1 a 3 sur les niveaux de reference sans repartir sur les autres packs.

## Raccourcis de validation

- `F6`: niveau golden precedent
- `F7`: niveau golden suivant
- `R`: recommencer le niveau courant
- `L`: charger le checkpoint courant, sinon recommencer
- `Espace`: demarrer / terminer un enregistrement
- `Backspace`: annuler l'enregistrement en cours
- `Delete`: arreter le replay en cours
- `F5`: sauvegarder le replay courant
- `F8`: relancer le replay courant depuis le debut
- `Tab`: basculer la vue sur le shadow

Le bandeau de statut affiche maintenant la position dans le parcours golden `classic`.

## Parcours golden

1. `classic/0_babysteps.tres`
   Attendu: chargement propre, spawn correct, sortie, interlevel.
2. `classic/1_shadowblocks.tres`
   Attendu: premier cycle record/replay sans divergence evidente.
3. `classic/8_control.tres`
   Attendu: boucle record -> replay -> sortie stable.
4. `classic/10_jumping.tres`
   Attendu: collisions, saut, spikes, restart fiable.
5. `classic/11_updown.tres`
   Attendu: checkpoint, restart, load checkpoint, reprise cohérente.
6. `classic/15_timing.tres`
   Attendu: timing, recordings cibles, replay stable.
7. `classic/20_shadow.tres`
   Attendu: stress simple du comportement shadow.
8. `classic/22_end.tres`
   Attendu: fin de pack, interlevel final, progression.

## Protocole iteration 2

Pour chaque niveau golden:

1. Charger le niveau avec `F6/F7`.
2. Jouer un premier essai sans checkpoint.
3. Rejouer avec au moins un cycle `Espace -> Espace`.
4. Tester `R` pendant un etat normal.
5. Si le niveau contient un checkpoint, tester `checkpoint -> R -> L`.
6. Si le shadow est actif, tester `Tab` pendant replay et hors replay.
7. Verifier que le bandeau runtime reste coherent:
   - temps
   - enregistrements
   - presence checkpoint
   - etat replay

## Critere de fermeture

- Iteration 1:
  Tous les golden levels `classic` se chargent et se terminent sans ressource manquante ni blocage evident.
- Iteration 2:
  Aucun cas `checkpoint/restart/replay` ne diverge visiblement sur le parcours golden.
- Iteration 3:
  Les niveaux golden couvrant collisions, replay, checkpoints, spikes et comportements relies aux blocs deja portes ne montrent plus d'ecart bloquant.

## Notes de reference

- Le restart preserve maintenant le checkpoint disponible, conformement au comportement observe dans le code original.
- `time_limit` et `recordings_count` servent ici de cibles de medaille / statistiques, pas de fail-state gameplay direct sur `classic`.
