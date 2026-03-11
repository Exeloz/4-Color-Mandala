# Format de mandala_progress.dat

Ce fichier est un snapshot texte de la progression joueur.

Version actuelle ecrite par le jeu: `MANDALA_PROGRESS_V2` (format compact/sparse).
Compatibilite lecture: `MANDALA_PROGRESS_V1` et `MANDALA_PROGRESS_V2`.

## Structure

Ordre attendu:

1. `MANDALA_PROGRESS_V2` (ou anciennement `MANDALA_PROGRESS_V1`)
2. `PALETTE <N>`
3. `N` lignes RGBA: `<r> <g> <b> <a>`
4. `MANDALAS <M>`
5. Pour chaque entree mandala/session:
  - `MANDALA <mandalaKey> <completedFlag> <regionCount> <frozenPaletteCount>`
  - `frozenPaletteCount` lignes RGBA: `<r> <g> <b> <a>`
   - `regionCount` lignes: `REGION <regionId> <colorIndex>`

## Signification des champs

- `completedFlag`: `0` = non complete, `1` = complete.
- `mandalaKey`:
  - Mandala normal: id numerique (`"1"`, `"2"`, ...)
  - Mandala hard: id suffixe `H` (`"1H"`, `"2H"`, ...)
  - Session Daily (copie): cle temporaire datee `R_<year>_<month>_<day>_<mandalaId>`
    - Exemple: `R_2026_3_11_2`
    - Cette entree sauvegarde la progression Daily sans ecraser la progression du mandala normal.
- `colorIndex`:
  - `-1` = region non coloriee
  - `>= 0` = index dans la palette active (celle de la section `PALETTE`)
- `frozenPaletteCount`:
  - `0` = mandala non fige
  - `> 0` = palette figee du mandala termine (conserve les couleurs de completion)
- `regionCount` (V2): nombre de regions effectivement sauvegardees (format sparse)
  - En V2, seules les regions avec `colorIndex >= 0` sont ecrites.
  - Les regions absentes sont considerees `-1` (non coloriees) au chargement.

## Exemple minimal

```text
MANDALA_PROGRESS_V2
PALETTE 2
255 255 255 255
65 105 225 255
MANDALAS 2
MANDALA 1 0 2 0
REGION 1 1
REGION 2 0
MANDALA R_2026_3_11_1 0 1 0
REGION 0 1
```

## Notes utiles

- Le parseur est strict sur les tokens et l'ordre.
- Les canaux RGBA sont clamps entre `0` et `255` au chargement.
- Si le fichier est invalide/incomplet, le chargement echoue et la progression est ignoree.
- Compatibilite: les anciens fichiers sans `frozenPaletteCount` sont acceptes.
- `mandalaKey` n'est pas limite a un entier: le parseur accepte aussi les cles de session (ex: Daily).
