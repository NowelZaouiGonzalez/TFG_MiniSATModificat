import os
import csv
import math

def procesar_txt(path_txt):
    with open(path_txt, 'r') as file:
        lines = file.readlines()

    M, N = map(int, lines[0].strip().split())
    pistas_filas = lines[1:1 + M]
    pistas_columnas = lines[1 + M:1 + M + N]
    pistas_totales = pistas_filas + pistas_columnas

    casillas = M * N

    tamaños_pistas = []
    pintadas = 0

    #Llibertat Pistes Fila

    llibertat_files=0

    #Llibertat Pistes Columna
    llibertat_columnes=0

    # Procesar pistes de filas
    for linea in pistas_filas:
        numeros = list(map(int, linea.strip().split()))
        pintadas += sum(numeros)

        llibertat_files += 0 if numeros == [0] else N - sum(numeros) + (len(numeros) - 1) #Quant espai lliure hi ha entre que es poden moure les pistes de fila

    # Procesar pistes de filas
    for linea in pistas_columnas:
        numeros = list(map(int, linea.strip().split()))
        llibertat_columnes += 0 if numeros == [0] else M - sum(numeros) + (len(numeros) - 1) #Quant espai lliure hi ha entre que es poden moure les pistes de fila

        
    for linea in pistas_totales:
        numeros = list(map(int, linea.strip().split()))
        tamaños_pistas.extend(numeros)
        

    cantidad_pistas = len(tamaños_pistas)
    tamaño_promedio_pistas = round(pintadas / cantidad_pistas, 2) if cantidad_pistas else 0.0

    # Desviación estandar
    if tamaños_pistas:
        media = sum(tamaños_pistas) / cantidad_pistas
        varianza = sum((x - media) ** 2 for x in tamaños_pistas) / cantidad_pistas
        desviacion = round(math.sqrt(varianza), 2)
    else:
        desviacion = 0.0

    # Pistes promedio per fila i columna
    total_pistas_filas = sum(len(line.strip().split()) for line in pistas_filas)
    total_pistas_columnas = sum(len(line.strip().split()) for line in pistas_columnas)
    promedio_fila = round(total_pistas_filas / M, 2) if M else 0.0
    promedio_columna = round(total_pistas_columnas / N, 2) if N else 0.0

    porcentaje = round((pintadas / casillas) * 100, 2) if casillas else 0.0
    ratio = round(M / N, 2) if N else 0.0

    llibertat_promig_fila=(llibertat_files)/M
    llibertat_promig_columnes=(llibertat_columnes)/N
    llibertat_promig_total = (llibertat_promig_fila + llibertat_promig_columnes) / 2
    llibertat_relativa_total=((llibertat_files+llibertat_columnes) /2)/(M*N)

    nombre_sin_extension = os.path.splitext(os.path.basename(path_txt))[0]

    return (
        nombre_sin_extension,  # 0
        casillas,              # 1
        pintadas,              # 2
        porcentaje,            # 3
        M,                     # 4
        N,                     # 5
        ratio,                 # 6
        cantidad_pistas,       # 7
        tamaño_promedio_pistas,# 8
        desviacion,            # 9
        promedio_fila,         # 10
        promedio_columna,       # 11
        llibertat_promig_fila,
        llibertat_promig_columnes,
        llibertat_promig_total,
        llibertat_relativa_total
        
    )

def main(directorio):
    resultados = []

    for archivo in os.listdir(directorio):
        if archivo.endswith('.txt'):
            path_txt = os.path.join(directorio, archivo)
            resultado = procesar_txt(path_txt)
            resultados.append(resultado)

    # Ordenar por la cantidad de casillas
    resultados.sort(key=lambda x: x[0])  # x[0] = casillas

    with open('infoNonogrames.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile, delimiter=';')
        writer.writerow([
            'Nom Nonograma',
            # 'Caselles',
            # 'Pintades',
            '% Caselles Pintades',
            # 'Files',
            # 'Columnes',
            'Ratio F/C',
            'Total Pistes',
            'Mitjana Mida Pistes',
            'Desviacio Est. Mida Pistes',
            'Promig Pistes/Fila',
            'Promig Pistes/Columna',
            # 'Promig Llibertat Fila',
            # 'Promig Llibertat Columna',
            'Promig Llibertat Total',
            'Relatiu Llibertat Total'
        ])
        for fila in resultados:
            writer.writerow([
                fila[0],  # Nombre
                # fila[1],  # Casillas
                # fila[2],  # Pintadas
                f"{fila[3]:.2f}",  # %
                # fila[4],  # Filas
                # fila[5],  # Columnas
                f"{fila[6]:.2f}",  # Ratio
                fila[7],  # Total pistas
                f"{fila[8]:.2f}",  # Tamaño promedio
                f"{fila[9]:.2f}",  # Desviación estándar
                f"{fila[10]:.2f}", # Pistas/Fila
                f"{fila[11]:.2f}",  # Pistas/Columna
                # f"{fila[12]:.2f}",  # llibertat files
                # f"{fila[13]:.2f}",  # llibertat Columna
                f"{fila[14]:.2f}",  # llibertat Total
                f"{fila[15]:.2f}"  # llibertat Relativa Total
            ])

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 2:
        print("Us: python script.py <directorio>")
    else:
        main(sys.argv[1])
