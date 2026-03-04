# Format de mandala_progress.dat

Ce fichier est un snapshot texte de la progression joueur.

## Structure

Ordre attendu:

1. `MANDALA_PROGRESS_V1`
2. `PALETTE <N>`
3. `N` lignes RGBA: `<r> <g> <b> <a>`
4. `MANDALAS <M>`
5. Pour chaque mandala:
  - `MANDALA <mandalaId> <completedFlag> <regionCount> <frozenPaletteCount>`
  - `frozenPaletteCount` lignes RGBA: `<r> <g> <b> <a>`
   - `regionCount` lignes: `REGION <regionId> <colorIndex>`

## Signification des champs

- `completedFlag`: `0` = non complete, `1` = complete.
- `colorIndex`:
  - `-1` = region non coloriee
  - `>= 0` = index dans la palette active (celle de la section `PALETTE`)
- `frozenPaletteCount`:
  - `0` = mandala non fige
  - `> 0` = palette figee du mandala termine (conserve les couleurs de completion)

## Exemple minimal

```text
MANDALA_PROGRESS_V1
PALETTE 2
255 255 255 255
65 105 225 255
MANDALAS 1
MANDALA 1 0 3 0
REGION 0 -1
REGION 1 1
REGION 2 0
```

## Notes utiles

- Le parseur est strict sur les tokens et l'ordre.
- Les canaux RGBA sont clamps entre `0` et `255` au chargement.
- Si le fichier est invalide/incomplet, le chargement echoue et la progression est ignoree.
- Compatibilite: les anciens fichiers sans `frozenPaletteCount` sont acceptes.
