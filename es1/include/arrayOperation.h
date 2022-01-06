#ifndef ARRAYOPERATION_H
#define ARRAYOPERATION_H

#include <stdio.h>
#include <stdlib.h>

/*
 * Funzione: printArray
 * --------------------
 *  Questa funzione effettua la stampa del contenuto un array
 *
 *  v: Puntatore all'array da stampare
 *
 *  n: Dimensione dell'array
 * 
 *  str: Stringa per esplicitare che cosa si sta stampando
 *
 *  ritorno: Non e' previsto alcun valore di ritorno
 *           
*/
void printArray(int* v, int n, char* str);

/*
 * Funzione: getFirstFreePosition
 * --------------------
 *  Questa funzione ritorna la prima posizione libera (definita da un numero uguale da -1) di un array (se presente)
 *
 *  v: Puntatore all'array da cui ricercare la posizione
 *
 *  n: Dimensione dell'array
 *
 *  ritorno: Viene ritornato l'indice dell'elemento libero dell'array se presente, altrimenti
 *           viene ritornato un -1
 *           
*/
int getFirstFreePosition(int* v, int n);

/*
 * Funzione: getFirstNonFreePosition
 * --------------------
 *  Questa funzione ritorna la prima posizione NON libera (definita da un numero diverso da -1) di un array (se presente)
 *
 *  v: Puntatore all'array da cui ricercare la posizione
 *
 *  n: Dimensione dell'array
 *
 *  ritorno: Viene ritornato l'indice dell'elemento NON libero dell'array se presente, altrimenti
 *           viene ritornato un -1
 *           
*/
int getFirstNonFreePosition(int* v, int n);

/*
 * Funzione: getIndex
 * --------------------
 *  Questa funzione ritorna l'indice dell'array nel quale e' presente un determinato valore passato come parametro (se presente)
 *
 *  v: Puntatore all'array da cui ricercare il valore
 *
 *  n: Dimensione dell'array
 * 
 *  val: Valore da ricercare
 *
 *  ritorno: Viene ritornato l'indice dell'array del valore ricercato se presente, altrimenti
 *           viene ritornato un -1
 *           
*/
int getIndex(int* v, int n, int val);

#endif