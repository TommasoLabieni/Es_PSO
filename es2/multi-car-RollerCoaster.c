#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>

/* Variabile per memorizzare il numero di passeggeri */
int n;
/* Variabile per memorizzare il numero di passeggeri che un'auto puo' contenere */
int C;
/* Variabile per memorizzare il numero delle auto */
int m;
/* Variabile per memorizzare l'indice dell'auto che sta facendo salire i passeggeri attualmente */
int car_act_salita = 0;
/* Variabile per memorizzare l'indice dell'auto che sta facendo scendere i passeggeri attualmente */
int car_act_uscita = 0;
/* Mutex per proteggere l'accesso alle variabili condivise */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/* Semaforo Privato per far salire i passeggeri */
sem_t S_CAR_LOAD;
/* Semaforo Privato per far scendere i passeggeri */
sem_t S_CAR_UNLOAD;
/* Semaforo per verificare se un'auto contiene il numero esatto di passeggeri per partire */
sem_t S_PASSEGGERI_SALITI;
/* Semaforo per verificare se TUTTI i passeggeri sono scesi dall'auto */
sem_t S_PASSEGGERI_SCESI;
/* Variabile intera per memorizzare il numero di passeggeri saliti all'interno di un auto */
int num_passeggeri_saliti = 0;
/* Variabile intera per memorizzare il numero di passeggeri scesi da un auto */
int num_passeggeri_scesi = 0;
/* Array dinamico di semafori per gestire l'ordine di ENTRATA nelle auto */
sem_t* S_LOAD_QUEUE;
/* Array dinamico di semafori per gestire l'ordine di USCITA dalle auto */
sem_t* S_UNLOAD_QUEUE;

/* Funzione board del passeggero per salire nell'auto */
void board(int indice) {
	pthread_mutex_lock(&mutex);
   printf("Il Thread PASSEGGERO di indice %d con identificatore %lu e' salito nell'auto di indice %d.\n", indice, pthread_self(), car_act_salita);
	/* il passeggero entra nell'auto -> occorre aggiornare la variabile che memorizza il numero di passeggeri contenuti nell'auto che sta attualmente effetuando il LOADING dei passeggeri */
	num_passeggeri_saliti++;
	/* Se e' stata colmata la capienza C dell'auto, occorre notificarla per farla partire */
	if (num_passeggeri_saliti == C) {
		/* Si risveglia il thread dell'auto */
		pthread_mutex_unlock(&mutex);
		sem_post(&S_PASSEGGERI_SALITI);
		return;
	} else {
		/* Altrimenti si risveglia il prossimo passeggero in attesa */
		pthread_mutex_unlock(&mutex);
		sem_post(&S_CAR_LOAD);
	}
}

/* Funzione unboard del passeggero per scendere dall'auto */
void unboard(int indice) {
	pthread_mutex_lock(&mutex);
   printf("Il Thread PASSEGGERO di indice %d con identificatore %lu e' sceso dall'auto di indice %d.\n", indice, pthread_self(), car_act_uscita);
	/* il passeggero esce dall'auto -> occorre aggiornare la variabile che memorizza il numero di passeggeri scesi dall'auto che sta attualmente effetuando l'UNLOADING dei passeggeri */
	num_passeggeri_scesi++;
	/* Se il passeggero che sta scendendo dall'auto e' l'ultimo occorre notificare quest'ultima per consentire all'auto successiva di effettuare l'unloading */
	if (num_passeggeri_scesi == C) {
		/* Si risveglia il thread dell'auto */
		pthread_mutex_unlock(&mutex);
		sem_post(&S_PASSEGGERI_SCESI);
		return;
	} else {
		/* Altrimenti si risveglia il prossimo passeggero in attesa */
		pthread_mutex_unlock(&mutex);
		sem_post(&S_CAR_UNLOAD);
	}
}

void *eseguiPasseggero(void *id)
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

   printf("Thread %d PASSEGGERO partito: Ho come identificatore %lu\n", *pi, pthread_self());

	while(1) {
		/* Prima di chiamare la funzione board() il passeggero deve attendere che un'auto chiami la funzione load() */
		sem_wait(&S_CAR_LOAD);
		/* il passeggero sale a bordo */
		board(*ptr);
		/* Successivamente il passeggero scende dall'auto, ma prima deve aspettare che l'auto chiami il metodo unload() */
		sem_wait(&S_CAR_UNLOAD);
		/* il passeggero scende dall'auto */
		unboard(*ptr);
		/* Fermo il thread per un breve periodo di tempo per evitare delle stampe sovrapposte. Nel file log_imp_2.txt vi e' mostrato il comportamento del programma senza questa sleep */
		sleep(1);
	}
   pthread_exit((void *) ptr);
}

