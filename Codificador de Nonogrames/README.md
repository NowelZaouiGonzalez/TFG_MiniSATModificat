
# Codificador de Nonogrames

Aquest directori conté un script en Python per generar codificacions SAT de Nonogrames en format DIMACS. El programa permet llegir un arxiu de Nonograma i generar la seva codificació segons diferents modes.

## 📥 Lectura del Nonograma

El programa llegeix la informació d'un Nonograma des d'un fitxer `.txt` especificat amb el paràmetre:

```
-i nom_nonograma.txt
```

El nom del Nonograma es considerarà el mateix que el del fitxer, però sense l’extensió `.txt`.

## ⚙️ Selecció del mode de codificació

Es pot seleccionar el mode de codificació amb l'opció `-m`:

```
-m 1         # Codificació ENC1
-m 2         # Codificació ENC2
-m 3         # Codificació ENC3
-m 3 -v2     # Codificació ENC3V2
```

## 🧩 Generació del fitxer DIMACS

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

## 🗂️ Fitxer auxiliar de tauler

Juntament amb el fitxer DIMACS, es genera també un fitxer:

```
tauler_{nom_codificació}
```

Aquest fitxer conté:
- A la primera línia: dues enters que indiquen el nombre de files (`F`) i columnes (`C`).
- A continuació: `F` línies amb `C` valors cadascuna, que representen les variables SAT associades a cada cel·la del Nonograma.

Aquest fitxer és útil per interpretar la solució SAT i reconstruir la solució visual del Nonograma.

---

## 🔧 Exemple d’ús

```bash
python3 generatorCNF_Nonogrames.py -i exemples/nonograma01.txt -m 2
```

Això generarà:
- El fitxer DIMACS dins de `Encodings/ENC2_Eficient/`
- El fitxer de tauler corresponent

---

## 📦 Requisits

- Python 3.12.3
- [pysat](https://pypi.org/project/python-sat/):  
  Instal·lació amb pip:

```bash
pip install python-sat
```


---

## ✍️ Autor

Nowel Zaoui Gonzalez
