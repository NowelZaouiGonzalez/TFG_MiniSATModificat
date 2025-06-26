# TFG_ModifiedMinisat

Aquest repositori conté els components desenvolupats per al Treball de Fi de Grau (TFG), relacionats amb la modificació de MiniSAT i la seva aplicació a la resolució de nonogrames.

## Contingut

- **MiniSAT modificat**  
  Codi del MiniSAT amb modificacions per analitzar clàusules binàries i ternàries durant la fase de simplificació i cerca.

- **Descarregador de nonogrames**  
  Script en Python que permet descarregar la informació de nonogrames des de [nonograms.org](https://www.nonograms.org/nonograms).

- **Generador de nonogrames aleatoris**  
  Script en Python que genera la informació de nonogrames resolubles amb la mida i densitat especificades.

- **Codificador de nonogrames**  
  Script en Python que codifica nonogrames a SAT segons una de les codificacions disponibles: `ENC1`, `ENC2`, `ENC3` o `ENC3V2`.

- **Informació dels nonogrames**
  - **Nonogrames**: informació descarregada de nonograms.org. Inclou una subcarpeta amb les solucions.
  - **Nonogrames Aleatoris**: informació generada aleatòriament.
  - **Nonogrames Aleatoris Assequibles**: subconjunt dels nonogrames aleatoris seleccionats per obtenir els resultats. Inclòs per referència fàcil.

- **Encodings (codificació SAT)**
  - **Nonogrames**: codificats amb `ENC2`, `ENC3`, `ENC3V2`.
  - **Nonogrames Aleatoris**: codificats amb `ENC3`, `ENC3V2`.
  - **Nonogrames Aleatoris Assequibles**: codificats amb `ENC3`, `ENC3V2`.

- **Resultats**  
  Resultats obtinguts en executar MiniSAT amb les diferents modificacions activades sobre les diverses codificacions.



