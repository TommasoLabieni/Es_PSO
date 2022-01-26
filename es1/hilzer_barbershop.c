#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include "include/arrayOperation.h"

/* Costante per definire il numero di barbieri (che definisce anche il numero di clienti che possono essere serviti concorrentemente */
#define NUM_BARBIERI 3
/* Costante per definire il numero massimo di clienti ammessi all'interno del negozio */
#define NUM_CLIENTI_MAX 20
/* Costante per definire il numero di posti nel divano */
#define NUM_POSTI_DIVANO 4
/* Costante per definire il numero di registri di cassa disponibili */
#define NUM_REGISTRI_CASSA 1

/* Variabile intera per memorizzare il numero di clienti seduti nel divano (utilizzata per motivi di debug)*/
int num_clienti_divano = 0;
/* Array dinamico per memorizzare la coda dei clienti nel divano */
int *clienti_divano;
/* Array dinamico per memorizzare la coda di persone in piedi. */
int *clienti_in_piedi;
/* Array dinamico per memorizzare la coda dei clienti serviti (utilizzato per motivi di debug)*/
int *clienti_serviti;
/* Mutex per l'accesso concorrente alle variabili condivise */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
/* Array di semafori per regolare i clienti che possono sedersi sul divano */
sem_t *S_DIVANO_OCCUPATO;
/* Array di semafori per regolare i clienti che vengono serviti */
sem_t *S_CLIENTI_SERVITI;
/* Semaforo per regolare i clienti entranti nel negozio */
sem_t S_CLIENTI_ENTRANTI;
/* Semaforo per regolare i clienti seduti nel divano */
sem_t S_DIVANO;
/* Semafori PRIVATI per sincronizzare i clienti e i barbieri per il taglio dei capelli */
sem_t S_CLIENTE_TO_BARBIERE_ENTRATO;
/* Array di semafori per attendere che un cliente si sia seduto nel divano prima di essere servito */
sem_t *S_CLIENTE_TO_BARBIERE_SERVITO;
/* Semaforo privato per segnalare al cliente che il taglio di capelli è terminato e che dunque deve pagare il conto */
sem_t S_BARBIERE_TO_CLIENTE;
/* Semaforo per bloccare il barbiere che prova ad utilizzare la cassa (il cui valore iniziale e' il numero dei registri di cassa disponibili) */
sem_t S_ACCEDI_CASSA;
/* Semaforo PRIVATO per bloccare il barbiere in attesa che il cliente sia pronto a pagare */
sem_t S_CLIENTE_PAGA;
/* Semaforo PRIVATO per bloccare il cliente in attesa che paghi il conto */
sem_t S_CONTO_SALDATO;

/* Funzione enterShop del cliente che entra nel negozio */
void enterShop(int indice) {
	/* NOTA: NON SERVE EFFETTUARE LA LOCK DEL MUTEX, IN QUANTO E' GIA' STATO PRESO PRIMA DELL'INVOCAZIONE DELLA SEGUENTE FUNZIONE */
  	printf("Il Thread CLIENTE di indice %d con identificatore %lu e' entrato nel negozio.\n", indice, pthread_self());
	/* Successivamente il cliente si mette nella coda delle persone in attesa */
	int i = getFirstFreePosition(clienti_in_piedi, NUM_CLIENTI_MAX);
	/* NOTA: Non si controlla se l'indice ritornato dalla funzione getFirstFreePosition sia uguale a -1, in quanto 
	 * l'array clienti_in_piedi e' di dimensione NUM_CLIENTI_MAX, e i clienti che entrano in questa funzione
	 * sono AL MASSIMO NUM_CLIENTI_MAX, in quanto l'entrata e' regolata dal semaforo S_CLIENTI_ENTRANTI che ha come
	 * valore iniziale proprio NUM_CLIENTI_MAX.
	*/
	clienti_in_piedi[i] = indice;
}

