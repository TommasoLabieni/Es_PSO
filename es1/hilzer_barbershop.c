// TODO: Scrivere le funzioni per liberare il posto nel divano (quindi shiftare i posti e inserire quello che e' da piu' tempo in piedi sul divano. Poi scrivere il codice per farsi tagliare i capelli e poi per pagare
// TODO: Scrivere codice del barbiere.
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Costante per definire il numero di barbieri (che definisce anche il numero di clienti servibili concorrentemente */
#define NUM_BARBIERI 3
/* Costante per definire il numero massimo di clienti ammessi all'interno del negozio */
#define NUM_CLIENTI_MAX 20
/* Costante per definire il numero di posti nel divano */
#define NUM_POSTI_DIVANO 4

/* Variabile intera per memorizzare il numero di clienti all'interno del negozio */
int num_clienti_act = 0;
/* Variabile intera per memorizzare il numero di clienti seduti nel divano */
int num_clienti_divano = 0;
/* Array statico per memorizzare la coda di persone nel divano */
int clienti_divano[NUM_POSTI_DIVANO];
/* Array statico per memorizzare la coda di persone in piedi. NOTA: AL MASSIMO IN PIEDI CI POSSONO ESSERE NUM_CLIENTI_MAX - NUM_BARBIERI (che indica anche il numero di clienti che possono essere serviti concorrentemente) - NUM_POSTI_DIVANO, che nelle condizioni specificate dal testo sarebbero: 20 - 3 - 4 = 13.
*/
int clienti_in_piedi[NUM_CLIENTI_MAX - NUM_BARBIERI - NUM_POSTI_DIVANO];
/* Mutex per l'accesso concorrente alle variabili condivise */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/* Variabile condition per bloccare i clienti che non possono sedersi sul divano */
pthread_cond_t C_DIVANO_OCCUPATO = PTHREAD_COND_INITIALIZER;
/* Semaforo per regolare i clienti che vengono serviti. (il cui valore di inizializzazione e' uguale al numero delle sedie disponibili) */
sem_t S_CLIENTI_SERVITI;


/* Funzione enterShop del cliente che entra nel negozio */
void enterShop(int indice) {
	/* NOTA: NON SERVE EFFETTUARE LA LOCK DEL MUTEX, IN QUANTO E' GIA' STATO PRESO PRIMA DELL'INVOCAZIONE DELLA SEGUENTE FUNZIONE */
	/* Variabile intera che memorizza il numero di massimo di persone che ci possono essere in coda */
	int max_num_clienti_coda = NUM_CLIENTI_MAX - NUM_BARBIERI - NUM_POSTI_DIVANO;
	/* Se qui allora il cliente e' entrato nel negozio -> incremento il numero di clienti all'interno del negozio */
	num_clienti_act++;
   printf("Il Thread CLIENTE di indice %d partito con identificatore %lu e' entrato nel negozio.\nIl numero di clienti all'interno e': %d", indice, pthread_self(), num_clienti_act);
	/* Successivamente il cliente si mette nella coda delle persone in attesa */
	for(int i=0; i < max_num_clienti_coda; i++) {
		if (clienti_in_piedi[i] == -1) {
			/* Una volta trovato l'indice in cui inserire il cliente, ne inserisco l'indice del thread */
			clienti_in_piedi[i] = indice;
			/* E termino l'esecuzione */
			return;
		}
	}
}

/* Funzione sitOnSofa del cliente che si siede sul divano */
void sitOnSofa(int indice) {
	/* NOTA: Anche qui non c'e' bisogno di prendere il mutex per lo stesso motivo della funzione enterShop. */
	/* Variabile intera che memorizza il numero di massimo di persone che ci possono essere in coda */
	int max_num_clienti_coda = NUM_CLIENTI_MAX - NUM_BARBIERI - NUM_POSTI_DIVANO;
	num_clienti_divano++;
   printf("Il Thread CLIENTE di indice %d partito con identificatore %lu si e' seduto nel divano.\nIl numero di clienti seduti e': %d", indice, pthread_self(), num_clienti_divano);
	/* A questo punto occorre inserirlo nella coda delle persone sedute sul divano */
	for (int i=0; i < NUM_POSTI_DIVANO; i++) {
		if (clienti_divano[i] == -1) {
			/* Se il posto i-esimo del divano e' libero, allora il cliente si siede (ossia inserisce l'indice del thread cliente nel posto i-esimo). */
			clienti_divano[i] = indice;
			/* E termino il ciclo */
			break;
		}
	}

	/* A questo punto bisogna liberare il posto in piedi occupato precedentemente dal cliente */

	for (int i=0 ;i < max_num_clienti_coda; i++) {
		/* Prima cerco la posizione del cliente */
		if (clienti_in_piedi[i] == indice) {
			/* Una volta trovato il cliente libero il suo posto */
			clienti_in_piedi[i] = -1;
			/* E shifto all'indietro l'array */
			for (int j=i; j < (max_num_clienti_coda - 1); i++) {
				clienti_in_piedi[j] = clienti_in_piedi[j+1];
			}
			/* Una volta shiftato l'array, termino l'esecuzione */
			return;
		}
	}
	
}

void *eseguiCliente(void *id)
{
   int *pi = (int *)id;
   int *ptr;
   
   ptr = (int *) malloc(sizeof(int));
   if (ptr == NULL)
   {
   	perror("Problemi con l'allocazione di ptr\n");
      exit(-1);
   }
   *ptr = *pi;

   printf("Thread CLIENTE di indice %d partito: Ho come identificatore %lu\n", *pi, pthread_self());
	/* Prima che il cliente possa entrare nel negozio, occorre verificare il numero di clienti dentro il negozio */
	pthread_mutex_lock(&mutex);
	while (num_clienti_act > NUM_CLIENTI_MAX) {
		/* In tal caso si addormenta per qualche istante sperando di trovare posto piu' avanti */
		/* Si rilascia il mutex per evitare deadlock */
		pthread_mutex_unlock(&mutex);
		sleep(1);
	}
	/* E poi il cliente entra all'interno del negozio */
	enterShop(*ptr);
	/* Una volta entrato, prova a sedersi nel divano (se libero) */
	while (num_clienti_divano > NUM_POSTI_DIVANO) {
		/* Se il divano e' pieno, allora il cliente deve aspettare */
		pthread_cond_wait(&C_DIVANO_OCCUPATO, &mutex);
	}
	/* Se qui allora il cliente puo' sedersi sul divano */
	sitOnSofa(*ptr);
	/* Una volta seduto nel divano, il clienti si mette in attesa di essere servito */
	pthread_mutex_unlock(&mutex);
	sem_wait(&S_CLIENTI_SERVITI);	
	/* Se qui allora il cliente si siede e deve rimpossessarsi del mutex e farsi tagliare i capelli */
	pthread_mutex_lock(&mutex);
	/* Prima si libera il posto nel divano */
	liberaPostoDivano(*ptr);
	
	
   pthread_exit((void *) ptr);
}

int main (int argc, char **argv)
{
   pthread_t *thread;
   int *taskids;
   int i;
   int *p;
   int NUM_THREADS;
   char error[250];

   /* Controllo sul numero di parametri */
   if (argc != 2 ) /* Deve essere passato esattamente un parametro */
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

   for (i=0; i < NUM_THREADS; i++)
   {
		taskids[i] = i;
   	printf("Sto per creare il thread %d-esimo\n", taskids[i]);
     	if (pthread_create(&thread[i], NULL, eseguiCliente, (void *) (&taskids[i])) != 0)
      {
      	sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo\n", taskids[i]);
         perror(error);
			exit(5);
      }
		printf("SONO IL MAIN e ho creato il Pthread %i-esimo con id=%lu\n", i, thread[i]);
   }

   for (i=0; i < NUM_THREADS; i++)
   {
		int ris;
		/* attendiamo la terminazione di tutti i thread generati */
   	pthread_join(thread[i], (void**) & p);
		ris= *p;
		printf("Pthread %d-esimo restituisce %d\n", i, ris);
   }
 
   exit(0);
}


