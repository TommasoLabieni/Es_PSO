#ifndef LISTA_H
#define LISTA_H

#include <stdio.h>
#include <stdlib.h>

/* Definizione della struttura dati */
typedef struct nodo {
	/* Elemento del nodo della lista */
	int element;
	/* Puntatore al prossimo nodo della lista */
	struct nodo* next;
} Nodo;

/* Definizione del tipo lista */
typedef Nodo* Lista;


/*
 * Funzione: esamina_lista
 * --------------------
 *  Questa funzione esamina una lista L
 *
 *  L: La lista da esaminare
 *
 *  id: Indice del Searcher (utilizzato per effettuare stampe rilevanti)
 *
 *  ritorno: Non e' previsto alcun valore di ritorno
 *           
*/
void esamina_lista(Lista L, int id);

/*
 * Funzione: aggiungi_elemento
 * --------------------
 *  Questa funzione aggiunge in coda ad una lista L, un nuovo elemento el
 *
 *  *L: Puntatore alla lista da modificare
 *
 *  el: l'elemento da aggiungere in coda
 *
 *  id: Indice dell'Inserter (utilizzato per effettuare stampe rilevanti)
 *
 *  ritorno: La funzione ritorna 0 in caso di successo
 *  		 altrimenti -1 (in caso di allocazione FALLITA del nuovo elemento)        
 *           
*/
int aggiungi_elemento(Lista* L, int el, int id);

/*
 * Funzione: elimina_elemento
 * --------------------
 *  Questa funzione elimina un elemento el da una lista L 
 *
 *  L: La lista da modificare
 *
 *  pos: La posizione dell'elemento da rimuovere
 *
 *  id: Indice del Deleter (utilizzato per effettuare stampe rilevanti)
 *
 *  ritorno: Non e' previsto alcun valore di ritorno
 *           
*/
void elimina_elemento(Lista* L, int pos, int id);

#endif
