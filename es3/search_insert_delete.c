#include "include/list.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

/* Semaforo per i searcher */
sem_t S_SEARCHER;
/* Semaforo per gli inserter */
sem_t S_INSERTER;
/* Semaforo per i deleter */
sem_t S_DELETER;
/* mutex per accedere alle variabili globali (ad eccezione della lista) */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/* mutex per evitare di avere piu' di uno scrittore attivo */
pthread_mutex_t inserter_mutex = PTHREAD_MUTEX_INITIALIZER;

/* ********** VARIABILI GLOBALI ********** */

/* Singly-Linked List condivisa */
Lista l = NULL;
/* Variabile intera che indica il numero di elementi aggiunti nella lista */
int num_elementi = 0;
/* Variabile intera che indica il numero di searcher attivi */
int searcher_attivi = 0;
/* Variabile intera che indica il numero di searcher bloccati */
int searcher_bloccati = 0;
/* Variabile booleana che indica se vi è un inserter attivo */
bool inserter_attivo = false;
/* Variabile intera che indica il numero di inserter bloccati */
int inserter_bloccati = 0;
/* Variabile booleana che indica se vi è un deleter attivo */
bool deleter_attivo = false;
/* Variabile intera che indica il numero di deleter bloccati */
int deleter_bloccati = 0;
/* Varibiabile booleana per evitare starvation dei deleter */
bool deleter_trigger = false;

/* *************************************** */

void prologo_searcher(int indice) {
	pthread_mutex_lock(&mutex);
	/* Se non vi e' alcun deleter attivo o se il trigger per evitare starvation non e' attivo, allora il searcher puo' eseguire */
	if (!deleter_attivo && !deleter_trigger) {
		searcher_attivi++;
		sem_post(&S_SEARCHER);
	} else {
		searcher_bloccati++;
		if (deleter_attivo)
   		printf("Il thread Searcher di indice %d con identificatore %lu e' bloccato a causa di del attivo\n", indice, pthread_self());
		if (deleter_trigger)
   		printf("Il thread Searcher di indice %d con identificatore %lu e' bloccato a causa di del trigger\n", indice, pthread_self());
	}
	pthread_mutex_unlock(&mutex);
	sem_wait(&S_SEARCHER);
}

void epilogo_searcher(int indice) {
	pthread_mutex_lock(&mutex);
	searcher_attivi--;
	/* Se il searcher che ha terminato la propria esecuzione e' L'ULTIMO
	 *	allora questo andra' a svegliare un deleter (se ve ne e' almeno uno
	 *	bloccato)
	*/
	if (searcher_attivi == 0) {
		if (deleter_bloccati > 0) {
			/* Prima di risvegliare un deleter, devo accertarmi che non ci sia alcun inserter in esecuzione */
			if (!inserter_attivo) {
				/* dal momento che un deleter e' stato risvegliato, e' possibile disattivare il trigger per evitare starvation */
				deleter_trigger = false;
				deleter_bloccati--;
				deleter_attivo = true;
				sem_post(&S_DELETER);
			} else {
				/* In caso contrario attivo la variabile per evitare starvation */
				deleter_trigger = true;
			}
		} 
	} else if (deleter_bloccati > 0) {
		/* Se iniziano ad accodarsi dei deleter, allora viene attivato il trigger per evitare starvation */
		deleter_trigger = true;
	}
	/* else if (searcher_bloccati > 0) {
		searcher_bloccati--;
		searcher_attivi++;
		sem_post(&S_SEARCHER);
	} else if ((inserter_bloccati > 0) && (!inserter_attivo)) {
		inserter_bloccati--;
		inserter_attivo = true;
		sem_post(&S_INSERTER);
	}*/
	pthread_mutex_unlock(&mutex);
}

void *esegui_searcher(void *id) {
   int *pi = (int *)id;
   int *ptr;

   ptr = (int *) malloc(sizeof(int));
   if (ptr == NULL)
   {
        perror("Problemi con l'allocazione di ptr\n");
        exit(-1);
   }
   *ptr = *pi;

   printf("Thread Searcher di indice %d partito: Ho come identificatore %lu\n", *pi, pthread_self());
	prologo_searcher(*ptr);
   printf("Il thread Searcher di indice %d con identificatore %lu e' nella sua sezione critica\n", *pi, pthread_self());
	/* Invocazione della funzione per esaminare la lista */
	esamina_lista(l, *ptr);
   printf("Il thread Searcher di indice %d con identificatore %lu terminato. %d rimanenti\n", *pi, pthread_self(), (searcher_attivi-1));
	epilogo_searcher(*ptr);
   pthread_exit((void *) ptr);
}

