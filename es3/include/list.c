#include "list.h"

void esamina_lista(Lista L, int id) {
	while(L != NULL) {
		printf("Thread Searcher con indice %d ha letto l'elemento %d\n", id, L->element);
		L = L->next;
	}
}

int aggiungi_elemento(Lista* L, int el, int id) {
	char error[255];	/* Variabile per memorizzare l'eventuale messaggio di errore */
	Lista tmp = *L;	/* Variabile per scorrere la lista */
	Nodo* nd = (Nodo*) malloc(sizeof(Nodo));
	nd->next = NULL;
	nd->element = el;
	if (nd == NULL) {
		sprintf(error, "Errore allocazione memoria per il nodo creato dal Thread di indice %d\n", id);
		perror(error);
		return -1;
	}
	/* Se la lista e' vuota, allora il nodo appena creato e' la nuova lista */
	if (tmp == NULL) {
		*L = nd;
	} else {
		/* Altrimenti aggiungo il nodo appena creato in coda alla lista */
		while(tmp->next != NULL) {
			tmp = tmp->next;
		}
		tmp->next = nd;
	}
	printf("Thread Inserter con indice %d ha aggiunto l'elemento %d\n", id, el);
	return 0;
}

void elimina_elemento(Lista* L, int el, int id) {
	Lista prev = NULL;	/* Variabile per memorizzare l'elemento precedente della lista */
	Lista temp = *L;		/* Variabile per scorrere la lista */	
	int i = 0;				/* Variabile di indice */

	/* Controllo se la lista e' vuota */
	if (temp == NULL)
		/* In tal caso non faccio nulla */
		return;

   /* Controllo se devo eliminare la testa */
   if (el == 0) {
       *L = temp->next; /* Sposto di un elemento la testa della lista */
       free(temp); /* Libero la memoria allocata per l'elemento da eliminare */
       return;
    }
 
    /* Se non devo eliminare la testa allora mi sposto fino all'elemento precedente di quello da eliminare */
    for (int i = 0; temp != NULL && i < (el - 1); i++)
        temp = temp->next;
 
    /* A questo punto il nodo temp->next e' l'elemento da eliminare */
    /* Creo una variabile che memorizza l'elemento SUCCESSIVO a quello da eliminare (per evitare di perdere
	  * tutti gli elementi successivi a quello da eliminare).
	 */
    Lista next = temp->next->next;
 
    /* Dealloco memoria per l'elemento da eliminare */
    free(temp->next);
 
	 /* E per ultimo recupero la parte della lista precedentemente memorizzata */
    temp->next = next; 
}
