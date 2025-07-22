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
  Script que utilitza un model Random Forest per identificar en quins aspectes els executables milloren o empitjoren.  
  Genera resultats d’importància de característiques i exporta fitxers CSV amb les dades analitzades.

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
