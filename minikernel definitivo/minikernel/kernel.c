/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"	/* Contiene defs. usadas por este modulo */
static void int_sw();

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Función que inicia la tabla de procesos
 */
static int prueba=0;

static void iniciar_tabla_proc(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		tabla_procs[i].estado=NO_USADA;
}

/*
 * Función que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre(){
	int i;

	for (i=0; i<MAX_PROC; i++)
		if (tabla_procs[i].estado==NO_USADA)
			return i;
	return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP * proc){
	if (lista->primero==NULL)
		lista->primero= proc;
	else
		lista->ultimo->siguiente=proc;
	lista->ultimo= proc;
	proc->siguiente=NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista){

	if (lista->ultimo==lista->primero)
		lista->ultimo=NULL;
	lista->primero=lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP * proc){
	BCP *paux=lista->primero;

	if (paux==proc){
		eliminar_primero(lista);//printk("ID_PROCESO %d: \n",p_proc_actual->id);
	  
	}
	else {
		for ( ; ((paux) && (paux->siguiente!=proc));
			paux=paux->siguiente);
		if (paux) {
		  
			if (lista->ultimo==paux->siguiente)
				lista->ultimo=paux;
			paux->siguiente=paux->siguiente->siguiente;
		}
	}
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int(){
	int nivel;

	//printk("-> NO HAY LISTOS. ESPERA INT\n");

	/* Baja al mínimo el nivel de interrupción mientras espera */
	nivel=fijar_nivel_int(NIVEL_1);
	halt();
	fijar_nivel_int(nivel);
}

/*
 * Función de planificacion que implementa un algoritmo FIFO.
 */