/* Funzione load dell'auto per consentire ai passeggeri di salire al suo interno */
void load(int indice) {
	int next_indice = (indice + 1) % m;		/* Variabile utilizzata per consentire all'auto successiva di far salire i passeggeri una volta caricati C passeggeri */
	
	/* Come prima cosa l'auto deve aspettare il suo turno per far salire i passeggeri */
	sem_wait(&S_LOAD_QUEUE[indice]);
	pthread_mutex_lock(&mutex);
	/* Aggiorno l'indice dell'auto che sta facendo salire i passeggeri */
	car_act_salita = indice+n;

   printf("Thread AUTO di indice %d con identificatore %lu inizia a far salire i passeggeri\n", indice+n, pthread_self());
	pthread_mutex_unlock(&mutex);
	/* Risveglio del passeggero in attesa di salire nell'auto */
	sem_post(&S_CAR_LOAD);
	/* A questo punto l'auto rimane in attesa che tutti e C i passeggeri siano saliti */
	sem_wait(&S_PASSEGGERI_SALITI);
	pthread_mutex_lock(&mutex);

	/* Si resetta il numero dei passeggeri saliti per preparare la salita nell'auto successiva */
	num_passeggeri_saliti = 0;
   printf("Thread AUTO di indice %d con identificatore %lu ha caricato tutti i %d passeggeri\n", indice+n, pthread_self(), C);

	/* A questo punto si notifica la prossima auto che puo' caricare i passeggeri */
	sem_post(&S_LOAD_QUEUE[next_indice]);

	pthread_mutex_unlock(&mutex);
}

/* Funzione run dell'auto che simula il giro sul tracciato dell'auto */
void run(int indice) {
   printf("Thread AUTO di indice %d con identificatore %lu sta girando nel tracciato\n", indice+n, pthread_self());
	//sleep(4);
}

void unload(int indice) {
	int next_indice = (indice + 1) % m;		/* Variabile utilizzata per consentire all'auto successiva di far scende i passeggeri una volta scesi C passeggeri */
	
	/* Come prima cosa l'auto deve aspettare il suo turno per far scendere i passeggeri */
	sem_wait(&S_UNLOAD_QUEUE[indice]);
	pthread_mutex_lock(&mutex);

	/* Aggiorno l'indice dell'auto che sta facendo scendere i passeggeri */
	car_act_uscita = indice+n;

   printf("Thread AUTO di indice %d con identificatore %lu inizia a far scendere i passeggeri\n", indice+n, pthread_self());
	pthread_mutex_unlock(&mutex);

	/* Risveglio del passeggero in attesa di scendere dall'auto */
	sem_post(&S_CAR_UNLOAD);
	/* A questo punto l'auto rimane in attesa che tutti e C i passeggeri siano scesi */
	sem_wait(&S_PASSEGGERI_SCESI);

	pthread_mutex_lock(&mutex);
	/* Si resetta il numero dei passeggeri scesi per preparare l'uscita dall'auto successiva */
	num_passeggeri_scesi = 0;
   printf("Thread AUTO di indice %d con identificatore %lu ha fatto scendere tutti i %d passeggeri\n", indice+n, pthread_self(), C);


	/* A questo punto si notifica la prossima auto che puo' far scendere i passeggeri */
	sem_post(&S_UNLOAD_QUEUE[next_indice]);

	pthread_mutex_unlock(&mutex);
}

