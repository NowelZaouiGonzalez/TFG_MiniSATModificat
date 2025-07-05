
# Codificador de Nonogrames

Aquest directori conté un script en Python per generar codificacions SAT de Nonogrames en format DIMACS. El programa permet llegir un arxiu de Nonograma i generar la seva codificació segons diferents modes.

## Lectura del Nonograma

El programa llegeix la informació d'un Nonograma des d'un fitxer `.txt` especificat amb el paràmetre:

```
-i nom_nonograma.txt
```

El **nom del Nonograma** es considerarà el mateix que el del fitxer, però sense l’extensió `.txt`.

### 🔄 Si no s'especifica el paràmetre `-i`:

Si no s'indica cap fitxer amb `-i nom_nonograma.txt`, el programa demanarà introduir **manualment des del teclat**:

1. La primera línia amb els valors `F C`.
2. Les següents `F + C` línies amb les pistes de files i columnes.

En aquest cas, el Nonograma assumirà per defecte el **nom `NONOGRAMA_BASIC`**.

---

### Format de la informació del Nonograma:

- **Primera línia:** dos valors enters separats per espai, `F C`, on  
  - `F` és el nombre de files  
  - `C` és el nombre de columnes

- **Les següents F + C línies:** contenen les pistes del Nonograma.
  - Les primeres `F` línies corresponen a les **pistes de cada fila**.
  - Les següents `C` línies corresponen a les **pistes de cada columna**.

- **Les línies posteriors (opcional):** es poden utilitzar per escriure comentaris o notes al fitxer.

---

### Regles de les pistes:

- Si una fila o columna té més d’un bloc com a pista, **tots els valors han de ser positius** i indicaran la mida de cada bloc en ordre.

- Si no hi ha cap bloc a la fila o columna, utilitza el **valor `0`** per indicar que és buida.

- Si no es vol especificar la pista d’una fila o columna (per exemple, en casos d’ambigüitat), utilitza un **valor negatiu**. Qualsevol valor menor que 0 s’interpretarà com a “pista no especificada”.

---

## Exemple de contingut d’un fitxer de Nonograma

```
5 5
2
1 1
3
1
0
-1
2
3
2
1 1
0

comentari: -1 per indicar ambigüetat
```

---

**Resum funcional:**

- Amb `-i`: llegeix el fitxer especificat.  
- Sense `-i`: l’usuari introdueix manualment i el nom per defecte serà `NONOGRAMA_BASIC`.


## Selecció del mode de codificació

Es pot seleccionar el mode de codificació amb l'opció `-m`:

```
-m 1         # Codificació ENC1
-m 2         # Codificació ENC2
-m 3         # Codificació ENC3
-m 3 -v2     # Codificació ENC3V2
```

## Generació del fitxer DIMACS

El programa genera un arxiu DIMACS amb la codificació SAT del Nonograma. El fitxer es desa amb el nom:

```
ENC{mode}_{nom_nonograma}
```

Aquest fitxer es desa dins la carpeta `Encodings/` amb subcarpetes segons el mode:

- `ENC1_NoEficient`
- `ENC2_Eficient`
- `ENC3_EficientiEnriquit`
- `ENC3_EficientiEnriquitV2`

> ⚠️ Aquesta funcionalitat utilitza la llibreria [`pysat`](https://pysathq.github.io/), que ha d’estar instal·lada prèviament.

## Fitxer auxiliar de tauler

Juntament amb el fitxer DIMACS, es genera també un fitxer:

```
tauler_{nom_codificació}.txt
```

Aquest fitxer conté:
- A la primera línia: dues enters que indiquen el nombre de files (`F`) i columnes (`C`).
- A continuació: `F` línies amb `C` valors cadascuna, que representen les variables SAT associades a cada cel·la del Nonograma.

Aquest fitxer és útil per interpretar la solució SAT i reconstruir la solució visual del Nonograma.

---

## Exemple d’ús

```bash
python3 generatorCNF_Nonogrames.py -i exemples/nonograma01.txt -m 2
```

Això generarà:
- El fitxer DIMACS dins de `Encodings/ENC2_Eficient/`
- El fitxer sera ENC2_nonograma01.dimacs
- El fitxer de tauler corresponent es trobara a `Encodings/ENC2_Eficient/Taulers/`
- El fitxer del tauler es tauler_ENC2_nonograma01.txt

---

## Requisits

- Python 3.12.3
- [pysat](https://pypi.org/project/python-sat/):  
  Instal·lació amb pip:

```bash
pip install python-sat
```


---

## ✍️ Autor

Nowel Zaoui Gonzalez