/* Funzione sitOnSofa del cliente che si siede sul divano */
void sitOnSofa(int indice) {
	/* NOTA: Anche qui non c'e' bisogno di prendere il mutex per lo stesso motivo della funzione enterShop. */
	int i; /* Variabile per memorizzare l'indice dell'array clienti_divano in cui inserire il cliente */
		
	/* Aumento il contatore delle persone sedute nel divano */
	num_clienti_divano++;
	/* A questo punto occorre inserirlo nella coda delle persone sedute sul divano */
	i = getFirstFreePosition(clienti_divano, NUM_POSTI_DIVANO);
	/* NOTA: Non si controlla se l'indice ritornato dalla funzione getFirstFreePosition sia uguale a -1, in quanto 
	 * l'array clienti_divano e' di dimensione NUM_POSTI_DIVANO, e i clienti che entrano in questa funzione
	 * sono AL MASSIMO NUM_POSTI_DIVANO, in quanto l'entrata e' regolata dal semaforo S_DIVANO che ha come
	 * valore iniziale proprio NUM_POSTI_DIVANO.
	*/
	clienti_divano[i] = indice;
	
	printf("Il Thread CLIENTE di indice %d con identificatore %lu si e' seduto nel divano. Il numero di clienti seduti e': %d\n", indice, pthread_self(), num_clienti_divano);
}

/* Funzione getHairCut del cliente per tagliare i capelli */
void getHairCut(int indice) {
	/* NOTA: Anche qui non serve bloccare il mutex perche' e' gia' stato fatto */
	/* Attendo che il barbiere svolga il proprio compito */
	sem_wait(&S_BARBIERE_TO_CLIENTE);
	printf("Taglio dei capelli per il Thread CLIENTE di indice %d con identificatore %lu terminato.\n", indice, pthread_self());

}

/* Funzione pay del cliente per pagare il conto */
void pay(int indice) {
	/* Si segnala al barbiere alla cassa che il cliente e' pronto a pagare */
	sem_post(&S_CLIENTE_PAGA);
	/* Si aspetta che il barbiere abbia accettato il pagamento */
	sem_wait(&S_CONTO_SALDATO);
	printf("Il Thread CLIENTE di indice %d con identificatore %lu ha pagato il conto.\n", indice, pthread_self());
}

void *eseguiCliente(void *id)
{   int *pi = (int *)id;
   	int *ptr;

   	ptr = (int *) malloc(sizeof(int));
   	if (ptr == NULL)
   	{
   		perror("Problemi con l'allocazione di ptr\n");
   		exit(-1);
   	}
   	*ptr = *pi;

   	printf("Thread CLIENTE di indice %d partito: Ho come identificatore %lu\n", *ptr, pthread_self());
	/* Prima che il cliente possa entrare nel negozio, occorre verificare il numero di clienti dentro il negozio. 
	 * Il tutto e' regolato dal semaforo S_CLIENTI_ENTRANTI 
	*/
	sem_wait(&S_CLIENTI_ENTRANTI);
	pthread_mutex_lock(&mutex);
 	/* Il cliente entra all'interno del negozio */
	enterShop(*ptr);
	pthread_mutex_unlock(&mutex);
	/* Il cliente sveglia il barbiere */
	sem_post(&S_CLIENTE_TO_BARBIERE_ENTRATO);
	/* Il cliente attende poi di sedersi nel divano */
	sem_wait(&S_DIVANO);
	/* Se qui allora almeno un posto nel divano e' libero. Il cliente si mette in attesa sul semaforo ad esso associato 
	 * per rispettare la politica FIFO richiesta per i clienti che passano da IN PIEDI al DIVANO
	*/
	sem_wait(&S_DIVANO_OCCUPATO[(*ptr)]);
	pthread_mutex_lock(&mutex);
	/* Se qui allora il cliente puo' effetivamente sedersi sul divano */
	sitOnSofa(*ptr);
	pthread_mutex_unlock(&mutex);
	/* Il cliente attende di essere servito -> segnala al barbiere che è in attesa */
	sem_post(&S_CLIENTE_TO_BARBIERE_SERVITO[(*ptr)]);
	/* Anche qui il cliente si mette in attesa sul semaforo associato per rispettare la politica FIFO richiesta per i clienti 
	 * che passano dal DIVANO ad ESSERE SERVITI 
	*/
	sem_wait(&S_CLIENTI_SERVITI[(*ptr)]);
	
	/* Se qui, il cliente viene servito -> Si procede con il taglio di capelli */
	getHairCut(*ptr);	
	/* Prima di uscire, il cliente paga il conto */
	pay(*ptr);
	printf("Thread CLIENTE di indice %d con identificatore %lu esce dal negozio.\n", *ptr, pthread_self());
	/* Sveglio infine l'eventuale prossimo cliente in attesa di entrare nel negozio */
	sem_post(&S_CLIENTI_ENTRANTI);
	pthread_exit((void *) ptr);
}

