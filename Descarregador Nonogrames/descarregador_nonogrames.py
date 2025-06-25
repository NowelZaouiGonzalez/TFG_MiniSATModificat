import argparse
from selenium import webdriver  # per instal·lar necessites: python -m pip install selenium
from selenium.webdriver.chrome.options import Options
from selenium.webdriver.common.by import By
from selenium.webdriver.chrome.service import Service
import time
import re
import os
import requests

# Variables globals
FILAS = None
COLUMNAS = None
MATRIZ_NMTL = None
MATRIZ_NMTT = None
DIRECTORIO_SALIDA = "Nonogrames"  # Directori on es desaran els fitxers

# Funció per transposar una matriu
def transposar_matriu(matriu):
    return list(map(list, zip(*matriu)))

# Funció per eliminar els valors 0 de la matriu
def eliminar_valors_0(matriu):
    return [[valor for valor in fila if valor != 0] for fila in matriu]

# Funció per extreure la taula d'una classe específica
def extreure_taula_de_classe(navegador, classe):
    taules = navegador.find_elements(By.CLASS_NAME, classe)

    if taules:
        files = taules[0].find_elements(By.TAG_NAME, "tr")
        matriu = []

        for fila in files:
            cel·les = fila.find_elements(By.TAG_NAME, "td")
            fila_matriu = []

            for cel·la in cel·les:
                text = cel·la.text.strip()

                # Si la cel·la conté text numèric, l’afegim a la matriu
                if text.isdigit():
                    fila_matriu.append(int(text))
                else:
                    fila_matriu.append(0)  # Usem 0 per cel·les buides o no numèriques

            matriu.append(fila_matriu)

        return matriu
    else:
        print(f"No s'ha trobat la taula amb la classe '{classe}' a la pàgina.")
        return None

# Funció per extreure el contingut de la pàgina
def extreure_contingut(link: str):
    global FILAS, COLUMNAS, MATRIZ_NMTL, MATRIZ_NMTT

    opcions = Options()
    opcions.add_argument("--headless")  # Executa el navegador en segon pla
    servei = Service("chromedriver.exe")
    navegador = webdriver.Chrome(service=servei, options=opcions)

    try:
        print(f"Carregant pàgina: {link}")
        navegador.get(link)
        time.sleep(2)  # Espera de 2 segons perquè es carregui el JS (ajustable)

        # Buscar l'etiqueta <td> amb la paraula 'Size:'
        td_elements = navegador.find_elements(By.XPATH, "//td[contains(text(), 'Size:')]")

        if not td_elements:
            raise Exception("No s'ha trobat la mida del nonograma (files i columnes).")

        for td in td_elements:
            # Expressió regular per obtenir N i M
            match = re.search(r"Size: (\d+)x(\d+)", td.text)
            if match:
                N = match.group(1)
                M = match.group(2)
                COLUMNAS = int(N)
                FILAS = int(M)

        if FILAS is None or COLUMNAS is None:
            raise Exception("No s'han pogut obtenir les dimensions (files i columnes) del nonograma.")

        # Extreure la taula 'nmtt'
        matriu_nmtt = extreure_taula_de_classe(navegador, "nmtt")
        if matriu_nmtt:
            MATRIZ_NMTT = transposar_matriu(matriu_nmtt)
            MATRIZ_NMTT = eliminar_valors_0(MATRIZ_NMTT)
        else:
            raise Exception("No s'ha pogut extreure la taula 'nmtt'.")

        # Extreure la taula 'nmtl'
        matriu_nmtl = extreure_taula_de_classe(navegador, "nmtl")
        if matriu_nmtl:
            MATRIZ_NMTL = eliminar_valors_0(matriu_nmtl)
        else:
            raise Exception("No s'ha pogut extreure la taula 'nmtl'.")

        # Descarregar la imatge de la solució
        if not descarregar_imatge_solucio(navegador):
            raise Exception("No s'ha pogut descarregar la imatge de la solució.")

        # Guardar la informació en un fitxer
        guardar_en_fitxer(link)

    except Exception as e:
        print(f"Error en processar la pàgina: {e}")
    finally:
        navegador.quit()

# Funció per generar el nom del fitxer amb un número incremental si cal
def generar_nom_fitxer():
    if FILAS is not None and COLUMNAS is not None:
        nom_base = f"NNGR_{FILAS}x{COLUMNAS}_01.txt"  # Sempre comença amb _01
        directori = DIRECTORIO_SALIDA

        if not os.path.exists(directori):
            os.makedirs(directori)

        path_fitxer = os.path.join(directori, nom_base)

        if not os.path.exists(path_fitxer):
            return path_fitxer

        contador = 2  # Comença des de 02, ja que 01 està ocupat
        while True:
            fitxer_incrementat = os.path.join(directori, f"NNGR_{FILAS}x{COLUMNAS}_{str(contador).zfill(2)}.txt")
            if not os.path.exists(fitxer_incrementat):
                return fitxer_incrementat
            contador += 1
    else:
        return "nonograma.txt"

# Funció per guardar la informació en un fitxer
def guardar_en_fitxer(link):
    fitxer_sortida = generar_nom_fitxer()

    with open(fitxer_sortida, "w") as fitxer:
        fitxer.write(f"{FILAS} {COLUMNAS}\n")

        for fila in MATRIZ_NMTL:
            fitxer.write(" ".join(map(str, fila)) + "\n")

        for fila in MATRIZ_NMTT:
            fitxer.write(" ".join(map(str, fila)) + "\n")

        fitxer.write(f"\nc {link}\n")

    print(f"Fitxer generat: {fitxer_sortida}")

# Funció per descarregar la imatge de la solució
def descarregar_imatge_solucio(navegador):
    try:
        enllac_imatge = navegador.find_element(By.ID, "nonogram_answer").get_attribute("href")

        nom_fitxer = generar_nom_fitxer().split("\\")[-1].replace(".txt", "")

        directori_solucions = os.path.join(DIRECTORIO_SALIDA, "Solucions")
        if not os.path.exists(directori_solucions):
            os.makedirs(directori_solucions)

        path_imatge = os.path.join(directori_solucions, f"SOLUTION_{nom_fitxer}.png")

        response = requests.get(enllac_imatge)
        if response.status_code == 200:
            with open(path_imatge, "wb") as imatge:
                imatge.write(response.content)
            print(f"Imatge de la solució descarregada: {path_imatge}")
            return True
        else:
            print("No s'ha pogut descarregar la imatge de la solució.")
            return False
    except Exception as e:
        print(f"Error en descarregar la imatge de la solució: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description="Extreu la mida del Nonograma i les taules de les classes 'nmtt' i 'nmtl'.")
    parser.add_argument('-l', '--link', required=True, help="Enllaç de la pàgina del Nonograma")
    args = parser.parse_args()

    extreure_contingut(args.link)

if __name__ == "__main__":
    main()