void prologo_inserter(int indice) {
	pthread_mutex_lock(&mutex);
	/* Se non e' attivo nessun thread di tipo deleter o inserter, allora l'attuale thread di tipo inserter puo' eseguire */
	if (!(deleter_attivo) && !(inserter_attivo)) {
		inserter_attivo = true;
		sem_post(&S_INSERTER);
	} else {
		if (deleter_attivo)
   		printf("Il thread Inserter di indice %d con identificatore %lu e' bloccato a causa di del attivo\n", indice, pthread_self());
		if (inserter_attivo)
   		printf("Il thread Inserter di indice %d con identificatore %lu e' bloccato a causa di ins attivo\n", indice, pthread_self());
		inserter_bloccati++;
	}
	pthread_mutex_unlock(&mutex);
	sem_wait(&S_INSERTER);
}

void epilogo_inserter(int indice) {
	pthread_mutex_lock(&mutex);
	inserter_attivo = false;
	if (deleter_bloccati != 0) {
		/* Se il numero di lettori attivi e' > 0, allora il deleter non puo' svegliarsi -> abilito il trigger per evitare la starvation del deleter */
		if (searcher_attivi > 0)
			deleter_trigger = true;
		else {
			/* dal momento che un deleter e' stato risvegliato, e' possibile disattivare il trigger per evitare starvation */
			deleter_trigger = false;
			deleter_bloccati--;
			deleter_attivo = true;
			sem_post(&S_DELETER);
		}
	} else if (inserter_bloccati > 0) {
		inserter_bloccati--;
		inserter_attivo = true;
		sem_post(&S_INSERTER);
	} 
	pthread_mutex_unlock(&mutex);
}

void *esegui_inserter(void *id) {
   int *pi = (int *)id;
   int *ptr;
	int r, ret;
   ptr = (int *) malloc( sizeof(int));
   if (ptr == NULL)
   {
        perror("Problemi con l'allocazione di ptr\n");
        exit(-1);
   }
   *ptr = *pi;

   printf("Thread Inserter di indice %d partito: Ho come identificatore %lu\n", *pi, pthread_self());
	prologo_inserter(*ptr);
   printf("Il thread Inserter di indice %d con identificatore %lu e' nella sua sezione critica\n", *pi, pthread_self());
	/* Generazione Casuale di un elemento (numero) da aggiungere alla lista */
	r = rand() % 1000;
	/* Invocazione della funzione per aggiungere un elemento alla lista */
	ret = aggiungi_elemento(&l, r, *ptr);
	epilogo_inserter(*ptr);
	/* Se e' avvenuto un errore in fase di aggiunta del nuovo elemento, verra' ritornato un valore di ritorno (-1)
	 * che verra' interpretato come un errore
	*/
	if (ret != 0) {
		*ptr = -1;
	} else {
		/* Altrimenti aggiorno in numero di elementi presenti nella lista */
		pthread_mutex_lock(&mutex);
		num_elementi++;
		pthread_mutex_unlock(&mutex);
	}
   printf("Il thread Inserter di indice %d con identificatore %lu terminato.\n", *pi, pthread_self());
   pthread_exit((void *) ptr);
}

void prologo_deleter(int indice) {
	pthread_mutex_lock(&mutex);
	/* Se non e' attivo nessun'altro thread, allora l'attuale thread di tipo deleter puo' eseguire */
	if ((searcher_attivi == 0) && !(inserter_attivo) && !(deleter_attivo)) {
		deleter_attivo = true;
		/* La variabile per evitare la starvation dei deleter puo' essere disattiva se un deleter andra' in esecuzione */
		deleter_trigger = false;
		sem_post(&S_DELETER);
	} else {
		deleter_bloccati++;
		if (searcher_attivi > 0)
   		printf("Il thread Deleter di indice %d con identificatore %lu e' bloccato a causa di search att\n", indice, pthread_self());
		if (deleter_attivo)
   		printf("Il thread Deleter di indice %d con identificatore %lu e' bloccato a causa di del attivo\n", indice, pthread_self());
		if (inserter_attivo)
   		printf("Il thread Deleter di indice %d con identificatore %lu e' bloccato a causa di ins attivo\n", indice, pthread_self());
	}
	pthread_mutex_unlock(&mutex);
	sem_wait(&S_DELETER);
}

void epilogo_deleter(int indice) {
	pthread_mutex_lock(&mutex);
	deleter_attivo = false;
	if (deleter_bloccati > 0) {
		deleter_bloccati--;
		deleter_trigger = false;
		deleter_attivo = true;
		sem_post(&S_DELETER);
	} else if ((searcher_bloccati > 0) || (inserter_bloccati > 0)) {
		while (searcher_bloccati > 0) {
			searcher_bloccati--;
			searcher_attivi++;
			sem_post(&S_SEARCHER);
		}
		if (inserter_bloccati > 0) {
			inserter_bloccati--;
			inserter_attivo = true;
			sem_post(&S_INSERTER);
		}
	} 
	pthread_mutex_unlock(&mutex);
}