void *eseguiAuto(void *id)
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

   printf("Thread %d AUTO partito: Ho come identificatore %lu\n", *pi+n, pthread_self());

	while(1) {
		/* Come prima cosa l'auto effettua il load dei passeggeri */
		load(*ptr);
		/* Una volta caricati tutti e C i passeggeri, l'auto puo' partire */
		run(*ptr);
		/* Terminato il giro, l'auto fa scendere i passeggeri */
		unload(*ptr);
	}
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
   if (argc != 4) /* Deve essere passato esattamente un parametro */
   {
   	sprintf(error,"Errore nel numero dei parametri. Inserire:\n \
	Numero dei passeggeri (n)\n \
	Numero di posti disponibili in un'auto (C)\n \
	Numero di auto (m)\n \
	NOTA: Ricorda che n > C! e che per avere delle situazioni rilevanti occorre inserire n tale che n > (C*m).\n");
		perror(error);
      exit(1);
   }

	/* ***** CONTROLLO PARAMETRI ***** */

	/* Assegno alla variabile globale n il numero dei passeggeri */
	n = atoi(argv[1]);
	/* Assegno alla variabile globale C il numero dei posti auto */
	C = atoi(argv[2]);
	/* Assegno alla variabile globale m il numero delle auto */
	m = atoi(argv[3]);
		
	/* Controllo se queste variabili sono tutte > 0 */
	if ((n <= 0) || (C <= 0) || (m <= 0)) {
		perror("Errore: Inserire tutti numeri > 0!\n");
		exit(2);
	}

	/* Controllo che il numero di passeggeri (var. n) sia > del numero dei posti disponibili (C*m) */
	if (n <= (C*m)) {
		sprintf(error, "Errore: La variabile n non e' tale che n > C*m, infatti n=%d e C*m=%d!\n", n, (C*m));
		perror(error);
		exit(3);
	}

	/* ******************************* */

	/* Assegno alla variabile NUM_THREADS il numero TOTALE di threads da creare, ossia il numero di threads passeggeri e il numero di threads auto */
   NUM_THREADS = n + m;

   thread=(pthread_t *) malloc(NUM_THREADS * sizeof(pthread_t));
   if (thread == NULL)
   {
        perror("Problemi con l'allocazione dell'array thread\n");
        exit(4);
   }
   taskids = (int *) malloc(NUM_THREADS * sizeof(int));
   if (taskids == NULL)
   {
        perror("Problemi con l'allocazione dell'array taskids\n");
        exit(5);
    }

	/* Alloco la memoria per gli array dinamici di semafori */
	S_LOAD_QUEUE = (sem_t*) malloc(m*sizeof(sem_t));
	if (S_LOAD_QUEUE == NULL) {
		perror("Errore allocazione memoria per semaforo S_LOAD_QUEUE\n");
		exit(6);
	}
	S_UNLOAD_QUEUE = (sem_t*) malloc(m*sizeof(sem_t));
	if (S_UNLOAD_QUEUE == NULL) {
		perror("Errore allocazione memoria per semaforo S_UNLOAD_QUEUE\n");
		exit(7);
	}

	/* ***** Inizializzo i semafori e le variabilli condition ***** */

	if (sem_init(&S_CAR_LOAD, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_CAR_LOAD \n");
		exit(8);
	}
	if (sem_init(&S_CAR_UNLOAD, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_CAR_UNLOAD \n");
		exit(9);
	}
	if (sem_init(&S_PASSEGGERI_SALITI, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_PASSEGGERI_SALITI \n");
		exit(10);
	}
	if (sem_init(&S_PASSEGGERI_SCESI, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_PASSEGGERI_SCESI \n");
		exit(10);
	}

	for (i = 0;i < m;i++) {
		/* L'auto di indice 0 e' la prima auto che carichera' e fara' scendere i passeggeri, e dunque sara' l'unica auto ad avere 
		 *	i semafori con valore iniziale uguale ad 1, mentre le restanti auto avranno come valore iniziale del semaforo 
		 * associato = 0 per rimanere (correttamente) in attesa
		*/
		if (i == 0) {
			if (sem_init(&S_LOAD_QUEUE[i], 0, 1) != 0) {
				perror("Errore inizializzazione semaforo S_LOAD_QUEUE \n");
				exit(11);
			}
			if (sem_init(&S_UNLOAD_QUEUE[i], 0, 1) != 0) {
				perror("Errore inizializzazione semaforo S_UNLOAD_QUEUE \n");
				exit(12);
			}
		} else {
			if (sem_init(&S_LOAD_QUEUE[i], 0, 0) != 0) {
				perror("Errore inizializzazione semaforo S_LOAD_QUEUE \n");
				exit(13);
			}
			if (sem_init(&S_UNLOAD_QUEUE[i], 0, 0) != 0) {
				perror("Errore inizializzazione semaforo S_UNLOAD_QUEUE \n");
				exit(14);
			}
		}
	}

	/* ************************************************************ */

   for (i=0; i < NUM_THREADS; i++)
   {
		taskids[i] = i;
		/* Creo i thread dei PASSEGGERI */
		if (i < n) {
   		printf("Sto per creare il thread %d-esimo (passeggero)\n", taskids[i]);
     		if (pthread_create(&thread[i], NULL, eseguiPasseggero, (void *) (&taskids[i])) != 0)
      	{
      		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (passeggero)\n", taskids[i]);
      	   perror(error);
				exit(15);
      	}
			printf("SONO IL MAIN e ho creato il Pthread %i-esimo (passeggero) con id=%lu\n", i, thread[i]);
		} else {
			/* Creo i thread delle auto */
   		printf("Sto per creare il thread %d-esimo (auto)\n", taskids[i]);
			/* Al thread dell'auto occorre passare il suo numero di indice, ossia l'indice i attuale MENO il numero totale dei passeggeri (n) */
     		if (pthread_create(&thread[i], NULL, eseguiAuto, (void *) (&taskids[i] - n)) != 0)
      	{
      		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (auto)\n", taskids[i]);
      	   perror(error);
				exit(16);
      	}
			printf("SONO IL MAIN e ho creato il Pthread %i-esimo (auto) con id=%lu\n", i, thread[i]);
		}
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


