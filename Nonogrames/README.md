# 🗂️ InformacioNonogrames.zip

Aquest arxiu ZIP conté informació estructurada dels **nonogrames** utilitzats en aquest treball, dividits en dues categories principals: nonogrames aleatoris i nonogrames descarregats.

## 📁 Contingut de l’arxiu

### 📂 Nonogrames Aleatoris

Conté dues subcarpetes:

- **Total Generats**: Inclou **tots** els nonogrames generats durant el treball.
- **Asequibles**: Subconjunt dels "Total Generats". Són els seleccionats per a les proves, complint que el MiniSAT original amb ENC3 triga entre **20 segons i 15 minuts** d'execució.

Els fitxers tenen el nom:
```
RNDNNGR_<Files>x<Columnes>_<Número>.txt
```

### 📂 Nonogrames Descarregats

Inclou els nonogrames extrets del web [nonograms.org](https://www.nonograms.org/nonograms). També conté la carpeta:

- **Solucions**: Imatges `.png` de la solució de cada nonograma descarregat.

Els fitxers tenen el nom:
```
NNGR_<Files>x<Columnes>_<Número>.txt
```

## 📄 Format dels Fitxers `.txt`

Tots els nonogrames (tant aleatoris com descarregats) segueixen el següent format:

1. **Primera línia**: dos números enters `F C` indicant les files i columnes del nonograma.
2. **Següents F línies**: pistes de cada fila.
3. **Següents C línies**: pistes de cada columna.

### ➤ Interpretació de les pistes:

- Una pista com `0` indica que la fila o columna **és completament buida**.
- Una pista com `-1` (o altres nombres negatius) indica que **no es proporciona la pista** per aquella fila o columna (valor ocult).
- Les altres pistes són llistes de blocs consecutius a pintar, separats per espais.

### Exemple:
```
5 5
2 1
1 1
0
-1
3
1
0
2
1 1
1
```

## 📝 Comentaris finals dins dels fitxers

- **Nonogrames Aleatoris**: Afegit al final:
  ```
  c Aquest nonograma s'ha generat amb una densitat de <d>%
  ```
  on `<d>` és la densitat real utilitzada per pintar cel·les.

- **Nonogrames Descarregats**: Afegit al final:
  ```
  c https://www.nonograms.org/nonograms/i/<id>
  ```
  on `<id>` és el identificador unic que te el nonograma a la pagina web.
  
  Enllaç a la pàgina d'origen del nonograma.