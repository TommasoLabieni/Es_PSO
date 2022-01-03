//TODO: Controllare se un thread è già stato risvegliato per la sua var condition (sia per divano sia per serviti)
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

/* Costante per definire il numero di barbieri (che definisce anche il numero di clienti servibili concorrentemente */
#define NUM_BARBIERI 3
/* Costante per definire il numero massimo di clienti ammessi all'interno del negozio */
#define NUM_CLIENTI_MAX 20
/* Costante per definire il numero di posti nel divano */
#define NUM_POSTI_DIVANO 4
/* Costante per definire il numero di registri di cassa disponibili */
#define NUM_REGISTRI_CASSA 1

/* Variabile intera per memorizzare il numero di clienti all'interno del negozio */
int num_clienti_act = 0;
/* Variabile intera per memorizzare il numero di clienti seduti nel divano */
int num_clienti_divano = 0;
/* Variabile intera per memorizzare il numero di clienti attualmente serviti */
int num_clienti_serviti_act = 0;
/* Array statico per memorizzare la coda dei clienti nel divano */
int clienti_divano[NUM_POSTI_DIVANO];
/* Array statico per memorizzare la coda di persone in piedi. NOTA: AL MASSIMO IN PIEDI CI POSSONO ESSERE NUM_CLIENTI_MAX - NUM_BARBIERI (che indica anche il numero di clienti che possono essere serviti concorrentemente) - NUM_POSTI_DIVANO, che nelle condizioni specificate dal testo sarebbero: 20 - 3 - 4 = 13.
*/
int clienti_in_piedi[NUM_CLIENTI_MAX - NUM_BARBIERI - NUM_POSTI_DIVANO];
/* Array statico per memorizzare la coda dei clienti serviti */
int clienti_serviti[NUM_BARBIERI];
/* Variabile booleana per indicare che il cliente e' pronto a pagare */
bool cliente_pronto_a_pagare = false;
/* Mutex per l'accesso concorrente alle variabili condivise */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/* Mutex utilizzato esclusivamente nella parte di pagamento */
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
/* Mutex utilizzato per controllare la capienza */
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
/* Variabile condition per bloccare i barbieri in attesa che i clienti siano pronti a pagare */
pthread_cond_t C_CLIENTE_PAGA = PTHREAD_COND_INITIALIZER;
/* Array di variabili condition per regolare i clienti che possono sedersi sul divano */
pthread_cond_t *C_DIVANO_OCCUPATO;
/* Array di variabili condition per regolare i clienti che vengono serviti */
pthread_cond_t *C_CLIENTI_SERVITI;
/* Semaforo per regolare i clienti entranti nel negozio */
sem_t S_CLIENTI_ENTRANTI;
/* Semafori PRIVATI per sincronizzare i clienti e i barbieri per il taglio dei capelli */
sem_t S_CLIENTE_TO_BARBIERE;
sem_t S_BARBIERE_TO_CLIENTE;
/* Semaforo per bloccare il barbiere che prova ad utilizzare la cassa (il cui valore iniziale e' il numero dei registri di cassa disponibili) */
sem_t S_ACCEDI_CASSA;
/* Semaforo PRIVATO per bloccare il cliente in attesa che paghi il conto */
sem_t S_CLIENTE_PAGA;
sem_t S_CONTO_SALDATO;

void printArray(int* v, int n, char* str) {
	printf("Stampo l'array: %s\n", str);
	for (int i=0;i<n;i++) {
		printf("%d:\t %d\n", i, v[i]);
	}
}

int getFirstFreePosition(int* v, int n) {
	for (int i=0;i < n; i++) {
		if (v[i] == -1) {
			return i;
		}
	}
	return -1;
}

int getFirstNonFreePosition(int* v, int n) {
	for (int i=0;i < n; i++) {
		if (v[i] != -1) {
			return i;
		}
	}
	return -1;
}

int getIndex(int* v, int n, int val) {
	for (int i=0;i < n; i++) {
		if (v[i] == val) {
			return i;
		}
	}
	return -1;
}

