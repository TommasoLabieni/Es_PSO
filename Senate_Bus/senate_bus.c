#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

/* Costante per indicare la capacita' del bus */
#define C 50

/* Variabile intera per memorizzare il numero dei rider in attesa del bus */
int num_rider_attesa = 0;
/* Variabile intera per memorizzare il numero dei rider saliti sul bus NOTA: QUESTA VARIABILE NON HA UNO SCOPO FUNZIONALE PER IL PROGRAMMA, MA E' UTILIZZATA SOLO PER DEBUG*/
int num_rider_saliti = 0;
/* Variabile intera per memorizzare il numero dei rider giunti alla fermata del bus */
int num_rider_bus_stop = 0;
/* Variabile boolean che indica se l'autobus e' arrivato e sta facendo salire i passeggeri (utilizzata per bloccare i passeggeri che arrivano in ritardo */
bool bus_pronto = false;
/* Variabile booleana che indica se l'ultimo rider in attesa e' salito sul bus */
bool ultimo_rider_salito = false;
/* Mutex per utilizzare le variabili condivise */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/* Variabile condition sulla quale si bloccheranno i riders che arrivano in ritardo (ossia quando l'autobus e' gia' arrivato) */
pthread_cond_t C_LATE_RIDER = PTHREAD_COND_INITIALIZER;
/* Variabile condition sulla quale si blocchera' l'autobus in attesa che siano saliti tutti i rider (tranne quelli in ritardo) */
pthread_cond_t C_LAST_RIDER_ON = PTHREAD_COND_INITIALIZER;
/* Variabile Condition per segnalare l'arrivo del bus */
pthread_cond_t C_BUS = PTHREAD_COND_INITIALIZER;
/* Variabile Condition per evitare che il numero di rider saliti sul bus superi la capacita' C di quest'ultimo */
pthread_cond_t C_BUS_CAPACITY = PTHREAD_COND_INITIALIZER;

/* Funzione boardBus del passeggero che sale sul Bus */
void boardBus(int indice) {
	/* NOTA: Il mutex era gia' stato preso dal thread RIDER corrente. */
   printf("Il thread RIDER di indice %d con identificatore %lu e' salito sul bus.\n", indice, pthread_self());
	/* Decremento il numero di passeggeri in attesa del bus (in quanto  il bus è arrivato)*/
	num_rider_attesa--;
	/* Decremento la variabile che conta il numero di riders alla fermata del bus */
	num_rider_bus_stop--;
	/* Incremento il numero dei rider saliti sul bus. */
	num_rider_saliti++;
	/* Se questo numero e' = 0 allora e' salito l'ultima persona in attesa -> segnalo all'autobus che puo' partire */
	if (num_rider_attesa == 0) {
		ultimo_rider_salito = true;
		pthread_mutex_unlock(&mutex);
		/* Risveglio il bus */
		pthread_cond_signal(&C_LAST_RIDER_ON);
		return;
	}
	pthread_mutex_unlock(&mutex);
}

void *eseguiRider(void *id)
{
   int *pi = (int *)id;
   int *ptr;
   
   ptr = (int *) malloc( sizeof(int));
   if (ptr == NULL)
   {
	   perror("Problemi con l'allocazione di ptr\n");
	   exit(-1);
   }
   *ptr = *pi;

   printf("Thread RIDER di indice %d partito: Ho come identificatore %lu\n", *pi, pthread_self());

	/* Se il rider prova a salire sul bus ma la capacita' e' stata colmata, allora il rider si blocchera' sulla var condition C_BUS_CAPACITY */
	pthread_mutex_lock(&mutex);
	while(num_rider_bus_stop >= C) {
   	printf("Il thread RIDER di indice %d con identificatore %lu e' bloccato a causa del num dei passeggeri.\n", *ptr, pthread_self());
		pthread_cond_wait(&C_BUS_CAPACITY, &mutex);
	}
	/* A questo punto incremento la variabile che conta il numero di riders giunti alla fermata del bus */
	num_rider_bus_stop++;
	/* Prima di mettersi in attesa, occorre verificare se l'autobus sta gia' facendo salire i passeggeri */
	while(bus_pronto) {
   	printf("Il thread RIDER di indice %d con identificatore %lu e' bloccato a causa dell'autobus arrivato .\n", *ptr, pthread_self());
		pthread_cond_wait(&C_LATE_RIDER, &mutex);
	}
	/* A questo punto incremento la variabile che conta il numero di riders in attesa del bus */
	num_rider_attesa++;
	
   printf("Il thread RIDER di indice %d con identificatore %lu e' in attesa del bus.\n", *pi, pthread_self());

   while (!bus_pronto) {
	   /* Il rider aspetta l'arrivo del bus */
	   pthread_cond_wait(&C_BUS, &mutex);
   } 
	/* Bus arrivato --> il rider invoca boardBus */
	boardBus(*ptr);
	
   pthread_exit((void *) ptr);
}

