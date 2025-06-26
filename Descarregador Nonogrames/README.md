# descarregador_nonogrames.py

Aquest script en **Python** permet descarregar automàticament Nonogrames des del portal [nonograms.org](https://www.nonograms.org/nonograms), així com la seva solució.

## Ús

Per executar-lo, cal proporcionar l'enllaç del Nonograma que es vol descarregar mitjançant el paràmetre `-l` o `--link`.

### Exemple:
python descarregador_nonogrames.py -l https://www.nonograms.org/nonograms/i/12345

## ⚙️ Requisits

Abans d'executar-lo, assegura't de tenir:

1. **La biblioteca `selenium` instal·lada**:
   python -m pip install selenium

2. **El fitxer `chromedriver`**:
   - Ha de ser de la **mateixa versió** que el teu navegador **Google Chrome** instal·lat.
   - Ha d’estar col·locat a la **mateixa carpeta** on es troba aquest script.

Pots descarregar el `chromedriver` des d’aquí:  
https://sites.google.com/chromium.org/driver/

## Sortida

Quan s’executa l’script:

- Es crea (si no existeix) una **carpeta principal** anomenada `Nonogrames`.
- Per **cada Nonograma descarregat**, s’hi genera un **fitxer `.txt`** amb el format:

  NNGR_<files>x<columnes>_<número>.txt

  Aquest fitxer conté la informació del Nonograma: la seva mida i les pistes corresponents.

- També es genera la imatge de la solució (`.png`) dins de la subcarpeta:

  Nonogrames/Solutions/SOLUTION_NNGR_<files>x<columnes>_<número>.png

## Execucions múltiples

Cada cop que s’executa l’script amb un nou enllaç:

- S’afegeix **un nou fitxer** `.txt` dins la carpeta `Nonogrames`.
- La numeració s’incrementa automàticament si ja existeix un fitxer amb el mateix nom base.
- D’aquesta manera, pots acumular diversos Nonogrames i les seves solucions en un **únic lloc organitzat**.