/* Funzione enterShop del cliente che entra nel negozio */
void enterShop(int indice) {
	/* NOTA: NON SERVE EFFETTUARE LA LOCK DEL MUTEX, IN QUANTO E' GIA' STATO PRESO PRIMA DELL'INVOCAZIONE DELLA SEGUENTE FUNZIONE */
	/* Variabile intera che memorizza il numero di massimo di persone che ci possono essere in coda */
	int max_num_clienti_coda = NUM_CLIENTI_MAX - NUM_BARBIERI - NUM_POSTI_DIVANO;
	/* Se qui allora il cliente e' entrato nel negozio -> incremento il numero di clienti all'interno del negozio */
	num_clienti_act++;
   printf("Il Thread CLIENTE di indice %d con identificatore %lu e' entrato nel negozio.\nIl numero di clienti all'interno e': %d\n", indice, pthread_self(), num_clienti_act);
	/* Successivamente il cliente si mette nella coda delle persone in attesa */
	for(int i=0; i < max_num_clienti_coda; i++) {
		if (clienti_in_piedi[i] == -1) {
			/* Una volta trovato l'indice in cui inserire il cliente, ne inserisco l'indice del thread */
			clienti_in_piedi[i] = indice;
			printArray(clienti_in_piedi, max_num_clienti_coda, "IN PIEDI");
			/* E termino l'esecuzione */
			return;
		}
	}
}

/* Funzione sitOnSofa del cliente che si siede sul divano */
void sitOnSofa(int indice) {
	/* NOTA: Anche qui non c'e' bisogno di prendere il mutex per lo stesso motivo della funzione enterShop. */
	/* Variabile intera che memorizza il numero di massimo di persone che ci possono essere in coda */
	int j; /* Variabile per scorrere l'array */
	int max_num_clienti_coda = NUM_CLIENTI_MAX - NUM_BARBIERI - NUM_POSTI_DIVANO;
	

	/* A questo punto occorre inserirlo nella coda delle persone sedute sul divano */
	for (int i=0; i < NUM_POSTI_DIVANO; i++) {
		if (clienti_divano[i] == -1) {
			/* Se il posto i-esimo del divano e' libero, allora il cliente si siede (ossia inserisce l'indice del thread cliente nel posto i-esimo). */
			clienti_divano[i] = indice;
			/* Aumento il contatore dei clienti sul divano */
			num_clienti_divano++;
			printf("Il Thread CLIENTE di indice %d con identificatore %lu si e' seduto nel divano. Il numero di clienti seduti e': %d\n", indice, pthread_self(), num_clienti_divano);
			/* E termino il ciclo */
			break;
		}
	}

	printArray(clienti_divano, NUM_POSTI_DIVANO, "DIVANO");
	/* A questo punto bisogna liberare il posto in piedi occupato precedentemente dal cliente */

	for (int i=0 ;i < max_num_clienti_coda; i++) {
		/* Prima cerco la posizione del cliente */
		if (clienti_in_piedi[i] == indice) {
			/* Una volta trovato il cliente libero il suo posto e shifto all'indietro l'array che memorizza i clienti in piedi */
			for (j=i; j < (max_num_clienti_coda - 1); j++) {
				clienti_in_piedi[j] = clienti_in_piedi[j+1];
			}
			/* Una volta shiftato l'array, libero l'ultimo posto */
			clienti_in_piedi[j] = -1;
			printArray(clienti_in_piedi, max_num_clienti_coda, "IN PIEDI");
			/* E termino l'esecuzione */
			return;
		}
	}
}