/* Funzione depart del bus che inizia il viaggio */
void depart(int indice) {
	pthread_mutex_lock(&mutex);
	printf("Il thread BUS di indice %d con identificatore %lu e' partito.\n", indice, pthread_self()); 
	printf("Il numero dei rider saliti e': %d\n", num_rider_saliti);
	num_rider_saliti = 0;
	/* Si resetta la variabile che blocca i rider che sono arrivati in ritardo */
	bus_pronto = false;
	/* Si resetta la variabile che blocca il bus in attesa che salga l'ultimo passeggero */
	ultimo_rider_salito = false;
	/* Si risvegliano tutti i rider arrivati in ritardo bloccati */
	pthread_cond_broadcast(&C_LATE_RIDER);
	/* Si risvegliano tutti i rider bloccati a causa della capacita' */
	pthread_cond_broadcast(&C_BUS_CAPACITY);
	pthread_mutex_unlock(&mutex);
}

void *eseguiBus(void *id)
{
   int *pi = (int *)id;
   int *ptr;
   
   ptr = (int *) malloc( sizeof(int));
   if (ptr == NULL)
   {
   	perror("Problemi con l'allocazione di ptr\n");
      exit(-1);
   }
   *ptr = *pi;
   printf("Thread BUS di indice %d partito: Ho come identificatore %lu\n", *pi, pthread_self());

	while(1) {
		/* L'autobus aspetta qualche istante di tempo in modo tale da consentire ai thread dei passeggeri di arrivare nell'area di attesa */
		pthread_mutex_lock(&mutex);
		printf("L'autobus e' arrivato. Chiunque arrivi da ora in poi dovra' aspettare la corsa successiva.\n");
		/* Viene settata la variabile che blocca i rider arrivati in ritardo */
		bus_pronto = true;
		/* Si controlla se vi sono rider in attesa */
		if (num_rider_attesa > 0) {
			/* Si segnala ai rider in attesa l'arrivo del bus */
			pthread_cond_broadcast(&C_BUS);
			/* A questo punto il bus aspetta che salga l'ultimo rider */
			while(!ultimo_rider_salito) {
				pthread_cond_wait(&C_LAST_RIDER_ON, &mutex);
			}
		} else {
			printf("L'autobus fa una corsa senza passeggeri\n");
		}
		pthread_mutex_unlock(&mutex);
		/* Infine si procede a partire */
		depart(*ptr);
	}
}

int main (int argc, char **argv)
{
   pthread_t *thread;
   int *taskids;
   int i;
	int bus_i;
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

	/* Si incrementa NUM_THREADS per considerare anche il thread del bus */
	NUM_THREADS++;

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

	/* Il thread BUS parte da una posizione casuale per garantire un comportamento del programma diverso per ogni esecuzione */
	srand(time(NULL));
	bus_i = rand() % NUM_THREADS;
   for (i=0; i < NUM_THREADS; i++)
   {
		taskids[i] = i;
		if (i != bus_i) {
   		printf("Sto per creare il thread %d-esimo (rider)\n", taskids[i]);
     		if (pthread_create(&thread[i], NULL, eseguiRider, (void *) (&taskids[i])) != 0)
      	{
      		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (rider)\n", taskids[i]);
      	   perror(error);
				exit(7);
      	}
			printf("SONO IL MAIN e ho creato il Pthread %i-esimo con id=%lu (rider)\n", i, thread[i]);
		} else {
   		printf("Sto per creare il thread %d-esimo (bus)\n", taskids[i]);
     		if (pthread_create(&thread[i], NULL, eseguiBus, (void *) (&taskids[i])) != 0)
      	{
      		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (bus)\n", taskids[i]);
      	   perror(error);
				exit(8);
      	}
			printf("SONO IL MAIN e ho creato il Pthread %i-esimo con id=%lu (bus)\n", i, thread[i]);
		}
   }

	/* Si attendono tutti e SOLI i thread dei passeggeri */
   for (i=0; i < NUM_THREADS; i++)
   {
		int ris;
		/* se i == bus_i allora si sta considerando il thread del bus -> si passa al thread successivo (se presente) */
		if (i == bus_i)
			continue;
		/* attendiamo la terminazione di tutti i thread generati */
   	pthread_join(thread[i], (void**) & p);
		ris= *p;
		printf("Pthread %d-esimo restituisce %d\n", i, ris);
   }
 
   exit(0);
}


