#include <omp.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <stdlib.h>
#include <mpi.h>

using namespace std;

#define NUMERO_ELEMENTOS 100000

void mostrarDatos(int* datos, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%d ", datos[i]);
    }
}

int valorMaximo(int* datos)
{
    int valorMaximo = datos[0];
    for (int i = 0; i < NUMERO_ELEMENTOS; i++)
    {
        if(valorMaximo < datos[i]) {
            valorMaximo = datos[i];
        }
    }
    return valorMaximo;
}

int compareIntegers(const void * a, const void * b)
{ 
    return ( *(int*)a - *(int*)b );
}

void bubbleSort(int* datos, int size) {
    int temporal;
    for(int i = 0; i < size; i++) {
        for(int j = i + 1; j < size; j++) {
            if(datos[i] > datos[j]) {
                temporal = datos[i];
                datos[i] = datos[j];
                datos[j] = temporal;
            }
        }
    }
}

int* getNumeros(int size) {
    int* numeros = (int*) malloc(size * sizeof(int));
    for (int i = 0; i < size; i++)
    {
        int numero =  rand() % size + 1;
        numeros[i] = numero;
    }
    return numeros;
}

void asignarArregloFrom(int* origen, int* destino, int inicio, int fin) {
    int k = 0;
    for (int i = inicio; i < fin; i++)
    {
        destino[i] = origen[k];
        k++;
    }
}

