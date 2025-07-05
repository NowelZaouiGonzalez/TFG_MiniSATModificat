# RandomGeneratorNonogram.py

Aquest script en **Python** permet generar **nonogrames aleatoris** donant la mida i la densitat desitjada. Els nonogrames generats s’emmagatzemen automàticament en fitxers `.txt`.

## Què fa?

- Genera una graella de mides personalitzables amb cel·les pintades aleatòriament segons una **densitat** donada.
- Calcula automàticament les **pistes** corresponents a cada fila i columna.
- Desa el nonograma generat a un fitxer `.txt` dins d'una carpeta anomenada `RandomNonograms/`, amb un nom únic.

## Requisits

Aquest script no depèn de cap biblioteca externa. Només cal tenir Python instal·lat.

## Ús

### Executa des de terminal:

```
python RandomGeneratorNonogram.py -f <files> -c <columnes> -d <densitat>
```

### Paràmetres:

- `-f`, `--files`: Nombre de **files** del nonograma (enter, requerit)
- `-c`, `--columnes`: Nombre de **columnes** del nonograma (enter, requerit)
- `-d`, `--densitat`: Percentatge de cel·les pintades (float entre 0 i 100, requerit)

### Exemple:

```
python RandomGeneratorNonogram.py -f 10 -c 15 -d 35.0
```

Això generarà un nonograma de 10x15 amb aproximadament un 35% de cel·les pintades.

## Sortida

- Es crea (si no existeix) la carpeta `RandomNonograms/`
- S'hi afegeix un fitxer `.txt` per cada nonograma generat, amb nom del format:
  ```
  RNDNNGR_<files>x<columnes>_<número>.txt
  ```

- El fitxer conté:
  - La mida del nonograma
  - Les pistes de cada fila (una per línia)
  - Les pistes de cada columna (una per línia)
  - Un comentari al final indicant la densitat real utilitzada

### Exemple de nom de fitxer:
```
RNDNNGR_10x15_01.txt
```

Si s’executa diverses vegades, es generaran fitxers nous amb números incrementals automàticament (…_02.txt, …_03.txt, etc.) si tenen la mateixa mida.

## 📝 Notes

- La densitat real pot variar lleugerament, ja que es calcula a partir del nombre total de cel·les pintades senceres.
