# MiniSAT Modificat

Aquest directori conté el codi de **MiniSAT modificat** utilitzat en aquest treball. La base correspon al **MiniSAT original**, amb la implementació i afegits per l’anàlisi de rendiment i les estratègies d’optimització.

## Com utilitzar-ho
Mirar el README dins de la carpeta. Es el mateix README que al minisat original i es pot compilar i executar de la mateixa manera.

## Opcions afegides implementades

| Opció | Tipus | Descripció | Valor per defecte |
|---|---|---|---|
| `-init-pol` | Int (0-2) | Controla el valor inicial de la **polaritat dels literals**:<br>0=Per defecte, 1=True, 2=False | 0 |
| `-ini-seed` | Int | Valor inicial per a la **seed de la funció doIT** (aleatorietat) | 23061912 |
| `-aBinaries` | Bool | Activa l’**anàlisi de clàusules binaries** | false |
| `-aTernaries` | Bool | Activa l’**anàlisi de clàusules ternàries**.<br>Activa també l’anàlisi de binaries. | false |
| `-see-impl` | Bool | Realitza la **cerca d’implicacions**. També activa anàlisi de binaries.<br>Si no s’especifica quantitat, es fa només una vegada. | false |
| `-opt-impl` | Bool | Optimitza l’**estructura de dades de les implicacions de les binaries** | false |
| `-ncerc` | String | Indica **cada quantes vegades es fa la cerca d’implicacions**:<br>`O`=One Time, `A`=Always, o un número enter especificant el nombre de vegades. | "" |
| `-e-er` | Bool | Activa l’estratègia **ER (Evitar Restarts / Recordar Unàries)** | false |
| `-pe-er` | Int (0-100) | Percentatge desitjat per l’estratègia **ER**. Assignar valor >0 activa l’estratègia. Si només s’activa ER sense valor, per defecte és 100. | 0 |
| `-r-ac` | Bool | Activa l’estratègia **AC (Evitar retrocés excessiu/ Recordar binàreis AC)** | false |
| `-pr-ac` | Int (0-100) | Percentatge desitjat per l’estratègia **AC**. Assignar valor >0 activa l’estratègia. Si només s’activa AC sense valor, per defecte és 100. | 0 |
| `-mirar-reduccio` | Bool | Mostra la **reducció de la base de dades** després de la primera simplificació. | false |

---

