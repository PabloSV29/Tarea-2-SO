# Proyecto-2-SO
Repositorio para SO

# Instrucciones de Configuración y Compilación

## Pre-requisitos

Asegúrate de tener instalados los siguientes componentes en tu sistema antes de compilar el proyecto:

- **g++**: El compilador de C++. Puedes instalarlo en Ubuntu con el siguiente comando:

    ```bash
    sudo apt-get install g++
 
## Compilación para parte 1

Ingresar la siguiente línea de código en el directorio donde se encuentre el archivo:


    ```bash
    g++ Tarea2.cpp -o simulapc -pthread
    ```

Una vez completados estos pasos, se generará 1 ejecutable:

El ejecutable simulapc se puede ejecutar usando:

    ```bash
    ./simulapc -p <numero_productores> -c <numero_consumidores> -s <tamano_inicial> -t <tiempo_limite>
    ```
Luego de la ejecución se generará el archivo log.txt el cual registra los cambios sufridos por la cola.

## Compilación para parte 2

Ingresar la siguiente línea de código en el directorio donde se encuentre el archivo:


    ```bash
    g++ mvirtual.cpp -o mvirtual
    ```

El ejecutable mvirtual se puede ejecutar usando:

    ```bash
    ./mvirtual -m <num_frames> -a <algoritmo> -f <archivo_referencias>
    ```

Donde <algoritmo> puede ser FIFO, LRU, "LRU Reloj simple" u Optimo.

Los comandos disponibles y más información se puede encontrar en el PDF adjunto.