/* Funzione liberaPostDivano del cliente per lberare un posto nel divano */
void liberaPostoDivano(int indice) {
	/* NOTA: Anche qui non serve bloccare il mutex perche' e' gia' stato fatto */
	int j; /* Variabile per scorrere l'array */
	int index = -1; /* Variabile per recuperare l'indice del prossimo thread IN PIEDI da risvegliare */
	bool cliente_in_piedi_trovato = false; /* Variabile booleana per evitare di svegliare un thread IN PIEDI che non sia quello da piu' tempo in attesa */
	/* Come prima cosa occorre ricercare l'indice in cui e' stato inserito il cliente */
	for (int i=0;i < NUM_POSTI_DIVANO; i++) {
		if ((clienti_divano[i] != indice) && (!cliente_in_piedi_trovato) && (clienti_divano[i] != -1)) {
			cliente_in_piedi_trovato = true;
			index = clienti_divano[i];
		}
		if (clienti_divano[i] == indice) {
			if (!cliente_in_piedi_trovato) {
				/* Se ancora non ho trovato il cliente in piedi controllo se il prossimo cliente (rispetto a quello da togliere) esiste */
				if (clienti_divano[i+1] != -1) {
					index = clienti_divano[i+1];
				}
			}
			/* Una volta trovato, shifto l'array che memorizza le persone sedute nel divano */
			for (j=i; j < (NUM_POSTI_DIVANO-1); j++) {
				if (clienti_divano[j] == -1)
					break;
				clienti_divano[j] = clienti_divano[j+1];
			}
			/* Una volta shiftato l'array, libero l'ultimo posto */
			clienti_divano[j] = -1;
			/* E decremento il contatore di persone sul divano */
			num_clienti_divano--;
			num_clienti_serviti_act++;
			/* Inserisco il cliente nella coda dei clienti serviti */
			i = getFirstFreePosition(clienti_serviti, NUM_BARBIERI);
			clienti_serviti[i] = indice;
			/* Segnalo all'eventuale thread in attesa sulla var condition C_DIVANO_OCCUPATO che si e' liberato un posto sul divano. */
   			printf("Il Thread CLIENTE di indice %d con identificatore %lu si e' seduto nella sedia e attende il taglio dei capelli.\nATTUALMENTE CI SONO %d CLIENTI SERVITI\n", indice, pthread_self(), num_clienti_serviti_act);
			char str[255];
			sprintf(str, "SERVITI %d", indice);
			printArray(clienti_serviti, NUM_BARBIERI, str);
			if (index != -1) {
				printf("RISVEGLIO DIVANO: %d\n", index - NUM_BARBIERI);
				pthread_cond_signal(&C_DIVANO_OCCUPATO[index - NUM_BARBIERI]);
			}
			/* E termino l'esecuzione */
			return;
		}
	}
}

/* Funzione getHairCut del cliente per tagliare i capelli */
void getHairCut(int indice) {
	/* NOTA: Anche qui non serve bloccare il mutex perche' e' gia' stato fatto */
	/* Il cliente invia un segnale al barbiere */
	sem_post(&S_CLIENTE_TO_BARBIERE);
	sem_wait(&S_BARBIERE_TO_CLIENTE);
	printf("Il Thread CLIENTE di indice %d con identificatore %lu ha terminato il taglio dei capelli.\n", indice, pthread_self());

}

/* Funzione pay del cliente per pagare il conto */
void pay(int indice) {
	/* NOTA: Anche qui non serve bloccare il mutex perche' e' gia' stato fatto */
	/* Si segnala al barbiere alla cassa che il cliente e' pronto a pagare */
	sem_post(&S_CLIENTE_PAGA);
	/* Si aspetta che il barbiere abbia accettato il pagamento */
	sem_wait(&S_CONTO_SALDATO);
	printf("Il Thread CLIENTE di indice %d con identificatore %lu ha pagato il conto.\n", indice, pthread_self());

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
	sem_wait(&S_CLIENTI_ENTRANTI);
	pthread_mutex_lock(&mutex);
	/* E poi il cliente entra all'interno del negozio */
	enterShop(*ptr);
	/* Una volta entrato, prova a sedersi nel divano (se libero) */
	while (num_clienti_divano >= NUM_POSTI_DIVANO) {
		printf("Thread CLIENTE di indice %d con identificatore %lu non puo' andare nel divano\n", *pi, pthread_self());
		/* Se il divano e' pieno, allora il cliente deve aspettare */
		pthread_cond_wait(&C_DIVANO_OCCUPATO[(*ptr)-NUM_BARBIERI], &mutex);
	}
	/* Se qui allora il cliente puo' sedersi sul divano */
	sitOnSofa(*ptr);
	/* Una volta seduto nel divano, il cliente si mette in attesa di essere servito */
	while ((num_clienti_serviti_act >= NUM_BARBIERI) ) {
		printf("Thread CLIENTE di indice %d con identificatore %lu non puo' essere servito\n", *pi, pthread_self());
		/* Il cliente aspetta SULLA PROPRIA VAR CONDITION! Dato che, nel numero dei threads, sono contati anche i barbieri, occorre sottrarli */
		pthread_cond_wait(&C_CLIENTI_SERVITI[(*ptr)-NUM_BARBIERI], &mutex);
	}
	/* Prima si libera il posto nel divano */
	liberaPostoDivano(*ptr);
	pthread_mutex_unlock(&mutex);
	/* Si procede con il taglio di capelli */
	getHairCut(*ptr);	
	/* Prima di uscire, il cliente paga il conto */
	pay(*ptr);
	pthread_mutex_lock(&mutex);
	/* A questo punto il cliente esce dal negozio -> si decrementa il contatore dei clienti nel negozio e dei clienti serviti */
	num_clienti_act--;
	num_clienti_serviti_act--;
	for (int i=0;i < NUM_BARBIERI; i++) {
		if (clienti_serviti[i] == *ptr) {
			clienti_serviti[i] = -1;
			break;
		}
	}

	int next_cliente_servito = -1;		/* Variabile che memorizza l'indice del prossimo (eventuale) cliente che andra' ad essere servito */
	/* Recupero l'indice dell'eventuale cliente in attesa da più tempo */
	next_cliente_servito = getFirstNonFreePosition(clienti_divano, NUM_POSTI_DIVANO);
	if (next_cliente_servito != -1)
	{
		/* MOLTO IMPORTANTE: IL CONTATORE DEI CLIENTI SERVITI VERRA' INCREMENTATO DOPO DAL CLIENTE RISVEGLIATO, NELLA FUNZIONE liberaPostoDivano!!!!!!! */
		/* Si sveglia il prossimo cliente in attesa di essere servito (si sottrae NUM_BARBIERI all'indice per il motivo specificato in fase di wait)*/
		printf("RISVEGLIO SERVITI: %d\n", clienti_divano[next_cliente_servito] - NUM_BARBIERI);
		pthread_cond_signal(&C_CLIENTI_SERVITI[clienti_divano[next_cliente_servito] - NUM_BARBIERI]);
   	}
	pthread_mutex_unlock(&mutex);
	/* Sveglio infine l'eventuale prossimo cliente in attesa di entrare nel negozio */
	sem_post(&S_CLIENTI_ENTRANTI);
	pthread_exit((void *) ptr);
}