void *esegui_deleter(void *id) {
   int *pi = (int *)id;
   int *ptr;
	int r;	

   ptr = (int *) malloc( sizeof(int));
   if (ptr == NULL)
   {
        perror("Problemi con l'allocazione di ptr\n");
        exit(-1);
   }
   *ptr = *pi;

   printf("Thread Deleter di indice %d partito: Ho come identificatore %lu\n", *pi, pthread_self());
	prologo_deleter(*ptr);
   printf("Il thread Deleter di indice %d con identificatore %lu e' nella sua sezione critica\n", *pi, pthread_self());
	/* Per l'eliminazione viene generato un numero casuale che sara' l'indice
	 *	dell'elemento da rimuovere dalla lista. Questo numero di indice va da 0 al
	 * numero massimo di elementi-1 presenti nella lista (informazione recuperata dalla
	 * variabile num_elementi. NOTA: l'eliminazione avviene SOLO E SOLTANTO
	 *	se la lista NON E' VUOTA! Altrimenti la rand() di num_elementi produrrebbe un errore (floating point exception).
	*/
	if (num_elementi > 0) {
		r = rand() % num_elementi;
   	//printf("Il thread Deleter di indice %d con identificatore %lu va a eliminare %d\n", *pi, pthread_self(), r);
		/* Invocazione della funzione per eliminare un elemento dalla lista */
		elimina_elemento(&l, r, *ptr);
		/* Dopo l'eliminazione occorre aggiornare il numero di elementi presenti nella lista */
		pthread_mutex_lock(&mutex);
		num_elementi--;
		pthread_mutex_unlock(&mutex);
	}
	epilogo_deleter(*ptr);
   printf("Il thread Deleter di indice %d con identificatore %lu terminato.\n", *pi, pthread_self());

   pthread_exit((void *) ptr);
}

int main (int argc, char **argv) {
   pthread_t *thread;
   int *taskids;
   int i;
	int r;
   int *p;
   int NUM_THREADS;
   char error[250];

   /* Controllo sul numero di parametri */
   if (argc != 2) /* Deve essere passato esattamente un parametro */
   {
   	sprintf(error,"Errore nel numero dei parametri %d\n", argc-1);
		perror(error);
      exit(1);
   }

   /* Calcoliamo il numero passato che sara' il numero di Pthread da creare */
   NUM_THREADS = atoi(argv[1]);
   if (NUM_THREADS <= 0) 
   {
   	sprintf(error,"Errore: Il primo parametro non e' un numero strettamente maggiore di 0 ma e' %d\n", NUM_THREADS);
	perror(error);
      exit(2);
   }

   thread=(pthread_t *) malloc(NUM_THREADS * sizeof(pthread_t));
   if (thread == NULL)
   {
      perror("Problemi con l'allocazione dell'array thread\n");
      exit(3);
   }
   taskids = (int *) malloc(NUM_THREADS * sizeof(int));
   if (taskids == NULL)
   {
      perror("Problemi con l'allocazione dell'array taskids\n");
   	exit(4);
   }	

	/* ***** INIZIALIZZAZIONE SEMAFORI ***** */

	if (sem_init(&S_SEARCHER, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo searcher\n");
		exit(5);
	}

	if (sem_init(&S_INSERTER, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo inserter\n");
		exit(6);
	}

	if (sem_init(&S_DELETER, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo deleter\n");
		exit(6);
	}

	/* ************************************************ */

	/* Generazione del seme */
	srand(time(NULL));
   for (i=0; i < NUM_THREADS; i++)
   {
   	taskids[i] = i;
		r = rand() % 3; 
		if((r == 0) || (i == (NUM_THREADS-1))) {
   		//printf("Sto per creare il thread %d-esimo (searcher)\n", taskids[i]);
      	if (pthread_create(&thread[i], NULL, esegui_searcher, (void *) (&taskids[i])) != 0)
      	{
      		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (searcher)\n", taskids[i]);
      	   perror(error);
				exit(5);
      	}
			printf("SONO IL MAIN e ho creato il Pthread %i-esimo (searcher) con id=%lu\n", i, thread[i]);
      }else if (r == 1) {
   		//printf("Sto per creare il thread %d-esimo (inserter)\n", taskids[i]);
      	if (pthread_create(&thread[i], NULL, esegui_inserter, (void *) (&taskids[i])) != 0)
      	{
      		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (inserter)\n", taskids[i]);
      	   perror(error);
				exit(5);
      	}
			printf("SONO IL MAIN e ho creato il Pthread %i-esimo (inserter) con id=%lu\n", i, thread[i]);
		} else {
   		//printf("Sto per creare il thread %d-esimo (deleter)\n", taskids[i]);
      	if (pthread_create(&thread[i], NULL, esegui_deleter, (void *) (&taskids[i])) != 0)
      	{
      		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (deleter)\n", taskids[i]);
      	   perror(error);
				exit(5);
      	}
			printf("SONO IL MAIN e ho creato il Pthread %i-esimo (deleter) con id=%lu\n", i, thread[i]);
		}
   }

   for (i=0; i < NUM_THREADS; i++)
   {
		int ris;
		/* attendiamo la terminazione di tutti i thread generati */
   	pthread_join(thread[i], (void**) & p);
		ris = *p;
		printf("Pthread %d-esimo restituisce %d\n", i, ris);
   }
	
   exit(0);
}


