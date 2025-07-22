# Altres codis

## `scriptFerResultats.sh`

Aquest script automatitza l'execució de MiniSat sobre un conjunt d'arxius `.dimacs`, extraient i guardant informació rellevant dels resultats.

---

### **Què fa?**

- Executa MiniSat sobre tots els fitxers `.dimacs` d’un directori especificat.
- Aplica un **timeout** per limitar el temps d’execució de cada problema.
- Llegeix els resultats generats per MiniSat, obtenint estadístiques com:
  - Restarts
  - Conflictes
  - Decisions
  - Propagacions
  - Conflict literals
  - Memòria utilitzada
  - Temps de CPU
  - Resultat final (SAT, UNSAT, INDETERMINATE o ERR)
- Guarda:
  - La solució (opcionalment) a `results/Solutions/`.
  - Informació de reducció si existeix un arxiu `reduccio.txt`.
  - Un arxiu CSV `results/resultats.csv` amb tots els resultats de manera ordenada.
  - Un petit `README` dins la carpeta `results` amb l’executable utilitzat i els paràmetres de timeout/memòria.

---

### **Com s'utilitza?**

| Opció | Descripció |
|-------|------------|
| `-d <directori>` | Directori amb els fitxers `.dimacs` (per defecte `.`). |
| `-t <timeout>` | Temps màxim d’execució per fitxer en segons (per defecte 3600 segons, és a dir, 1 hora). |
| `-e <executable>` | Executable de MiniSat a utilitzar (per defecte `minisat`). |
| `-s` | Si s’especifica, es guarda la solució en la carpeta `results/Solutions/`. |


### **Exemple d'execució:**

```bash
bash scriptFerResultats.sh -d ./problemes -t 1800 -e minisat_original -s
```
