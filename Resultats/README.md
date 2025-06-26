# Resultats

Aquesta carpeta conté els resultats obtinguts durant l'execució de les codificacions dels Nonogrames. Degut a la mida dels arxius, s'han mogut a un repositori extern.

Pots descarregar-los des de Google Drive:  
[Resultats - Google Drive](https://drive.google.com/drive/folders/11iwLnrAlJhg3BqRHSZdIgZPv-z4yVBDI?usp=sharing)

## Contingut

Hi trobaràs dos arxius `.zip`, cadascun corresponent a diferents conjunts de Nonogrames:

- **Nonogrames Descarregats**  
  Conté els resultats obtinguts amb les configuracions del MiniSAT modificat utilitzant els encodings **ENC2**, **ENC3** i **ENC3V2**.

- **Nonogrames Aleatoris Assequibles**  
  Conté els resultats obtinguts amb MiniSAT modificat amb els encodings **ENC3** i **ENC3V2**.

Dins de cada `.zip`, hi ha una estructura de carpetes per a cada encoding utilitzat. Dins de cada carpeta d'encoding, trobaràs subcarpetes corresponents a diferents configuracions de MiniSAT.

### Dins de cada subcarpeta de configuració:

- **reduccions/**  
  Conté, per a cada nonograma, les estadístiques de reducció després de la primera simplificació, indicant quantes clàusules **binaries** i **ternàries** queden per a la cerca.

- **solucions/**  
  Conté, per a cada nonograma, la **solució** trobada per MiniSAT.

- **Fitxers `.txt`**  
  Per a cada problema, la **sortida completa per pantalla** generada per MiniSAT, incloent estadístiques i informació de diagnòstic.

- **res_(*nomSubcarpeta*).csv**  
  Resum general per a cada problema, amb les següents columnes:

  | Restarts | Conflicts | Decisions | Propagations | Conflict Literals | Memory Used | CPU Time | Execution Time | Result |
  |----------|-----------|-----------|---------------|--------------------|--------------|-----------|------------------|--------|

  - `Result` pot ser **SAT**, **UNS** o **IND**.

-  **info_(*nomSubcarpeta*).csv**  
  Informació extra generada per la versió modificada de MiniSAT, com:

  - Nombre de clàusules **unàries**, **binàries** i **ternàries** descobertes durant la cerca.
  - Quantitat de clàusules **noves** generades, agrupades segons la **mida**.

---

