import argparse
import os
import random

# Funció per generar un nonograma aleatori
def generar_nonograma(files, columnes, densitat):
    densitat = min(densitat, 100.0) / 100.0  # Normalitzar a [0, 1]
    total_celes = files * columnes
    cel_pintades = int(total_celes * densitat)

    matriu = [[0 for _ in range(columnes)] for _ in range(files)]
    posicions = [(i, j) for i in range(files) for j in range(columnes)]
    random.shuffle(posicions)

    for i, j in posicions[:cel_pintades]:
        matriu[i][j] = 1

    # Funció per obtenir pistes d’una fila o columna
    def obtenir_pistes_linia(linia):
        pistes = []
        comptador = 0
        for cela in linia:
            if cela == 1:
                comptador += 1
            elif comptador > 0:
                pistes.append(comptador)
                comptador = 0
        if comptador > 0:
            pistes.append(comptador)
        return pistes or [0]

    pistes_files = [obtenir_pistes_linia(fila) for fila in matriu]
    columnes_transposades = list(zip(*matriu))
    pistes_columnes = [obtenir_pistes_linia(col) for col in columnes_transposades]

    return pistes_files, pistes_columnes, densitat * 100


# Funció per guardar el nonograma generat en un fitxer
def guardar_nonograma(files, columnes, pistes_files, pistes_columnes, densitat):
    carpeta = "RandomNonograms"
    os.makedirs(carpeta, exist_ok=True)

    base_nom = f"RNDNNGR_{files}x{columnes}_"
    numero = 1
    while True:
        sufix = f"{numero:02d}"
        nom_fitxer = os.path.join(carpeta, f"{base_nom}{sufix}.txt")
        if not os.path.exists(nom_fitxer):
            break
        numero += 1

    with open(nom_fitxer, "w") as f:
        f.write(f"{files} {columnes}\n")
        for pista in pistes_files:
            f.write(" ".join(map(str, pista)) + "\n")
        for pista in pistes_columnes:
            f.write(" ".join(map(str, pista)) + "\n")
        f.write(f"c Aquest nonograma s'ha generat amb una densitat del {densitat:.2f}%\n")

    print(f"Nonograma desat a: {nom_fitxer}")


# Punt d’entrada del programa
def main():
    parser = argparse.ArgumentParser(description="Generador de nonogrames aleatoris.")
    parser.add_argument("-f", "--files", type=int, required=True, help="Nombre de files")
    parser.add_argument("-c", "--columnes", type=int, required=True, help="Nombre de columnes")
    parser.add_argument("-d", "--densitat", type=float, required=True, help="Densitat en percentatge (p. ex. 40.5)")

    args = parser.parse_args()

    pistes_files, pistes_columnes, densitat_real = generar_nonograma(args.files, args.columnes, args.densitat)
    guardar_nonograma(args.files, args.columnes, pistes_files, pistes_columnes, densitat_real)


if __name__ == "__main__":
    main()
