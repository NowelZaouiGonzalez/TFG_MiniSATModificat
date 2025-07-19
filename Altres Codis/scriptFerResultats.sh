#!/bin/bash

# Función para procesar el archivo y extraer la información de MiniSat
read_minisat_result() {
    local input_file="$1"
    local timeout_value="$2"

    # Inicializar variables
    local restarts=""
    local conflicts=""
    local decisions="0" #Si es 0, el percentatge es nan i la ER falla. 
    local propagations=""
    local conflict_literals=""
    local memory_used=""
    local cpu_time=""
    local result="ERR"

    # Leer el archivo línea por línea
    while IFS= read -r line; do
        if [[ $line =~ ^restarts[[:space:]]*:[[:space:]]*([0-9]+) ]]; then
            restarts="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^conflicts[[:space:]]*:[[:space:]]*([0-9]+) ]]; then
            conflicts="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^decisions[[:space:]]*:[[:space:]]*([0-9]+)[[:space:]]*\([0-9.]+[[:space:]]*%\ random\)[[:space:]]*\(([0-9]+)[[:space:]]*/sec\) ]]; then
        #elif [[ $line =~ ^decisions[[:space:]]*:[[:space:]]*([0-9]+)[[:space:]]*\(([^)]+)\ random\)[[:space:]]*\(([0-9]+)[[:space:]]*/sec\) ]]; then
            decisions="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^propagations[[:space:]]*:[[:space:]]*([0-9]+)[[:space:]]*\(([0-9]+)[[:space:]]*/sec\) ]]; then
            propagations="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^conflict\ literals[[:space:]]*:[[:space:]]*([0-9]+) ]]; then
            conflict_literals="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^Memory\ used[[:space:]]*:[[:space:]]*([0-9.]+)[[:space:]]*MB ]]; then
            memory_used="${BASH_REMATCH[1]}"
        elif [[ $line =~ ^CPU\ time[[:space:]]*:[[:space:]]*([0-9.]+)[[:space:]]*s ]]; then
            cpu_time="${BASH_REMATCH[1]}"
        elif [[ $line =~ UNSATISFIABLE ]]; then
            result="UNS"
        elif [[ $line =~ SATISFIABLE ]]; then
            result="SAT"
        elif [[ $line =~ INDETERMINATE ]]; then
            result="IND"
            # Si el parámetro timeout_value está presente, asignarlo a cpu_time
            if [[ -n "$timeout_value" ]]; then
                cpu_time="$timeout_value"
                fi
        fi
    done < "$input_file"

    # Devolver los valores como una cadena
    echo "$restarts $conflicts $decisions $propagations $conflict_literals $memory_used $cpu_time $result"
}

# Variable para la ruta del directorio con los archivos .dimacs
dimacs_dir="."
# Variable para el tiempo de espera en segundos (por defecto 1 hora)
time_limit=3600

mem_limit=4500

# Executable minisat que utilitzara
executable_minisat="minisat"

guardar_solucio=false

# Procesar los parámetros opcionales -d y -t
while getopts ":d:t:e:s" opt; do
    case ${opt} in
        d )
            dimacs_dir="$OPTARG"
            ;;
        t )
            time_limit="$OPTARG"
            ;;
        e )
            executable_minisat="$OPTARG"
            ;;
        s )
            guardar_solucio=true
            ;;
        \? )
            echo "Opción inválida: -$OPTARG" 1>&2
            exit 1
            ;;
        : )
            echo "Opción -$OPTARG requiere un argumento." 1>&2
            exit 1
            ;;
    esac
done
shift $((OPTIND -1))

# Crear directorio results si no existe
mkdir -p results
mkdir -p results/Solutions
mkdir -p results/ReduccioInfo

# Archivo CSV de resultados
result_file="results/resultats.csv"

# Escribir encabezado en el archivo CSV
echo "File;Restarts;Conflicts;Decisions;Propagations;Conflict Literals;Memory Used;CPU Time;Execution Time;Result" > "$result_file"

# Iterar sobre todos los archivos .dimacs en el directorio especificado
for dimacs_file in "$dimacs_dir"/*.dimacs; do
    echo "Analizando archivo: $dimacs_file"
    executable_name=$(basename "$executable_minisat")
    echo "Executant amb $executable_name"

    # Extraer solo el nombre del archivo sin el path y cambiar la extensión a .txt
    filename=$(basename "$dimacs_file")
    base_filename="${filename%.dimacs}"
    txt_filename="${base_filename}.txt"
    
    # Obtener la hora de inicio en formato HH:MM
    start_time_human=$(date +"%H:%M")

    # Calcular la hora estimada de timeout sumando los segundos de time_limit
    timeout_time_human=$(date -d "@$(( $(date +%s) + time_limit ))" +"%H:%M")

    echo "Hora de inicio: $start_time_human"
    echo "Hora estimada de timeout: $timeout_time_human"
    # Ejecutar minisat
    start_time=$(date +%s%N)  # Guarda el tiempo inicial en nanosegundos

    arxiu_solucio=""
    if $guardar_solucio; then
        arxiu_solucio="results/Solutions/solution_$base_filename.txt"
    fi

    read -ra EXEC <<< "$executable_minisat"  # Divide el string en un array
    EXEC[0]="./${EXEC[0]}" 
    timeout --signal=SIGINT $time_limit "${EXEC[@]}" "-mem-lim=$mem_limit" "$dimacs_file" $arxiu_solucio > "results/$txt_filename" &
    pid=$!  # Guarda el PID del proceso en segundo plano
    wait $pid  # Espera a que termine completamente


    # # Ejecutar el script minisat_original en segundo plano y obtener su PID
    # "./$executable_minisat" "-mem-lim=$mem_limit" "$dimacs_file" "results/Solutions/solution_$base_filename.txt" > "results/$txt_filename" &
    # pid=$!

    # # Si el tiempo de espera es mayor que 0, esperar y enviar SIGINT si el proceso aún se está ejecutando
    # if [[ $time_limit -gt 0 ]]; then
    #     {
    #         sleep $time_limit
    #         if ps -p $pid > /dev/null; then
    #             kill -SIGINT $pid
    #         fi
    #     } &
    # fi

    # # Esperar a que el proceso termine después de enviar SIGINT
    # wait $pid

    end_time=$(date +%s%N)  # Guarda el tiempo final en nanosegundos

    execution_time_ms=$((end_time - start_time))
    execution_time=$(awk "BEGIN {printf \"%.6f\", ($end_time - $start_time) / 1000000000}") #Convertir a segundos con decimales
    echo "Tiempo total de ejecución: $execution_time segundos"

    # Verificar si l'arxiu reduccio.txt existeix i el mou si es necesari
    if [[ -f "reduccio.txt" ]]; then
        mv "reduccio.txt" "results/ReduccioInfo/reduccio_$base_filename.txt"
    fi

    # Llamar a la función con el archivo txt como argumento y capturar los valores
    result_string=$(read_minisat_result "results/$txt_filename")

    # Asignar los valores de la cadena a las variables sin necesidad de declararlas previamente
    read -r restarts_original conflicts_original decisions_original propagations_original conflict_literals_original memory_used_original cpu_time_original result <<< "$result_string"

    # Escribir los resultados en el archivo CSV
    echo "$base_filename;$restarts_original;$conflicts_original;$decisions_original;$propagations_original;$conflict_literals_original;$memory_used_original;$cpu_time_original;$execution_time;$result" >> "$result_file"
done

echo "Aquests resultats s'han obtinguts executant $executable_name amb un timeout de $time_limit segons i una memoria limit de $mem_limit MB" > "results/README"

echo "Resultados guardados en $result_file"
