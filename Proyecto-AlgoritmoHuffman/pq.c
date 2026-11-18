#include "pq.h"
#include "confirm.h"
#include <stdlib.h>

#define INITIAL_CAP 10
/* Crea la cola de prioridad PQ e inicializa sus atributos
retorna un puntero a la cola de prioridad 
retorna NULL si hubo error*/
PQ pq_create() {
    PQ pq = (PQ) malloc(sizeof(struct Heap));
    CONFIRM_NOTNULL(pq, NULL);
    /* +1 porque usamos índices 1-based (arr[0] queda vacío) */
    pq->arr = (PrioValue*)malloc(sizeof(PrioValue) * (INITIAL_CAP + 1));
    if (pq->arr == NULL) {
        free(pq);
        return NULL;
    }

    pq->cap = INITIAL_CAP;
    pq->size = 0;
    return pq;
}

/*
Agrega un valor a la cola con la prioridad dada

retorna TRUE si tuvo exito, FALSE si no
*/
BOOLEAN pq_add(PQ pq, void* valor, int prioridad) {
    CONFIRM_NOTNULL(pq, FALSE);

    /* Redimensionar si está lleno */
    if (pq->size >= pq->cap) {
        int nueva_cap = pq->cap * 2;
        PrioValue* nuevo_arr = (PrioValue*)realloc(pq->arr,
            sizeof(PrioValue) * (nueva_cap + 1));
        CONFIRM_NOTNULL(nuevo_arr, FALSE);
        pq->arr = nuevo_arr;
        pq->cap = nueva_cap;
    }

    /* Crear el nodo PrioValue (es un puntero a struct) */
    PrioValue nodo = (PrioValue)malloc(sizeof(struct _PrioValue));
    CONFIRM_NOTNULL(nodo, NULL);
    nodo->prio = prioridad;
    nodo->value = valor;

    /* Insertar al final (1-based: posición size+1) */
    pq->size++;
    pq->arr[pq->size] = nodo;

    /* Sift-up: subir hasta restaurar la propiedad del min-heap */
    int i = pq->size;
    while (i > 1) {
        int padre = i / 2;
        if (pq->arr[padre]->prio > pq->arr[i]->prio) {
            PrioValue temp = pq->arr[i];
            pq->arr[i] = pq->arr[padre];
            pq->arr[padre] = temp;
            i = padre;
        }
        else {
            break;
        }
    }
    return TRUE;
}

/* 
  Saca el valor de menor prioridad (cima del monticulo) y lo guarda en la posicion retVal (paso por referencia)
  retorna FALSE si tiene un error
  retorna TRUE si tuvo EXITO
*/
BOOLEAN pq_remove(PQ pq, void** retVal) {
    if (pq == NULL || pq->size == 0 || retVal == NULL) return FALSE;

    /* La raíz (mínimo) está en arr[1] */
    *retVal = pq->arr[1]->value;
    free(pq->arr[1]);

    /* Mover el último elemento a la raíz y reducir tamaño */
    pq->arr[1] = pq->arr[pq->size];
    pq->size--;

    /* Sift-down: bajar hasta restaurar la propiedad del min-heap */
    int i = 1;
    while (1) {
        int hijo_izq = 2 * i;
        int hijo_der = 2 * i + 1;
        int menor = i;

        if (hijo_izq <= pq->size &&
            pq->arr[hijo_izq]->prio < pq->arr[menor]->prio)
            menor = hijo_izq;

        if (hijo_der <= pq->size &&
            pq->arr[hijo_der]->prio < pq->arr[menor]->prio)
            menor = hijo_der;

        if (menor != i) {
            PrioValue temp = pq->arr[i];
            pq->arr[i] = pq->arr[menor];
            pq->arr[menor] = temp;
            i = menor;
        }
        else {
            break;
        }
    }
    return TRUE;
}

/* retorna el tama�o de la cola de prioridad, 
   retorna 0 si hubo error 
 */
int pq_size(PQ pq) {
    CONFIRM_NOTNULL(pq, 0);
    return pq->size;
}

/* Destruye la cola de prioridad, 
retorna TRUE si tuvo exito
retorna FALSE si tuvo error*/
BOOLEAN pq_destroy(PQ pq) {
    CONFIRM_NOTNULL(pq, NULL);

    /* Liberar cada nodo PrioValue individualmente */
    for (int i = 1; i <= pq->size; i++) {
        free(pq->arr[i]);
    }
    free(pq->arr);
    free(pq);
    return TRUE;
}