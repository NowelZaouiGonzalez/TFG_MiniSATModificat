import argparse
import os
import pandas as pd
import re

def netejar_nom(nom):
    return re.sub(r'^ENC\d+_', '', nom)

def normalitzar_columnes(df):
    df.columns = [col.strip().replace(" ", "_") for col in df.columns]
    return df

def comparar_csvs(base_csv, comparar_csv, output_dir):
    base_df = pd.read_csv(base_csv, sep=';')
    comparar_df = pd.read_csv(comparar_csv, sep=';')

    base_df = normalitzar_columnes(base_df)
    comparar_df = normalitzar_columnes(comparar_df)

    if 'File' not in base_df.columns or 'File' not in comparar_df.columns:
        print(f"⚠️ Alguna de les taules no conté la columna 'File'")
        print(f"base_df: {base_df.columns.tolist()}")
        print(f"comparar_df: {comparar_df.columns.tolist()}")
        return

    comparar_df = comparar_df[comparar_df['File'].isin(base_df['File'])]
    base_df = base_df[base_df['File'].isin(comparar_df['File'])]

    combinat = comparar_df.merge(base_df, on='File', suffixes=('', '_base'))

    camps = ['Restarts', 'Conflicts', 'Decisions', 'CPU_Time']

    for camp in camps:
        millora_col = f'Millora_{camp}'
        empitjora_col = f'Empitjora_{camp}'

        def es_buit(val):
            return pd.isna(val) or val == '' or str(val).strip() == ''

        def avaluar_millora(fila):
            resultat_base = str(fila.get('Result_base', '')).strip().upper()
            resultat_actual = str(fila.get('Result', '')).strip().upper()

            valor_actual = fila.get(camp, None)
            valor_base = fila.get(f'{camp}_base', None)

            if pd.isna(fila.get('Result')) or resultat_actual == 'ERR':
                return 0

            if resultat_base == 'IND' and resultat_actual != 'IND':
                return 1
            elif resultat_actual == 'IND' and resultat_base != 'IND':
                return 0
            elif resultat_base != 'IND' and resultat_actual != 'IND':
                if resultat_base != resultat_actual:
                    raise ValueError(f"Resultat incompatible entre base i actual: {resultat_base} vs {resultat_actual} per {fila['File']}")
                try:
                    valor_actual = pd.to_numeric(valor_actual, errors='coerce')
                    valor_base = pd.to_numeric(valor_base, errors='coerce')
                    return 1 if valor_actual < valor_base else 0
                except Exception:
                    return 0
            else:
                return 0

        def avaluar_empitjora(fila):
            resultat_base = str(fila.get('Result_base', '')).strip().upper()
            resultat_actual = str(fila.get('Result', '')).strip().upper()

            valor_actual = fila.get(camp, None)
            valor_base = fila.get(f'{camp}_base', None)

            if pd.isna(fila.get('Result')) or resultat_actual == 'ERR':
                return 1

            if resultat_base == 'IND' and resultat_actual != 'IND':
                return 0
            elif resultat_actual == 'IND' and resultat_base != 'IND':
                return 1
            elif resultat_base != 'IND' and resultat_actual != 'IND':
                if resultat_base != resultat_actual:
                    raise ValueError(f"Resultat incompatible entre base i actual: {resultat_base} vs {resultat_actual} per {fila['File']}")
                try:
                    valor_actual = pd.to_numeric(valor_actual, errors='coerce')
                    valor_base = pd.to_numeric(valor_base, errors='coerce')
                    return 1 if valor_actual > valor_base else 0
                except Exception:
                    return 0
            else:
                return 0

        combinat[millora_col] = combinat.apply(avaluar_millora, axis=1)
        combinat[empitjora_col] = combinat.apply(avaluar_empitjora, axis=1)

    # Desa un CSV per a cada millora
    for camp in camps:
        millora_col = f'Millora_{camp}'
        nom_csv = f'Better_in_{camp}.csv'
        output_path = os.path.join(output_dir, nom_csv)

        df_sortida = combinat[['File', millora_col]].copy()
        df_sortida['File'] = df_sortida['File'].apply(netejar_nom)
        df_sortida.to_csv(output_path, sep=';', index=False)

    # Desa un CSV per a cada empitjora
    for camp in camps:
        empitjora_col = f'Empitjora_{camp}'
        nom_csv = f'Worst_in_{camp}.csv'
        output_path = os.path.join(output_dir, nom_csv)

        df_sortida = combinat[['File', empitjora_col]].copy()
        df_sortida['File'] = df_sortida['File'].apply(netejar_nom)
        df_sortida.to_csv(output_path, sep=';', index=False)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--directori', required=True, help='Directori principal amb subcarpetes')
    parser.add_argument('-b', '--base', default='minisat000', help='Nom de la carpeta base de comparació (per defecte: minisat000)')
    args = parser.parse_args()

    directori = args.directori
    nom_base = args.base
    nom_directori = os.path.basename(os.path.normpath(directori))
    output_root = f'nonogramesMillors_{nom_directori}'
    os.makedirs(output_root, exist_ok=True)

    base_path = os.path.join(directori, nom_base)
    base_csv = os.path.join(base_path, f'res_{nom_base}.csv')
    if not os.path.exists(base_csv):
        print(f"❌ No s'ha trobat l'arxiu CSV de referència a {nom_base}")
        return

    for nom_carpeta in os.listdir(directori):
        path = os.path.join(directori, nom_carpeta)
        if not os.path.isdir(path) or nom_carpeta == nom_base:
            continue

        csv_path = os.path.join(path, f'res_{nom_carpeta}.csv')
        if not os.path.isfile(csv_path):
            print(f"⚠️ No s'ha trobat CSV per a {nom_carpeta}")
            continue

        output_dir = os.path.join(output_root, f'with_{nom_carpeta}')
        os.makedirs(output_dir, exist_ok=True)

        comparar_csvs(base_csv, csv_path, output_dir)

if __name__ == '__main__':
    main()
