/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H

#include "const.h"
#include "HAL.h"
#include "llamsis.h"
#include <string.h>

#define NO_RECURSIVO 0
#define RECURSIVO 1
#define MUTEX_BLOQUEADO 1
#define MUTEX_DESBLOQUEADO 0

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
        int id;				/* ident. del proceso */
        int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
        contexto_t contexto_regs;	/* copia de regs. de UCP */
        void * pila;			/* dir. inicial de la pila */
	BCPptr siguiente;		/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */
	unsigned int tiempo_dormir;
	unsigned int rodaja;
	int numero_mutex;               	/*numero que dice cuantos mutex tiene abiertos*/
	struct mutex *descriptores_mutex_sistema[NUM_MUT_PROC]; /*Array que almacena los descriptores de los mutex*/
} BCP;

/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en semáforo, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};
lista_BCPs lista_dormidos= {NULL, NULL};
/*
 *
 * Definición del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;

//MUTEX

typedef struct  mutex{
	char nombre_mutex[MAX_NOM_MUT]; //Nombre del mute, el +1 es para el caracter de terminación
	int valor;			  //Valor del mutex 0 o 1
	int num_procesos;		//Número de procesos que están usando el mutex
	int tipo_mutex;			//Almacena si el mutex es o no recursivo
	lista_BCPs lista_bloqueados; 	//Procesos bloqueados por el mutex
	int BCP_id_lock;		//almacena el identificador del BCP que tiene el mutex bloqueado para que solo el pueda desbloquearlo
	int contador_recursivo;		//almacena la cantidad de veces que se ha bloqueado un mutex recursivo
	
	
}mutex;



struct lista_mutex{
  
    mutex lista[NUM_MUT]; //lista con los mutex creados
    lista_BCPs bloqueados_en_espera; //Procesos que estan en espera para crear un mutex
    int contador_mutex; //Cuantos mutex hay creados

  
  
}lista_mutex;
/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();
int obtener_id_pr();
int dormir();
int crear_mutex();
int abrir_mutex();
int lock();
int unlock();
int cerrar_mutex();

/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
					{obtener_id_pr},
					{dormir},
					{crear_mutex},
					{abrir_mutex},
					{cerrar_mutex},
					{lock},
					{unlock}};

#endif /* _KERNEL_H */