/* Funzione cutHair del barbiere per tagliare i capelli */
void cutHair(int indice) {
   printf("Il Thread BARBIERE di indice %d con identificatore %lu INIZIA il taglio di capelli.\n", indice, pthread_self());
	/* Sleep per fingere il taglio di capelli. NOTA: Questa sleep andra' a "rallentare" anche il cliente in quanto quest'ultimo sara' in attesa di essere risvegliato dal barbiere */
	sleep(4);
   printf("Il Thread BARBIERE di indice %d con identificatore %lu HA TERMINATO il taglio di capelli.\n", indice, pthread_self());
	sem_post(&S_BARBIERE_TO_CLIENTE);
}

/* Funzione acceptPayment del barbiere per accettare il pagamento e consentire al cliente di uscire */
void acceptPayment(int indice) {
	/* NOTA: Anche qui non serve bloccare il mutex perche' e' gia' stato fatto */
//   printf("Il Thread BARBIERE di indice %d con identificatore %lu ha riscosso il pagamento.\n", indice, pthread_self());
	sem_post(&S_CONTO_SALDATO);
}

void *eseguiBarbiere(void *id)
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

   printf("Thread BARBIERE di indice %d partito: Ho come identificatore %lu\n", *pi, pthread_self());
	while(true) {
		/* Il barbiere si mette in attesa che un cliente lo svegli */
		sem_wait(&S_CLIENTE_TO_BARBIERE);
		/* Una volta sveglio, il barbiere procede al taglio di capelli */
		cutHair(*ptr);
		/* Terminato il taglio, attendo che il cliente sia pronto a pagare */
		sem_wait(&S_CLIENTE_PAGA);
		/* Se qui, allora il cliente ha pagato e ha SICURAMENTE invocato la funzione pay */
		/* A questo punto il barbiere deve presentarsi alla cassa e far pagare il cliente. Dal momento che vi e' soltanto una cassa, bisogna verificare che questa non sia gia' utilizzata da altri barbieri */
		sem_wait(&S_ACCEDI_CASSA);
		acceptPayment(*ptr);
		/* Si libera il registro di cassa */
		sem_post(&S_ACCEDI_CASSA);
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
   		printf(error,"Errore: Il primo parametro non e' un numero strettamente maggiore di 0 ma e' %d\n", NUM_THREADS);
		perror(error);
   	   	exit(2);
   	}
	/* A NUM_THREADS ci sommo anche il numero di barbieri */
	NUM_THREADS += NUM_BARBIERI;
	
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
	
	/* Alloco memoria per gli array di variabili condition */
	C_CLIENTI_SERVITI = (pthread_cond_t*) malloc((NUM_THREADS - NUM_BARBIERI) * sizeof(pthread_cond_t));
	C_DIVANO_OCCUPATO = (pthread_cond_t*) malloc((NUM_THREADS - NUM_BARBIERI) * sizeof(pthread_cond_t));
	if (C_CLIENTI_SERVITI == NULL) {
		perror("Problemi con l'allocazione dell'array C_CLIENTI_SERVITI");
		exit(5);
	}
	if (C_DIVANO_OCCUPATO == NULL) {
		perror("Problemi con l'allocazione dell'array C_DIVANO_OCCUPATO");
		exit(6);
	}

	/* ***** INIZIALIZZAZIONE DEI SEMAFORI E VAR CONDITION ***** */
	for (i=0; i < NUM_THREADS - NUM_BARBIERI; i++) {
		if (pthread_cond_init(&C_CLIENTI_SERVITI[i], NULL) != 0) {
			sprintf(error, "Errore inizializzazione variabile condition C_CLIENTI_SERVITI %d-esima", i);
			perror(error);
			exit(6);
		}
		if (pthread_cond_init(&C_DIVANO_OCCUPATO[i], NULL) != 0) {
			sprintf(error, "Errore inizializzazione variabile condition C_DIVANO_OCCUPATO %d-esima", i);
			perror(error);
			exit(6);
		}
	}

	if (sem_init(&S_CLIENTE_TO_BARBIERE, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_CLIENTE_TO_BARBIERE\n");
		exit(6);
	}

	if (sem_init(&S_BARBIERE_TO_CLIENTE, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_BARBIERE_TO_CLIENTE\n");
		exit(7);
	}


	if (sem_init(&S_ACCEDI_CASSA, 0, NUM_REGISTRI_CASSA) != 0) {
		perror("Errore inizializzazione semaforo S_ACCEDI_CASSA\n");
		exit(8);
	}

	if (sem_init(&S_CONTO_SALDATO, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_CONTO_SALDATO\n");
		exit(9);
	}

	if (sem_init(&S_CLIENTE_PAGA, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_CLIENTE_PAGA\n");
		exit(9);
	}


	if (sem_init(&S_CLIENTI_ENTRANTI, 0, NUM_CLIENTI_MAX) != 0) {
		perror("Errore inizializzazione semaforo S_CLIENTI_ENTRANTI\n");
		exit(9);
	}

	/* ********************************************************* */

	/* ***** INIZIALIZZAZIONE DEGLI ARRAY CONDIVISI ***** */

	/* Inizializzo l'array dei clienti in piedi */
	for (i=0;i < (NUM_CLIENTI_MAX - NUM_BARBIERI - NUM_POSTI_DIVANO); i++) {
		clienti_in_piedi[i] = -1;
	}

	/* Inizializzo l'array dei clienti nel divano */
	for (i=0;i < NUM_POSTI_DIVANO; i++) {
		clienti_divano[i] = -1;
	}

	/* Inizializzo l'array dei clienti serviti */
	for (i=0;i < NUM_BARBIERI; i++) {
		clienti_serviti[i] = -1;
	}

	/* ************************************************** */

	/* Creo i thread dei barbieri */
	for (i=0; i < NUM_BARBIERI; i++) {
		taskids[i] = i;
//   	printf("Sto per creare il thread %d-esimo (barbiere)\n", taskids[i]);
     	if (pthread_create(&thread[i], NULL, eseguiBarbiere, (void *) (&taskids[i])) != 0)
      {
      	sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (BARBIERE)\n", taskids[i]);
         perror(error);
			exit(10);
      }
		printf("SONO IL MAIN e ho creato il Pthread %i-esimo con id=%lu (barbiere)\n", i, thread[i]);
	}	

   for (; i < NUM_THREADS; i++)
   {
		taskids[i] = i;
//   	printf("Sto per creare il thread %d-esimo (cliente)\n", taskids[i]);
     	if (pthread_create(&thread[i], NULL, eseguiCliente, (void *) (&taskids[i])) != 0)
      {
      	sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (cliente)\n", taskids[i]);
         perror(error);
			exit(11);
      }
		printf("SONO IL MAIN e ho creato il Pthread %i-esimo con id=%lu (cliente)\n", i, thread[i]);
   }

	/* Aspetto che terminino TUTTI E SOLI i thread dei clienti */
   for (i=NUM_BARBIERI; i < NUM_THREADS; i++)
   {
		int ris;
		/* attendiamo la terminazione di tutti i thread generati */
   	pthread_join(thread[i], (void**) & p);
		ris= *p;
		printf("Pthread %d-esimo restituisce %d\n", i, ris);
		pthread_cond_destroy(&C_CLIENTI_SERVITI[i]);
   }
 
   exit(0);
}