static BCP * planificador(){
	while (lista_listos.primero==NULL)
		espera_int();		/* No hay nada que hacer */
	return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso(){
	BCP * p_proc_anterior;
	
	if(p_proc_actual->numero_mutex!=0)
	cerrar_mutex_proceso(p_proc_actual);
	
	liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */
  
	p_proc_actual->estado=TERMINADO;
	
	eliminar_primero(&lista_listos); /* proc. fuera de listos */

	/* Realizar cambio de contexto */
	p_proc_anterior=p_proc_actual;
	p_proc_actual=planificador();

	printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
			p_proc_anterior->id, p_proc_actual->id);

	liberar_pila(p_proc_anterior->pila);
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
        return; /* no debería llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit(){

	if (!viene_de_modo_usuario())
	 
		panico("excepcion aritmetica cuando estaba dentro del kernel");


	printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem(){

	
	if (!viene_de_modo_usuario())
		panico("excepcion de memoria cuando estaba dentro del kernel");

	
	printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
	liberar_proceso();

        return; /* no debería llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal(){
	char car;

	car = leer_puerto(DIR_TERMINAL);
	printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

        return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj(){

	//printk("-> TRATANDO INT. DE RELOJ\n");
	if(p_proc_actual->estado == LISTO){
	  if(p_proc_actual->rodaja != 0){
	    p_proc_actual->rodaja =  p_proc_actual->rodaja-1;
	    printf("rodaja %d\n",  p_proc_actual->rodaja);
	  
	    if(p_proc_actual->rodaja == 0){
	      int_sw(); 
	    }
	  }else{
	      p_proc_actual->rodaja = 3;
	  }
	}
	// control de proceso
	if(lista_dormidos.primero != NULL){
		// proceso con el que se operará
		BCP * p_dormido;
		// proceso siguiente de la lista
		BCP * p_dormido_sig;
		// variable que almacenará en el nivel de interrupción
		int nivel;
		// se accede mediante el struct al primer elemento de la lista	
		p_dormido = lista_dormidos.primero;
		// bucle que procesa la lista de procesos dormidos
		while(p_dormido != NULL){
			// se reduce el tiempo
			p_dormido->tiempo_dormir--;
			// se guarda el siguiente proceso
			p_dormido_sig = p_dormido->siguiente;
			// en el caso del que tiempo acabe
			if(p_dormido->tiempo_dormir <= 0){
				// se desbloquea el proceso
				p_dormido->estado = LISTO;
				// se notifica la interrupción de reloj
				nivel=fijar_nivel_int(NIVEL_3);
				// se notifica por pantalla que el proceso ha despertado
				printk("proceso %d despierta y se va a la lista de listos \n", p_dormido->id);
				// se elimina el proceso de la lista de dormidos
				eliminar_elem(&lista_dormidos, p_dormido);
				// se inserta en la lista de listos
				insertar_ultimo(&lista_listos, p_dormido);
				// se termina la interrupción del reloj
				fijar_nivel_int(nivel);			
			}
			// se pasa al siguiente proceso
			p_dormido = p_dormido_sig;
		}
	}
        return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis(){
	int nserv, res;

	nserv=leer_registro(0);
	if (nserv<NSERVICIOS)
		res=(tabla_servicios[nserv].fservicio)();
	else
		res=-18;		/* servicio no existente */
	escribir_registro(0,res);
	return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw(){
	printk("-> TRATANDO INT. SW\n");
	cambio_pr(&lista_listos);
}

void iniciar_lista_mutex_sistema(){  
  lista_mutex.contador_mutex=0;
  lista_mutex.bloqueados_en_espera.primero=NULL;
  lista_mutex.bloqueados_en_espera.ultimo=NULL;
  
  for(int i=0;i<NUM_MUT;i++){
    lista_mutex.lista[i].num_procesos=0;
  }  
}

void iniciar_lista_mutex(BCP *proc){  
  proc->numero_mutex=0;  
  for(int i=0;i<NUM_MUT_PROC;i++){    
   proc->descriptores_mutex_sistema[i]=NULL;    
  }  
  
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */



static int crear_tarea(char *prog){
	void * imagen, *pc_inicial;
	int error=0;
	int proc;
	BCP *p_proc;
	int nivel;

	proc=buscar_BCP_libre();
	if (proc==-1)
		return -1;	/* no hay entrada libre */

	/* A rellenar el BCP ... */
	p_proc=&(tabla_procs[proc]);
	/* crea la imagen de memoria leyendo ejecutable */	
	imagen=crear_imagen(prog, &pc_inicial);	
	
	if (imagen){
		p_proc->info_mem=imagen;
		p_proc->pila=crear_pila(TAM_PILA);
		fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
			pc_inicial,
			&(p_proc->contexto_regs));
		p_proc->id=proc;
		iniciar_lista_mutex(p_proc); //Inicio la lista de mutex del proceso
		p_proc->estado=LISTO;
		// rodaja del round robin
		p_proc->rodaja=TICKS_POR_RODAJA;		
		/* lo inserta al final de cola de listos */
		nivel=fijar_nivel_int(NIVEL_3);
		insertar_ultimo(&lista_listos, p_proc);
		fijar_nivel_int(nivel);
		error= 0;
	}
	else
		error= -1; /* fallo al crear imagen */

	return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso(){
	char *prog;
	int res;

	printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
	prog=(char *)leer_registro(1);
	
	res=crear_tarea(prog);
	return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir()
{
	char *texto;
	unsigned int longi;

	texto=(char *)leer_registro(1);
	longi=(unsigned int)leer_registro(2);

	escribir_ker(texto, longi);
	return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso(){

	printk("-> FIN PROCESO %d\n", p_proc_actual->id);

	liberar_proceso();

        return 0; /* no debería llegar aqui */
}



/*
 *
 * Rutina de inicialización invocada en arranque
 *
 */
int main(){
	/* se llega con las interrupciones prohibidas */

	instal_man_int(EXC_ARITM, exc_arit); 
	instal_man_int(EXC_MEM, exc_mem); 
	instal_man_int(INT_RELOJ, int_reloj); 
	instal_man_int(INT_TERMINAL, int_terminal); 
	instal_man_int(LLAM_SIS, tratar_llamsis); 
	instal_man_int(INT_SW, int_sw); 

	iniciar_cont_int();		/* inicia cont. interr. */
	iniciar_cont_reloj(TICK);	/* fija frecuencia del reloj */
	iniciar_cont_teclado();		/* inici cont. teclado */

	iniciar_tabla_proc();		/* inicia BCPs de tabla de procesos */
	iniciar_lista_mutex_sistema();
	/* crea proceso inicial */
	if (crear_tarea((void *)"init")<0)
		panico("no encontrado el proceso inicial");
	
	/* activa proceso inicial */
	p_proc_actual=planificador();
	cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	panico("S.O. reactivado inesperadamente");
	return 0;
}
int obtener_id_pr(){
	printk("El identificador es: %d \n",p_proc_actual->id);
	return p_proc_actual->id;
}

int dormir (){
	// variable que contendrá los segundos que duerme
	unsigned int segundos;
	// variable que abarcará el nivel de interrupción
	int nivel;
	// proceso anterior
	BCP * p_proc_anterior;
	// obtenemos el número de segundos que duerme
	segundos=(unsigned int)leer_registro(1);	
	// le damos a su atributo el tiempo
	p_proc_actual->tiempo_dormir = segundos*TICK;		
	// bloqueamos al proceso	
	// notificamos por pantalla que el proceso es bloqueado
	printk ("proceso actual %d dormido por %u ticks\n", p_proc_actual->id, p_proc_actual->tiempo_dormir);
	// el proceso anterior copia los atributos del actual
	cambio_pr(&lista_dormidos);
	return 0;	
}

 





void cambio_pr(lista_BCPs *lis){ //Método que cambia de proceso y lo coloca en la lista correspondiente, ya sea en la de bloqueados de un mutex o cualquier otra
	BCP * p_proc_anterior;
	
	int nivel;

	p_proc_anterior=p_proc_actual;	
	nivel=fijar_nivel_int(NIVEL_3);

	//El proceso no sigue. Si hubiera alguna replanificación pendiente
	//hay que desactivarla puesto que ya se está haciendo 


	//Se usa eliminar_elem ya que proc. actual no tiene porque ser el 1º
	eliminar_elem(&lista_listos, p_proc_actual);

	/* Si se ha especificado una lista destino para el BCP, se inserta.
	   Si lista_destino != &lista_listos -> c.contexto voluntario */
	if (lis)
		insertar_ultimo(lis, p_proc_anterior);
		if (lis != &lista_listos)
			/* C. contexto voluntario -> estado=BLOQUEADO */
			p_proc_actual->estado=BLOQUEADO;
		

	p_proc_actual=planificador();
	p_proc_actual->rodaja=TICKS_POR_RODAJA;
	/* Si el proceso ya ha terminado, no se salva y se libera la pila */
	if (p_proc_anterior->estado==TERMINADO) {
		liberar_pila(p_proc_anterior->pila);
		cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
	}
	else
	cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));

	fijar_nivel_int(nivel);
	
	
}

mutex* buscar_mutex(char *nombre){ //Buscar un mutex por nombre

 for(int i=0;i<NUM_MUT;i++){	
  if(lista_mutex.contador_mutex>=0){    
    if(strncmp(lista_mutex.lista[i].nombre_mutex,nombre,MAX_NOM_MUT)==0){
      return &lista_mutex.lista[i];      
    }
}
}
return NULL;
}


int nombre_valido(char *nombre){//comprueba si el nombre del mutex es correcto y no se produce excepción por ser demasiado largo
  if(strlen(nombre)<=MAX_NOM_MUT){
    return 0;} 
}

int buscar_descriptor_BCP(BCP *proc){  
  for(int i=0;i<NUM_MUT_PROC;i++){    
    if(proc->descriptores_mutex_sistema[i]==NULL){    
    return i;    
  }  
}
  return -6;
}


mutex *buscar_mutex_sistema(){ //Busca un mutex que esté vacío sin usarse
  
 for(int i=0;i<NUM_MUT;i++){
 if(lista_mutex.lista[i].num_procesos<=0){
 return &lista_mutex.lista[i];  //Devuelvo la referencia con el & porque es justo ese mutex de la lista el que quiero
}  

} 
  return NULL; //no hay ninguno libre
}

int crear_mutex(){ 
	char *nombre;
	int tipo,nivel;
	int descriptor;
	int bucle;
	mutex* mut;  
	
	nombre=(char*)leer_registro(1);
	tipo=(int)leer_registro(2);
	

	
      if(p_proc_actual->numero_mutex==NUM_MUT_PROC){
	
	return -7;
	
	
	
      }
      if(nombre_valido(nombre)!=0){
	  
    return -2;
	}
	
	if(buscar_mutex(nombre)!=NULL){ //Compruebo que no exista un mutex con ese nombre
	  
      return -3;
	  
	}
	
	
	
	  
	 
	    
	  while(lista_mutex.contador_mutex==NUM_MUT){
	    
	  
	    
	   
	     bucle=1;
	       
	  cambio_pr(&lista_mutex.bloqueados_en_espera);
	     
	     
	   
	     
	   
	    
	    
	  }
	  
	
	  
	      
	  if( bucle ==1){
	    //Volvemos a comprobar el nombre por salir del bucle anterior
	  if(buscar_mutex(nombre)!=0){ //Compruebo que no exista un mutex con ese nombre
	  
     // printk("Ya existe un mutex con ese nombre");
      return -4;
	  
	  
	} 
	    
	  }
      
	  
      descriptor=buscar_descriptor_BCP(p_proc_actual);
	  
      p_proc_actual->numero_mutex++;
      
      mut=buscar_mutex_sistema();
      
      p_proc_actual->descriptores_mutex_sistema[descriptor]=mut;
      
      
   
      
      mut->num_procesos++;
      
      mut->tipo_mutex=tipo;
      
      mut->valor=MUTEX_DESBLOQUEADO;
      
      
      strcpy(mut->nombre_mutex, nombre);
      
      
      
      mut->lista_bloqueados.primero=NULL;
      
      mut->lista_bloqueados.ultimo=NULL;
      
      lista_mutex.contador_mutex++;
      
      
      
      return descriptor;
}


int lock(){  
 BCP*p_proc_anterior;
	int bloqueado;
	int nivel;
	mutex*mut;
	unsigned int mutexid = (unsigned int)leer_registro(1);
	
	
	if(p_proc_actual->descriptores_mutex_sistema[mutexid]==NULL){
	  return -12;
	}
	  mut=p_proc_actual->descriptores_mutex_sistema[mutexid];
	do {
		bloqueado = 0;
		if(mut->num_procesos > 0) {
			//Verificamos si esta o no esta bloqueado
			if(mut->valor>0) {
				//Comprobamos si es RECURSIVO
				if(mut->tipo_mutex == RECURSIVO) {
					//Comprobamos si es el dueño
					if(mut->BCP_id_lock == p_proc_actual->id) {
						//Aumentamos el numero de bloqueos en el mutex
						mut->valor++;
					}
					// Si no, bloqueamos al proceso
					else {
						p_proc_actual->estado = BLOQUEADO;
						// Ya no se debe hacer C. de contexto involuntario
						
						nivel = fijar_nivel_int(NIVEL_3);

						eliminar_primero(&lista_listos);
						//Lo insertamos en la lista de bloqueados por un lock
						insertar_ultimo(&mut->lista_bloqueados, p_proc_actual);
						//Hacemos un C de Contexto
						p_proc_anterior = p_proc_actual;
						p_proc_actual = planificador();

						//printk("*** C de CONTEXTO POR UN LOCK: de %d a %d\n",
						//p_proc_anterior->id, p_proc_actual->id);
						
						//Restauramos el contexto del nuevo actual
						cambio_contexto(&(p_proc_anterior->contexto_regs), &(p_proc_actual->contexto_regs));
						fijar_nivel_int(nivel);
						//Ahora indicamos que hay que volver a comprobar para que no se 
						//cuele ningun proceso
						bloqueado = 1;
					}
				}
				else if(mut->tipo_mutex == NO_RECURSIVO) {
					//Vemos si es el dueño del bloqueo
					if(mut->BCP_id_lock == p_proc_actual->id) {
						//Comprobamos si ya hay un proceso bloqueandolo
						if(mut->BCP_id_lock == p_proc_actual->id) {
							//Si es asi, capturamos el error. Ya que se produciria interbloqueo
							//printk("ERROR: se esta produciendo un caso de interbloqueo trivial\n");
							return -1;
						}
						//En caso contrario bloqueamos el mutex
						mut->valor++;
					}
					//Si no es el dueño bloqueamos al proceso
					else {
						p_proc_actual->estado = BLOQUEADO;
						// Ya no es necesario hacer cambio de contexto involuntario
						
						nivel = fijar_nivel_int(NIVEL_3);
						
						eliminar_primero(&lista_listos);
						insertar_ultimo(&mut->lista_bloqueados, p_proc_actual);
						//Hacemos un C de Contexto
						p_proc_anterior = p_proc_actual;
						p_proc_actual = planificador();

						//printk("*** C de CONTEXTO POR UN LOCK: de %d a %d\n",
							//p_proc_anterior->id, p_proc_actual->id);
						//Restauramos el contexto del nuevo actual
						cambio_contexto(&(p_proc_anterior->contexto_regs),
							&(p_proc_actual->contexto_regs));
						fijar_nivel_int(nivel);

						//Indamos que hay que volver a comprobar para que no se cuele
						//ningun proceso
						bloqueado = 1;
					}
				}
			}
			else if(mut->valor == 0) {
				//Hacemos que el proceso actual pase a ser el nuevo propietario
				//Bloqueamos al mutex
				mut->valor++;
				mut->BCP_id_lock = p_proc_actual->id;
			}
			else {
				//printk("ERROR: error interno en el mutex");
				return -1;
			}
		}
		else {
			//printk("ERROR: se esta intentando bloquar un mutex que aun no ha sido abierto");
			return -1;
		}
	}
	while (bloqueado);
	return 0;
     
    }
    
    
  unlock(){
    BCP*aux;
    int nivel;
    unsigned int mutexid = (unsigned int)leer_registro(1);
    mutex* mut;
    
    if(p_proc_actual->descriptores_mutex_sistema[mutexid]==NULL){
      
      return -18;
    }
    mut=p_proc_actual->descriptores_mutex_sistema[mutexid];
	//verificamos que existe el mutex
	if(mut->num_procesos > 0) {
		//Comprobamos si esta bloqueado
		if(mut->valor > 0) {
			//Comprobamos el tipo
			if(mut->tipo_mutex == RECURSIVO) {
				//Comprobamos si es el dueno del bloqueo
				if(mut->BCP_id_lock == p_proc_actual->id) {
					//Disminuimos el numero de bloqueos
					mut->valor--;
					if(mut->valor == 0) {
						aux = mut->lista_bloqueados.primero;
						//Verificamos si hay algun proc en espera
						if(aux != NULL) {
							//Despertamos al proceso bloqueado
							aux->estado = LISTO;
							nivel = fijar_nivel_int(NIVEL_3);
							eliminar_primero(&mut->lista_bloqueados);
							insertar_ultimo(&lista_listos, aux);
							fijar_nivel_int(nivel);
						}
					}
				}
				//En caso contrario, capturamos el error
				else {
					//printk("ERROR: mutex tiene que ser boqueado por el mismo proceso\n");
				}
			}
			//En caso de NO_RECURSIVO
			else if(mut->tipo_mutex == NO_RECURSIVO) {
				//Verificamos si es el dueno
				if(mut->BCP_id_lock == p_proc_actual->id) {
					//Desbloqueamos
					mut->valor--;
					if(mut->valor != 0) {
						//printk("ERROR: intento de desbloqueo del mutex no recursivo ha fallado\n");
						return -1;
					}
					aux = mut->lista_bloqueados.primero;
					//Verificamos si hay algun proceso en espera
					if(aux != NULL) {
						aux->estado = LISTO;
						nivel = fijar_nivel_int(NIVEL_3);
						eliminar_primero(&mut->lista_bloqueados);
						insertar_ultimo(&lista_listos, aux);
						fijar_nivel_int(nivel);
					}
				}
				else {
					//printk("ERROR: mutex tiene que ser boqueado por el mismo proceso\n");
				}
			}
		}
		//En caso de no estar bloqueado
		else if(mut->valor == 0) {
			//printk("ERROR: un mutex no bloqueado no puede ser desbloqueado");
		}
		else {
			//printk("ERROR: error interno en el mutex");
			return -1;
		}
	}
	else {
		//printk("ERROR: no se puede desbloquear un mutex que no ha sido abierto");
		return -1;
	}
	return 0;
    
  }

int cerrar_mutex(){
 int mutexid; 
 mutexid=(int)leer_registro(1); 
 cerrar_mutex_aux(mutexid,p_proc_actual);
}

 int cerrar_mutex_aux(int mutexid,BCP* proc){   

  mutex* mut;
  int nivel;
  if(proc->descriptores_mutex_sistema[mutexid]==NULL){
    
    return -1;
  }
  
  mut=p_proc_actual->descriptores_mutex_sistema[mutexid];
  proc->numero_mutex--;
  proc->descriptores_mutex_sistema[mutexid]=NULL;
  
  if(mut->valor>0){
    
    mut->valor=0;
  
  BCP* aux = mut->lista_bloqueados.primero;
		if(aux != NULL) {
			aux->estado = LISTO;
			nivel = fijar_nivel_int(NIVEL_3);
			eliminar_primero(&mut->lista_bloqueados);
			insertar_ultimo(&lista_listos,aux);
			fijar_nivel_int(nivel);
  }
  }
  if(mut->BCP_id_lock == p_proc_actual->id) {
	
		mut->valor = 0;
		BCP *aux = mut->lista_bloqueados.primero;
		if(aux != NULL) {
			aux->estado = LISTO;
			nivel = fijar_nivel_int(NIVEL_3);
			eliminar_elem(&mut->lista_bloqueados,aux);
			insertar_ultimo(&lista_listos,aux);
			fijar_nivel_int(nivel);
		}
	}
  if(--mut->num_procesos==0){
   
    lista_mutex.contador_mutex--;
    
    
    if(lista_mutex.bloqueados_en_espera.primero!=NULL){
      
   
      
      BCP *aux;
      BCP *proc;
      proc=lista_mutex.bloqueados_en_espera.primero;
      
         
      aux=proc->siguiente;
      
      proc->estado=LISTO;
      
      nivel=fijar_nivel_int(NIVEL_3);
	
      eliminar_elem(&lista_mutex.bloqueados_en_espera,proc);
      
      insertar_ultimo(&lista_listos,proc);
      
      
     
    
      fijar_nivel_int(nivel);
      
    }
   
  }
  return 0;}

void cerrar_mutex_proceso(BCP* proc){  
  int i;  
  for(i=0; (i<NUM_MUT_PROC); i++){    
    if(proc->descriptores_mutex_sistema[i]!=NULL)
    cerrar_mutex_aux(i,proc);
  }  
}
int abrir_mutex()
{
  

  char* nombre;
  mutex *mut;
  int descriptor;
   
  
  
  nombre=(char*)leer_registro(1);
  
if(nombre_valido(nombre)!=0){ //Compruebo que tenga un nombre introducido valido
 return -8; 
}

if(buscar_mutex(nombre)==NULL){//compruebo que el mutex exista
  return -9;
}


if (p_proc_actual->numero_mutex==NUM_MUT_PROC){ //Compruebo que haya descriptores libres
		
  return -10;

}

mut=buscar_mutex(nombre);



descriptor =  buscar_descriptor_BCP(p_proc_actual);

p_proc_actual->numero_mutex++;

p_proc_actual->descriptores_mutex_sistema[descriptor]=mut;


printk("NOMBRE DESCRIPTOR %s\n",p_proc_actual->descriptores_mutex_sistema[descriptor]->nombre_mutex);
mut->num_procesos++;

	

      return descriptor;
}








