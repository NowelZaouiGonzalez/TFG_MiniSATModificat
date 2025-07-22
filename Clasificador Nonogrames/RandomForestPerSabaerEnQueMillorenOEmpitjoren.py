import os
import argparse
import pandas as pd
from sklearn.ensemble import RandomForestClassifier
from sklearn.tree import DecisionTreeClassifier, export_text
from sklearn.preprocessing import LabelEncoder
from collections import defaultdict

def mostrar_regles_arbre(model, feature_names):
    regles = export_text(model, feature_names=list(feature_names))
    print("Regles de l'arbre de decisió:\n")
    print(regles)

def carregar_info_nonogrames(info_path):
    df = pd.read_csv(info_path, sep=';')
    df = df.rename(columns=lambda col: col.strip().replace(" ", "_"))
    return df

def entrenar_i_guardar_importancies_general(df_info, df_dades, target_col, executable, tipus_csv, output_folder, usar_arbre_decisio=False):
    df_dades['Nom_Nonograma'] = df_dades['File'].apply(lambda x: os.path.splitext(os.path.basename(x))[0])

    # Eliminar 'Result' si estem en el cas d'Is_IND
    if tipus_csv in ["Is", "Is_No"] and "Result" in df_dades.columns:
        df_dades = df_dades.drop(columns=["Result"])

    df = df_info.merge(df_dades, on='Nom_Nonograma')

    if target_col not in df.columns:
        print(f"⚠️ Columna {target_col} no trobada en les dades fusionades.")
        return

    y = df[target_col]
    X = df.drop(columns=['File', 'Nom_Nonograma', target_col])

    for col in X.select_dtypes(include='object').columns:
        le = LabelEncoder()
        X[col] = le.fit_transform(X[col])

    # Model segons l'opció
    if usar_arbre_decisio:
        model = DecisionTreeClassifier(random_state=42)
        nom_model = "arbre_decisio"
    else:
        model = RandomForestClassifier(random_state=42)
        nom_model = "random_forest"

    model.fit(X, y)

    importancies = model.feature_importances_
    features = X.columns
    resultats = sorted(zip(features, importancies), key=lambda x: x[1], reverse=True)

    # Crear DataFrame de resultats
    df_resultats = pd.DataFrame(resultats, columns=["Feature", "Importancia"])

    # Afegir regles si és arbre de decisió
    if usar_arbre_decisio:
        regles = export_text(model, feature_names=list(features))
        regles_path = os.path.join(output_folder, f"{tipus_csv}_{target_col}_regles.txt")
        with open(regles_path, "w", encoding="utf-8") as f:
            f.write(regles)

    # Guardar CSV
    nom_arxiu = f"{tipus_csv}_{target_col}.csv"
    ruta_sortida = os.path.join(output_folder, nom_arxiu)
    df_resultats.to_csv(ruta_sortida, index=False, sep=';')
    print(f"Guardat {ruta_sortida}")

def analitzar_directori(directori, info_path, usar_arbre_decisio):
    df_info = carregar_info_nonogrames(info_path)

    # Carpeta arrel de sortida segons model
    base_name = os.path.basename(os.path.normpath(directori))
    nom_output_folder = f"Importancies{base_name}"
    if usar_arbre_decisio:
        nom_output_folder = os.path.join(nom_output_folder, "DecisionTree")
    else:
        nom_output_folder = os.path.join(nom_output_folder, "RandomForest")
    os.makedirs(nom_output_folder, exist_ok=True)

    for carpeta in os.listdir(directori):
        path = os.path.join(directori, carpeta)
        if not os.path.isdir(path) or not carpeta.startswith("with_"):
            continue

        # Obtenir nom de l'executable (sense "with_")
        executable = carpeta.replace("with_", "")

        # Crear subcarpeta amb nom de l'executable dins la carpeta de model
        output_subfolder = os.path.join(nom_output_folder, executable)
        os.makedirs(output_subfolder, exist_ok=True)

        arxius_metriques = defaultdict(dict)
        arxiu_is_ind = None

        for arxiu in os.listdir(path):
            arxiu_path = os.path.join(path, arxiu)
            if not arxiu.endswith(".csv"):
                continue

            if arxiu.startswith("Better_in_"):
                metrica = arxiu.replace("Better_in_", "").replace(".csv", "")
                arxius_metriques[metrica]["millora"] = arxiu_path
            elif arxiu.startswith("Worst_in_"):
                metrica = arxiu.replace("Worst_in_", "").replace(".csv", "")
                arxius_metriques[metrica]["empitjora"] = arxiu_path
            elif arxiu.startswith("Is_IND"):
                arxiu_is_ind = arxiu_path

        # Processar mètriques ordenadament
        for metrica in sorted(arxius_metriques.keys()):
            arxius = arxius_metriques[metrica]

            if "millora" in arxius:
                df_millora = pd.read_csv(arxius["millora"], sep=';')
                target_col = f"Millora_{metrica}"
                if target_col in df_millora.columns:
                    entrenar_i_guardar_importancies_general(
                        df_info, df_millora, target_col, executable, "Millora", output_subfolder, usar_arbre_decisio)

            if "empitjora" in arxius:
                df_empitjora = pd.read_csv(arxius["empitjora"], sep=';')
                target_col = f"Empitjora_{metrica}"
                if target_col in df_empitjora.columns:
                    entrenar_i_guardar_importancies_general(
                        df_info, df_empitjora, target_col, executable, "Empitjora", output_subfolder, usar_arbre_decisio)

        # Processar Is_IND al final
        if arxiu_is_ind:
            df_ind = pd.read_csv(arxiu_is_ind, sep=';')
            if "Is_IND" in df_ind.columns:
                entrenar_i_guardar_importancies_general(
                    df_info, df_ind, "Is_IND", executable, "Is", output_subfolder, usar_arbre_decisio)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d', '--directori', required=True, help='Directori amb les carpetes with_<executable>')
    parser.add_argument('-i', '--info', default='infoNonogrames.csv', help='Arxiu CSV amb info dels nonogrames')
    parser.add_argument('-dt', '--decision_tree', action='store_true', help="Usar arbre de decisió en lloc de random forest")
    args = parser.parse_args()

    if not os.path.exists(args.info):
        print(f"No s'ha trobat l'arxiu {args.info}")
        return

    analitzar_directori(args.directori, args.info, args.decision_tree)

if __name__ == '__main__':
    main()