/* Funzione cutHair del barbiere per tagliare i capelli */
void cutHair(int indice, int indice_cliente) {
	printf("Il Thread BARBIERE di indice %d con identificatore %lu INIZIA il taglio di capelli per il cliente di indice: %d.\n", indice, pthread_self(), indice_cliente);
	/* Sleep per fingere il taglio di capelli. NOTA: Questa sleep andra' a "rallentare" anche il cliente in quanto quest'ultimo 
	 * sara' in attesa di essere risvegliato dal barbiere sul semaforo S_BARBIERE_TO_CLIENTE
	*/
	sleep(2);
	printf("Il Thread BARBIERE di indice %d con identificatore %lu HA TERMINATO il taglio di capelli per il cliente di indice: %d.\n", indice, pthread_self(), indice_cliente);
	sem_post(&S_BARBIERE_TO_CLIENTE);
}

/* Funzione acceptPayment del barbiere per accettare il pagamento e consentire al cliente di uscire */
void acceptPayment(int indice) {
	/* Segnalo al cliente che il conto e' stato saldato */
	sem_post(&S_CONTO_SALDATO);
}

void *eseguiBarbiere(void *id)
{
   int *pi = (int *)id;
   int *ptr;
   int next_cliente_divano = -1;		/* Variabile per memorizzare l'indice del prossimo cliente che va a sedersi sul divano */
   int next_cliente_da_servire = -1;	/* Variabile per memorizzare l'indice del prossimo cliente da servire */
   int i, j;

   ptr = (int *) malloc(sizeof(int));
   if (ptr == NULL)
   {
   	perror("Problemi con l'allocazione di ptr\n");
      exit(-1);
   }
   *ptr = *pi;

   printf("Thread BARBIERE di indice %d partito: Ho come identificatore %lu\n", *ptr, pthread_self());
	while(true) {
		/* Il barbiere si mette in attesa che un cliente lo svegli */
		sem_wait(&S_CLIENTE_TO_BARBIERE_ENTRATO);
		pthread_mutex_lock(&mutex);
		/* Ora sveglio il cliente che e' in piedi da piu' tempo */
		printArray(clienti_in_piedi, NUM_CLIENTI_MAX, "IN PIEDI");
		/* Recupero la prima posizione libera in quanto i clienti vengono inseriti in ordine FIFO per rispettare il vincolo di servire
		 * il cliente che e' in attesa da piu' tempo
		*/
		i = getFirstNonFreePosition(clienti_in_piedi, NUM_CLIENTI_MAX);
		/* NOTA: Non si controlla se l'indice ritornato dalla funzione getFirstNonFreePosition sia uguale a -1, in quanto 
		* sicuramente, A QUESTO PUNTO DEL PROGRAMMA, un cliente ha invocato la funzione enterShop e dunque si e' inserito nell'array
		* clienti_in_piedi
		*/
		next_cliente_divano = clienti_in_piedi[i];
		/* Una volta trovato il cliente, shifto all'indietro l'array che memorizza i clienti in piedi */
		for (j=i; j < (NUM_CLIENTI_MAX-1); j++) {
			clienti_in_piedi[j] = clienti_in_piedi[j+1];
		}
		/* Una volta shiftato l'array, libero l'ultimo posto */
		clienti_in_piedi[j] = -1;
	
		pthread_mutex_unlock(&mutex);
		printf("Thread BARBIERE di indice %d con identificatore %lu vuole risvegliare il thread cliente di indice %d CHE E' IN PIEDI.\n", *ptr, pthread_self(), next_cliente_divano);
		/* Risveglio il cliente che potra' ora mettersi a sedere sul divano */
		sem_post(&S_DIVANO_OCCUPATO[next_cliente_divano]);
		printf("Thread BARBIERE di indice %d con identificatore %lu ha risvegliato il thread cliente di indice %d CHE ERA IN PIEDI.\n", *ptr, pthread_self(), next_cliente_divano);
		/* Aspetto che il cliente che si e' seduto nel divano sia pronto al servizio. Senza questo passaggio c'è il rischio
		 * di non avere ancora alcun cliente nel divano e dunque, dal momento che l'azione successiva è quella di servire un cliente seduto nel divano,
		 * si rischierebbe di non servire il cliente!
		*/
		sem_wait(&S_CLIENTE_TO_BARBIERE_SERVITO[next_cliente_divano]);
		pthread_mutex_lock(&mutex);
		/* Ora sveglio il cliente che e' seduto da piu' tempo */
		printArray(clienti_divano, NUM_POSTI_DIVANO, "DIVANO");
		/* Recupero la prima posizione libera in quanto i clienti vengono inseriti in ordine FIFO per rispettare il vincolo di servire
		 * il cliente che e' in attesa da piu' tempo
		*/
		i = getFirstNonFreePosition(clienti_divano, NUM_POSTI_DIVANO);
		/* NOTA: Non si controlla se l'indice ritornato dalla funzione getFirstNonFreePosition sia uguale a -1, in quanto 
		* sicuramente, A QUESTO PUNTO DEL PROGRAMMA, un cliente ha invocato la funzione sitOnSofa e dunque si e' inserito nell'array
		* clienti_divano
		*/
		next_cliente_da_servire = clienti_divano[i];
		/* Una volta trovato, shifto l'array che memorizza i clienti seduti nel divano */
		for (j=i; j < (NUM_POSTI_DIVANO-1); j++) {
			clienti_divano[j] = clienti_divano[j+1];
		}
		/* Una volta shiftato l'array, libero l'ultimo posto */
		clienti_divano[j] = -1;
		/* Il cliente viene servito -> decremento il contatore di persone sul divano */
		num_clienti_divano--;
		pthread_mutex_unlock(&mutex);

		printf("Thread BARBIERE di indice %d partito con identificatore %lu vuole risvegliare il thread di indice %d CHE E' NEL DIVANO.\n", *ptr, pthread_self(), next_cliente_da_servire);
		sem_post(&S_CLIENTI_SERVITI[next_cliente_da_servire]);
		/* Si e' liberato un posto nel divano -> aumento il valore del semaforo che regola il numero di clienti che possono sedersi nel divano */
		sem_post(&S_DIVANO);
		printf("Thread BARBIERE di indice %d partito con identificatore %lu ha risvegliato il thread di indice %d CHE ERA NEL DIVANO.\n", *ptr, pthread_self(), next_cliente_da_servire);
		
		/* Ora il barbiere procede al taglio dei capelli */
		cutHair(*ptr, next_cliente_da_servire);
		/* Terminato il taglio, attendo che il cliente sia pronto a pagare */
		sem_wait(&S_CLIENTE_PAGA);
		/* Se qui, allora il cliente ha pagato e ha SICURAMENTE invocato la funzione pay */
		/* A questo punto il barbiere deve presentarsi alla cassa e far pagare il cliente. Dal momento che vi e' soltanto una cassa
		 *bisogna verificare che questa non sia gia' utilizzata da altri barbieri ù
		*/
		sem_wait(&S_ACCEDI_CASSA);
		/* Una volta che il barbiere e' al registro di cassa accetta il pagamento */
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
	
	thread=(pthread_t *) malloc((NUM_THREADS + NUM_BARBIERI) * sizeof(pthread_t));
	if (thread == NULL)
	{
		perror("Problemi con l'allocazione dell'array thread\n");
		exit(3);
	}
	taskids = (int *) malloc((NUM_THREADS + NUM_BARBIERI) * sizeof(int));
	if (taskids == NULL)
	{
			perror("Problemi con l'allocazione dell'array taskids\n");
			exit(4);
	}
	
	/* Alloco memoria per gli array di semafori */
	S_CLIENTI_SERVITI = (sem_t*) malloc((NUM_THREADS) * sizeof(sem_t));
	S_DIVANO_OCCUPATO = (sem_t*) malloc((NUM_THREADS) * sizeof(sem_t));
	S_CLIENTE_TO_BARBIERE_SERVITO = (sem_t*) malloc((NUM_THREADS) * sizeof(sem_t));
	if (S_CLIENTI_SERVITI == NULL) {
		perror("Problemi con l'allocazione dell'array S_CLIENTI_SERVITI");
		exit(5);
	}
	if (S_DIVANO_OCCUPATO == NULL) {
		perror("Problemi con l'allocazione dell'array S_DIVANO_OCCUPATO");
		exit(6);
	}
	if (S_CLIENTE_TO_BARBIERE_SERVITO == NULL) {
		perror("Problemi con l'allocazione dell'array S_CLIENTE_TO_BARBIERE_SERVITO");
		exit(7);
	}
	
	/* ***** INIZIALIZZAZIONE DEI SEMAFORI ***** */

	for (i=0; i < NUM_THREADS; i++) {
		if (sem_init(&S_CLIENTI_SERVITI[i], 0, 0) != 0) {
			sprintf(error, "Errore inizializzazione semaforo S_CLIENTI_SERVITI %d-esimo", i);
			perror(error);
			exit(8);
		}
		if (sem_init(&S_DIVANO_OCCUPATO[i], 0, 0) != 0) {
			sprintf(error, "Errore inizializzazione semaforo S_DIVANO_OCCUPATO %d-esimo", i);
			perror(error);
			exit(9);
		}
		if (sem_init(&S_CLIENTE_TO_BARBIERE_SERVITO[i], 0, 0) != 0) {
			sprintf(error, "Errore inizializzazione semaforo S_CLIENTE_TO_BARBIERE_SERVITO %d-esimo", i);
			perror(error);
			exit(10);
		}
	}

	if (sem_init(&S_CLIENTE_TO_BARBIERE_ENTRATO, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_CLIENTE_TO_BARBIERE_ENTRATO\n");
		exit(11);
	}

	if (sem_init(&S_BARBIERE_TO_CLIENTE, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_BARBIERE_TO_CLIENTE\n");
		exit(12);
	}

	if (sem_init(&S_DIVANO, 0, NUM_POSTI_DIVANO) != 0) {
		perror("Errore inizializzazione semaforo S_CLIENTE_TO_BARBIERE_ENTRATO\n");
		exit(13);
	}

	if (sem_init(&S_ACCEDI_CASSA, 0, NUM_REGISTRI_CASSA) != 0) {
		perror("Errore inizializzazione semaforo S_ACCEDI_CASSA\n");
		exit(14);
	}

	if (sem_init(&S_CONTO_SALDATO, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_CONTO_SALDATO\n");
		exit(15);
	}

	if (sem_init(&S_CLIENTE_PAGA, 0, 0) != 0) {
		perror("Errore inizializzazione semaforo S_CLIENTE_PAGA\n");
		exit(16);
	}


	if (sem_init(&S_CLIENTI_ENTRANTI, 0, NUM_CLIENTI_MAX) != 0) {
		perror("Errore inizializzazione semaforo S_CLIENTI_ENTRANTI\n");
		exit(17);
	}
	
	/* ***************************************** */

	/* ***** INIZIALIZZAZIONE DEGLI ARRAY CONDIVISI ***** */

	/* Alloco la memoria per gli array */
	clienti_in_piedi = (int *) malloc(NUM_CLIENTI_MAX * sizeof(int));
	if (clienti_in_piedi == NULL) {
		perror("Problemi allocazione memoria array clienti_in_piedi\n");
		exit(18);
	}
	clienti_divano = (int *) malloc(NUM_POSTI_DIVANO * sizeof(int));
	if (clienti_divano == NULL) {
		perror("Problemi allocazione memoria array clienti_divano\n");
		exit(19);
	}
	clienti_serviti = (int *) malloc(NUM_BARBIERI * sizeof(int));
	if (clienti_serviti == NULL) {
		perror("Problemi allocazione memoria array clienti_serviti\n");
		exit(20);
	}

	/* Inizializzo l'array dei clienti in piedi */
	for (i=0;i < NUM_CLIENTI_MAX; i++) {
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

   	for (i = 0; i < NUM_THREADS; i++)
	{
		taskids[i] = i;
     	if (pthread_create(&thread[i], NULL, eseguiCliente, (void *) (&taskids[i])) != 0)
      	{
      		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (cliente)\n", taskids[i]);
        	perror(error);
			exit(21);
      	}
		printf("SONO IL MAIN e ho creato il Pthread %i-esimo con id=%lu (cliente)\n", i, thread[i]);
	}

   	/* Creo i thread dei barbieri */
	for (;i < NUM_THREADS + NUM_BARBIERI; i++) {
		taskids[i] = i;
     	if (pthread_create(&thread[i], NULL, eseguiBarbiere, (void *) (&taskids[i])) != 0)
		{
    		sprintf(error,"SONO IL MAIN E CI SONO STATI PROBLEMI DELLA CREAZIONE DEL thread %d-esimo (BARBIERE)\n", taskids[i]);
        	perror(error);
			exit(22);
      	}
		printf("SONO IL MAIN e ho creato il Pthread %i-esimo con id=%lu (barbiere)\n", i, thread[i]);
	}	

	/* Aspetto che terminino TUTTI E SOLI i thread dei clienti */
   for (i=0; i < NUM_THREADS; i++)
   {
		int ris;
		/* attendiamo la terminazione di tutti i thread generati */
   		pthread_join(thread[i], (void **) &p);
		ris= *p;
		printf("Pthread %d-esimo restituisce %d\n", i, ris);
   }

   exit(0);
}