int main(int argc, char ** argv) {
    int size, rango, i, nprocs, id, inicio, fin;
    MPI_Status status;
    MPI_Request request;
    int* arrayBucket = NULL;
    double tiempoSecuencial, tiempoParalelo, tiempoInicial, tiempoFinal;
    //Reservacion de memoria para los bloques

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    int numBuckets = nprocs; //<==== NUMERO DE BLOQUES = NPROCS
    vector<int> buckets[numBuckets];
    int* datos;
    int* datosSecuencial;
    int* datosOrdenados;

    if(id == 0) {
        datos = getNumeros(NUMERO_ELEMENTOS);
        datosOrdenados = (int*) malloc(NUMERO_ELEMENTOS * sizeof(int));
        datosSecuencial = (int*) malloc(NUMERO_ELEMENTOS * sizeof(int));

        /*printf("ARREGLO NO ORDENADO => ");
        mostrarDatos(datos, NUMERO_ELEMENTOS);
        printf("\n");*/

        asignarArregloFrom(datos, datosSecuencial, 0, NUMERO_ELEMENTOS);
        tiempoInicial = MPI_Wtime();
        //ORDENANDO CON ALGORITMO DE LIBRERIA
        qsort(datosSecuencial, NUMERO_ELEMENTOS, sizeof(int), compareIntegers);
        //ORDENANDO CON ALGORITMO IMPLEMENTADO POR EQUIPO 5
        //bubbleSort(datosSecuencial, NUMERO_ELEMENTOS);
        tiempoFinal = MPI_Wtime();
        tiempoSecuencial = tiempoFinal - tiempoInicial;

        /*printf("ARREGLO ORDENADO SECUENCIALMENTE => ");
        mostrarDatos(datosSecuencial, NUMERO_ELEMENTOS);
        printf("\n");*/
        printf("TIEMPO SECUENCIAL => %lf\n", tiempoSecuencial);

        //Reparticion de elementos a los bloques segun su clasificacion
        rango = valorMaximo(datos) / numBuckets;
        if (rango == 0)
        {
            rango = 1;
        }
        for (i = 0; i < NUMERO_ELEMENTOS; i++)
        {
            int dato = datos[i];
            int idx = dato / rango;
            if (idx > numBuckets - 1)
            {
                idx -= idx - (numBuckets - 1); 
            }
            buckets[idx].push_back(dato);
        }
        //Fin de reparticion de elementos a los bloques segun su clasificacion

        tiempoInicial = MPI_Wtime();
        //ENVIANDO EL BLOQUE CORRESPONDIENTE A CADA PROCESO DE ACUERDO A SU ID (ID = IDX)
        for (i = 0; i < numBuckets; i++)
        {
            size = buckets[i].size();
            arrayBucket = (int*) malloc(size * sizeof(int));
            copy(buckets[i].begin(), buckets[i].end(), arrayBucket);

            MPI_Isend(&size, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &request);
            MPI_Isend(arrayBucket, size, MPI_INT, i, 1, MPI_COMM_WORLD, &request);

            /*printf("ENVIANDO BLOQUE A PROCESO %d => ", i);
            mostrarDatos(arrayBucket, size);
            printf("\n");*/
        }

        //PROCESO MASTER TAMBIEN HACE TRABAJO, ORDENA EL BLOQUE ASIGNADO DE ACUERDO A SU ID (ID=IDX)
        MPI_Recv(&size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        arrayBucket = (int*) malloc(size * sizeof(int));
        MPI_Recv(arrayBucket, size, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        
        //ORDENANDO CON ALGORITMO DE LIBRERIA
        qsort(arrayBucket, size,sizeof(int), compareIntegers);
        //ORDENANDO CON ALGORITMO IMPLEMENTADO POR EQUIPO 5
        //bubbleSort(arrayBucket, size);
        
        inicio = 0;
        fin = size;

        /*printf("RECIBIENDO BLOQUE ORDENADO DE PROCESO %d => ", status.MPI_SOURCE);
        mostrarDatos(arrayBucket, size);
        printf("\n");*/

        //printf("INICIO => %d Y FIN => %d\n", inicio, fin);
        asignarArregloFrom(arrayBucket, datosOrdenados, inicio, fin);

        /*printf("ARREGLO ORDENADO => ");
        mostrarDatos(datosOrdenados, NUMERO_ELEMENTOS);
        printf("\n");*/

        for (i = 1; i < numBuckets; i++)
        {
            MPI_Recv(&size, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &status);
            arrayBucket = (int*) malloc(size * sizeof(int));
            MPI_Recv(arrayBucket, size, MPI_INT, i, 1, MPI_COMM_WORLD, &status);

            inicio = fin;
            fin += size;
            asignarArregloFrom(arrayBucket, datosOrdenados, inicio, fin);

            /*printf("RECIBIENDO BLOQUE ORDENADO DE PROCESO %d => ", status.MPI_SOURCE);
            mostrarDatos(arrayBucket, size);
            printf("\n");*/

            //printf("INICIO => %d Y FIN => %d\n", inicio, fin);
        }

        /*printf("ARREGLO ORDENADO PARALELAMENTE => ");
        mostrarDatos(datosOrdenados, NUMERO_ELEMENTOS);
        printf("\n");*/

        tiempoFinal = MPI_Wtime();
        tiempoParalelo = tiempoFinal - tiempoInicial;
        printf("TIEMPO PARALELO => %lf\n", tiempoParalelo);
        printf("SPEEDUP => %lf\n", (tiempoSecuencial / tiempoParalelo));

    }else {//ENTRAN LOS HILOS CON ID != 0
        /*RECIBIENDO BLOQUE O BUCKET A ORDENAR SEGUN EL ID DE PROCESO*/
        MPI_Recv(&size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        arrayBucket = (int*) malloc(size * sizeof(int));
        MPI_Recv(arrayBucket, size, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
        
        //ORDENANDO CON ALGORITMO DE LIBRERIA
        qsort(arrayBucket, size,sizeof(int), compareIntegers);
        //ORDENANDO CON ALGORITMO IMPLEMENTADO POR EQUIPO 5
        //bubbleSort(arrayBucket, size);

        //ENVIANDO TAMAÑO Y BLOQUE ORDENADO
        MPI_Isend(&size, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &request);
        MPI_Isend(arrayBucket, size, MPI_INT, 0, 1, MPI_COMM_WORLD, &request);
    }

    MPI_Finalize();
}