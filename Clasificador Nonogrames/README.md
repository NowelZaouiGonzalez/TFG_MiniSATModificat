# Classificador Nonogrames

Aquest directori conté les dades i scripts principals per l’anàlisi i classificació de nonogrames mitjançant models de machine learning.

## Carpetes

- **CSV**  
  Carpeta amb fitxers CSV (son els mateixos que trobaràs a Resultats)  
  Utilitzada com a font de dades per als scripts d’entrenament i anàlisi.

- **ResultatsBetterorWorst**  
  Carpeta on s’emmagatzemen els resultats que indiquen en quins aspectes un executable millora o empitjora.  
  Conté CSVs amb mètriques comparatives i informes d’anàlisi.

- **ResultatsImportancies**  
  Carpeta que conté els resultats de la importància de les característiques extretes pels models (Random Forest, arbres de decisió).  
  També inclou possibles regles i explicacions generades per l’anàlisi.

## Scripts Python

- **RandomForestPerSabaerEnQueMillorenOEmpitjoren.py**  
  Script que utilitza un model de Random Forest (o opcionalment un arbre de decisió) per identificar quins factors o característiques dels nonogrames influeixen en les millores o empitjoraments detectats per diferents executables.  
  Processa fitxers CSV amb resultats classificats per mètrica (millora o empitjorament) i genera fitxers CSV amb la importància relativa de cada característica, ajudant a entendre quins atributs són més rellevants.  
  L’entrada principal és un directori amb subcarpetes `with_<executable>` que contenen fitxers CSV de comparació, i un fitxer d’informació dels nonogrames (`infoNonogrames.csv`) amb característiques descriptives.

  · **Ús i requisits de dades:**  
    - Carpeta amb subcarpetes `with_<executable>`, cadascuna amb fitxers CSV que indiquen millores (`Better_in_<mètrica>.csv`), empitjoraments (`Worst_in_<mètrica>.csv`) i opcionalment `Is_IND.csv`.  
    - Fitxer CSV d’informació dels nonogrames (`infoNonogrames.csv`), amb característiques descriptives de cada nonograma.

  · **Execució:**  
    ```bash
    python RandomForestPerSabaerEnQueMillorenOEmpitjoren.py -d <directori> [-i <info_csv>] [--decision_tree]
    ```

  · **Paràmetres d'entrada:**  
    - `-d`  
      Directori base que conté les subcarpetes `with_<executable>`. Aquestes subcarpetes inclouen els fitxers CSV amb les dades d'anàlisi.

    - `-i` (opcional)  
      Fitxer CSV amb la informació descriptiva dels nonogrames. Per defecte, `infoNonogrames.csv`.

    - `--decision_tree` (opcional)  
      Si es passa aquesta opció, s'utilitza un arbre de decisió en lloc d'un Random Forest per l'entrenament i anàlisi.

  · **Sortida:**  
    Es genera una estructura de carpetes amb els resultats organitzats per executable i mètrica. Cada carpeta conté fitxers CSV amb la importància de les característiques. En cas d'utilitzar arbre de decisió, també s'exporta un fitxer de text amb les regles del model.





- **CodiSaberInfoCasellesNonogrames.py**  
  Script que processa fitxers Nonogrames `.txt` amb les pistes dels nonogrames per calcular mètriques per al classificador.  
  Genera un CSV resum amb dades com el percentatge de caselles pintades, mida i desviació de les pistes, i la llibertat relativa.  
  Els fitxers `infoNonogramesDescarregats.csv` i `infoNonogramesRandom.csv` són exemples de resultats generats per aquest script.  

- **SaberAmbQuinsAMillorat_Empitjorat.py**  
  Script que compara resultats CSV entre una carpeta base i altres carpetes per detectar millores i empitjoraments en diferents mètriques (Restarts, Conflicts, Decisions, CPU_Time).

  **Estructura esperada de la carpeta d’entrada:**
    ```plaintext
    directori/
    ├── minisat000/                  # carpeta base (per defecte)
    │   └── res_minisat000.csv       # CSV base
    ├── with_executable1/
    │   └── res_with_executable1.csv # CSV a comparar
    ├── with_executable2/
    │   └── res_with_executable2.csv
    └── ...
    ```
  **Ús bàsic:**
```bash
    python SaberAmbQuinsAMillorat_Empitjorat.py -d <directori> [-b <carpeta_base>]
```
**Sortida:**  
Carpeta `nonogramesMillors_<nom_directori>/with_<nom_carpeta>/` amb fitxers CSV indicant millores i empitjoraments per mètrica.



---

Aquest README explica l’estructura bàsica i funcionalitat principal del directori per facilitar-ne la comprensió i ús.
