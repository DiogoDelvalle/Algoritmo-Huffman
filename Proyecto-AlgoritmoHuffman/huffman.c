#include "huffman.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arbol.h"
#include "pq.h"
#include "bitstream.h"
#include "confirm.h"

/*====================================================
     Constantes
  ====================================================*/

#define NUM_CHARS 256

  /*
    Estructura para almacenar el valor de un nodo del arbol.
    c    = el caracter (significativo solo en nodos hoja)
    frec = la frecuencia / peso del nodo
  */
typedef struct _keyvaluepair {
    char c;
    int frec;
} keyvaluepair;

/*====================================================
     Campo de bits
  ====================================================*/

typedef struct _campobits {
    unsigned int bits;
    int tamano;
} campobits;

/*
  Agrega un bit al campobits usando aritmetica de bits.
  El primer bit agregado queda en la posicion 0 (LSB).
*/
static void bits_agregar(campobits* bits, int bit) {
    CONFIRM_RETURN(bits);
    CONFIRM_RETURN((unsigned int)bits->tamano < 8 * sizeof(bits->bits));
    bits->tamano++;
    if (bit) {
        bits->bits = bits->bits | (0x1 << (bits->tamano - 1));
    }
}

/*
  Lee el bit en la posicion pos dentro del campobits.
  pos=0 es el primer bit agregado.
*/
static int bits_leer(campobits* bits, int pos) {
    CONFIRM_TRUE(bits, 0);
    CONFIRM_TRUE(!(pos < 0 || pos > bits->tamano), 0);
    int bit = (bits->bits & (0x1 << (pos))) >> (pos);
    return bit;
}

static void testCampobitsBitstream() {
    BitStream bs = NULL;
    BitStream bsIn = NULL;
    char* testbitstreamtxt = "testbitsteam.txt";
    int i = 0;
    campobits* b = (campobits*)malloc(sizeof(struct _campobits));
    CONFIRM_RETURN(b);
    b->bits = 0;
    b->tamano = 0;
    bits_agregar(b, 0);
    bits_agregar(b, 0);
    bits_agregar(b, 1);
    bits_agregar(b, 0);
    bits_agregar(b, 1);
    bs = OpenBitStream(testbitstreamtxt, "w");
    for (i = 0; i < b->tamano; i++) {
        int bit = bits_leer(b, i);
        PutBit(bs, bit);
    }
    PutByte(bs, 'z');
    CloseBitStream(bs);
    free(b);
    bsIn = OpenBitStream(testbitstreamtxt, "r");
    printf("%d", GetBit(bsIn));
    printf("%d", GetBit(bsIn));
    printf("%d", GetBit(bsIn));
    printf("%d", GetBit(bsIn));
    printf("%d", GetBit(bsIn));
    printf("%c\n", GetByte(bsIn));
    CloseBitStream(bsIn);
}

static void imprimirNodoEjemplo(Arbol nodo) {
    CONFIRM_RETURN(nodo);
    keyvaluepair* val = (keyvaluepair*)arbol_valor(nodo);
    printf("%d,%c", val->frec, val->c);
}

static void testArbol() {
    keyvaluepair* v1 = malloc(sizeof(struct _keyvaluepair));
    keyvaluepair* v2 = malloc(sizeof(struct _keyvaluepair));
    keyvaluepair* v3 = malloc(sizeof(struct _keyvaluepair));
    keyvaluepair* v11 = malloc(sizeof(struct _keyvaluepair));
    keyvaluepair* v12 = malloc(sizeof(struct _keyvaluepair));
    Arbol n1, n2, n3, n11, n12;
    CONFIRM_RETURN(v1);  CONFIRM_RETURN(v2);  CONFIRM_RETURN(v3);
    CONFIRM_RETURN(v11); CONFIRM_RETURN(v12);
    v1->c = ' ';  v1->frec = 3;
    v2->c = 's';  v2->frec = 2;
    v3->c = ' ';  v3->frec = 5;
    v11->c = 'e'; v11->frec = 2;
    v12->c = 'w'; v12->frec = 1;
    n1 = arbol_crear(v1);
    n2 = arbol_crear(v2);
    n3 = arbol_crear(v3);
    n11 = arbol_crear(v11);
    n12 = arbol_crear(v12);
    arbol_agregarIzq(n3, n2);
    arbol_agregarDer(n3, n1);
    arbol_agregarIzq(n1, n11);
    arbol_agregarDer(n1, n12);
    arbol_imprimir(n3, imprimirNodoEjemplo);
    arbol_destruir(n3);
    free(v1); free(v2); free(v3); free(v11); free(v12);
}

void campobitsDemo() {
    printf("***************CAMPOBITS DEMO*******************\n");
    testCampobitsBitstream();
    printf("***************ARBOL DEMO******************\n");
    testArbol();
    printf("***************FIN DEMO*****************\n");
}

/*====================================================
     Declaraciones de funciones privadas
  ====================================================*/

static int   calcular_frecuencias(int* frecuencias, char* entrada);
static Arbol crear_huffman(int* frecuencias);
static int   codificar(Arbol T, char* entrada, char* salida, int total_chars);
static void  crear_tabla(campobits* tabla, Arbol T, campobits* bits);

static Arbol leer_arbol(BitStream bs);
static void  decodificar(BitStream in, FILE* out, Arbol arbol, int total_chars);

static void  imprimirNodo(Arbol nodo);

/* Auxiliares que no estaban en el esqueleto original */
static void  escribir_nodo(Arbol nodo, void* dato);
static void  destruir_valores(Arbol nodo, void* ignorado);

/*====================================================
     Implementacion de funciones publicas
  ====================================================*/

  /*
    Comprime el archivo 'entrada' y escribe el resultado en 'salida'.
    Retorna 0 si no hay errores.
  */
int comprimir(char* entrada, char* salida) {
    int frecuencias[NUM_CHARS];
    int total_chars = 0;
    int i;
    Arbol arbol = NULL;

    /* -------------------------------------------------------
       PASO 1: primer recorrido del archivo — contar cuantas
       veces aparece cada byte. El propio byte sirve de indice
       en el arreglo frecuencias[256].
    ------------------------------------------------------- */
    CONFIRM_TRUE(0 == calcular_frecuencias(frecuencias, entrada), 1);

    /* -------------------------------------------------------
       PASO 2: construir el arbol de Huffman.
       Algoritmo: insertar cada caracter con frec > 0 en una
       min-heap (PQ). Repetidamente sacar los dos de menor peso,
       combinarlos en un nodo padre y reinsertarlo, hasta que
       quede un solo arbol.
    ------------------------------------------------------- */
    arbol = crear_huffman(frecuencias);
    CONFIRM_TRUE(arbol, 1);

    printf("\n=== Arbol de Huffman (compresion) ===\n");
    arbol_imprimir(arbol, imprimirNodo);

    /* Calcular total de caracteres para el encabezado */
    for (i = 0; i < NUM_CHARS; i++) total_chars += frecuencias[i];

    /* -------------------------------------------------------
       PASO 3: segundo recorrido del archivo — codificar.
       Escribe en 'salida':
         [4 bytes: total_chars little-endian]
         [arbol serializado en preorden]
         [texto codificado bit a bit segun la tabla Huffman]
    ------------------------------------------------------- */
    if (0 != codificar(arbol, entrada, salida, total_chars)) {
        arbol_postorden(arbol, destruir_valores, NULL);
        arbol_destruir(arbol);
        return 1;
    }

    /* Liberar memoria */
    arbol_postorden(arbol, destruir_valores, NULL);
    arbol_destruir(arbol);

    return 0;
}

/*
  Descomprime el archivo 'entrada' y escribe el resultado en 'salida'.
  Retorna 0 si no hay errores.
*/
int descomprimir(char* entrada, char* salida) {
    BitStream in = 0;
    FILE* out = NULL;
    Arbol arbol = NULL;
    int total_chars = 0;

    /* -------------------------------------------------------
       Abrir el archivo comprimido en modo BINARIO.

       CRITICO para Windows: el modo texto ("r") convierte la
       secuencia 0x0D 0x0A a 0x0A al leer, y trata 0x1A como
       EOF anticipado. Cualquiera de esos efectos corrompe el
       flujo de bits del .huff. Pasamos "rb"; OpenBitStream
       solo chequea el primer caracter ('r') para decidir el
       modo lectura/escritura, por lo que "rb" funciona igual.
    ------------------------------------------------------- */
    in = OpenBitStream(entrada, "rb");
    CONFIRM_TRUE(in, 1);

    /* -------------------------------------------------------
       PASO 1: leer el encabezado de 4 bytes con el numero
       exacto de caracteres del archivo original (little-endian).
       Esto es imprescindible para saber cuando parar de
       decodificar y no producir caracteres basura de los bits
       de relleno (padding) al final del ultimo byte.
    ------------------------------------------------------- */
    total_chars = (int)GetByte(in);
    total_chars |= ((int)GetByte(in) << 8);
    total_chars |= ((int)GetByte(in) << 16);
    total_chars |= ((int)GetByte(in) << 24);

    /* -------------------------------------------------------
       PASO 2: reconstruir el arbol de Huffman.
       Fue serializado en preorden: bit 0 = nodo interno,
       bit 1 + byte ASCII = nodo hoja.
    ------------------------------------------------------- */
    arbol = leer_arbol(in);
    if (!arbol) {
        CloseBitStream(in);
        return 1;
    }

    printf("\n=== Arbol de Huffman (descompresion) ===\n");
    arbol_imprimir(arbol, imprimirNodo);

    /* -------------------------------------------------------
       PASO 3: abrir el archivo de salida como FILE* binario.
       No usamos BitStream aqui porque la salida son bytes
       completos (caracteres decodificados), no bits sueltos.
       Usar BitStream agregaría un byte de conteo al cerrar
       el archivo, corrompiendo el resultado.
    ------------------------------------------------------- */
    out = fopen(salida, "wb");
    if (!out) {
        CloseBitStream(in);
        arbol_postorden(arbol, destruir_valores, NULL);
        arbol_destruir(arbol);
        return 1;
    }

    /* -------------------------------------------------------
       PASO 4: decodificar.
       Navegar el arbol con cada bit leido (0=izq, 1=der).
       Al llegar a una hoja escribir su caracter.
       Parar cuando se hayan producido exactamente total_chars
       caracteres (el contador del encabezado).
    ------------------------------------------------------- */
    decodificar(in, out, arbol, total_chars);

    CloseBitStream(in);
    fclose(out);

    /* Liberar memoria del arbol */
    arbol_postorden(arbol, destruir_valores, NULL);
    arbol_destruir(arbol);

    return 0;
}

/*====================================================
     Implementacion de funciones privadas
  ====================================================*/

  /*
    Abre 'entrada' y cuenta cuantas veces aparece cada byte.
    Utiliza el valor del byte como indice del arreglo frecuencias[256].
    Retorna 0 si no hay errores.
  */
static int calcular_frecuencias(int* frecuencias, char* entrada) {
    int i, c;
    FILE* f = NULL;

    /* Paso 1: inicializar todo en cero */
    for (i = 0; i < NUM_CHARS; i++) {
        frecuencias[i] = 0;
    }

    /* Paso 2: abrir y leer el archivo byte a byte */
    f = fopen(entrada, "rb");
    CONFIRM_TRUE(f, 1);

    while ((c = fgetc(f)) != EOF) {
        frecuencias[(unsigned char)c]++;
    }

    fclose(f);
    return 0;
}

/*
  Construye el arbol de Huffman usando una cola de prioridades (min-heap).
  Algoritmo:
    1. Crear un nodo hoja por cada caracter con frec > 0.
    2. Insertar todos los nodos en la PQ con prioridad = frecuencia.
    3. Mientras la PQ tenga mas de un elemento:
         a. Sacar los dos nodos de menor frecuencia (t1, t2).
         b. Crear un nodo padre con frec = frec(t1)+frec(t2).
         c. t1 -> hijo izquierdo, t2 -> hijo derecho.
         d. Reinsertar el padre en la PQ.
    4. El ultimo elemento de la PQ es la raiz del arbol.
*/
static Arbol crear_huffman(int* frecuencias) {
    int i;
    PQ pq;
    Arbol raiz = NULL;

    pq = pq_create();
    CONFIRM_NOTNULL(pq, NULL);

    /* Crear nodos hoja para cada caracter presente */
    for (i = 0; i < NUM_CHARS; i++) {
        if (frecuencias[i] > 0) {
            keyvaluepair* kv = (keyvaluepair*)malloc(sizeof(struct _keyvaluepair));
            Arbol nodo;
            CONFIRM_NOTNULL(kv, NULL);
            kv->c = (char)i;
            kv->frec = frecuencias[i];
            nodo = arbol_crear(kv);
            CONFIRM_NOTNULL(nodo, NULL);
            pq_add(pq, nodo, frecuencias[i]);
        }
    }

    /* Combinar los dos arboles de menor peso hasta quedar con uno */
    while (pq_size(pq) > 1) {
        Arbol t1 = NULL, t2 = NULL, padre = NULL;
        keyvaluepair* kv1 = NULL;
        keyvaluepair* kv2 = NULL;
        keyvaluepair* kvPadre = NULL;
        int frecPadre;

        pq_remove(pq, (void**)&t1);   /* menor frecuencia */
        pq_remove(pq, (void**)&t2);   /* segunda menor    */

        kv1 = (keyvaluepair*)arbol_valor(t1);
        kv2 = (keyvaluepair*)arbol_valor(t2);

        frecPadre = kv1->frec + kv2->frec;

        /* Nodo interno: c=0 indica que no es hoja */
        kvPadre = (keyvaluepair*)malloc(sizeof(struct _keyvaluepair));
        CONFIRM_NOTNULL(kvPadre, NULL);
        kvPadre->c = 0;
        kvPadre->frec = frecPadre;

        padre = arbol_crear(kvPadre);
        CONFIRM_NOTNULL(padre, NULL);

        arbol_agregarIzq(padre, t1);
        arbol_agregarDer(padre, t2);

        pq_add(pq, padre, frecPadre);
    }

    /* Extraer la raiz */
    if (pq_size(pq) == 1) {
        pq_remove(pq, (void**)&raiz);
    }

    pq_destroy(pq);
    return raiz;
}

/*
  Funcion de visita para arbol_preorden.
  Escribe al BitStream:
    - bit 0 si el nodo es interno
    - bit 1 + byte ASCII si el nodo es hoja
  Esto serializa el arbol para poder reconstruirlo al descomprimir.
*/
static void escribir_nodo(Arbol nodo, void* dato) {
    BitStream bs = (BitStream)dato;

    if (!arbol_izq(nodo) && !arbol_der(nodo)) {
        /* Hoja */
        keyvaluepair* kv = (keyvaluepair*)arbol_valor(nodo);
        PutBit(bs, 1);
        PutByte(bs, (unsigned char)kv->c);
    }
    else {
        /* Nodo interno */
        PutBit(bs, 0);
    }
}

/*
  Funcion de visita para arbol_postorden.
  Libera el keyvaluepair de cada nodo sin tocar la estructura del arbol.
  Debe llamarse ANTES de arbol_destruir().
*/
static void destruir_valores(Arbol nodo, void* ignorado) {
    if (nodo) {
        keyvaluepair* kv = (keyvaluepair*)arbol_valor(nodo);
        if (kv) free(kv);
    }
}

/*
  Llena la tabla de codigos Huffman recorriendo el arbol recursivamente.
  Cada vez que se baja a la izquierda se agrega un bit 0,
  cada vez que se baja a la derecha se agrega un bit 1.
  Al llegar a una hoja se guarda el campobits acumulado en tabla[c].

  Parametros:
    tabla - arreglo de NUM_CHARS campobits, uno por caracter
    T     - nodo actual del arbol
    bits  - codigo acumulado hasta este nodo
*/
static void crear_tabla(campobits* tabla, Arbol T, campobits* bits) {
    if (!T) return;

    if (!arbol_izq(T) && !arbol_der(T)) {
        /* Nodo hoja: registrar el codigo */
        keyvaluepair* kv = (keyvaluepair*)arbol_valor(T);
        /* Caso especial: arbol con un solo caracter -> codigo = "0" */
        if (bits->tamano == 0) {
            bits_agregar(bits, 0);
        }
        tabla[(unsigned char)kv->c] = *bits;
        return;
    }

    /* Bajar izquierda con bit 0 */
    {
        campobits izq = *bits;
        bits_agregar(&izq, 0);
        crear_tabla(tabla, arbol_izq(T), &izq);
    }

    /* Bajar derecha con bit 1 */
    {
        campobits der = *bits;
        bits_agregar(&der, 1);
        crear_tabla(tabla, arbol_der(T), &der);
    }
}

/*
  Codifica el archivo 'entrada' usando el arbol T y escribe en 'salida'.

  Formato del archivo comprimido:
    [arbol serializado en preorden] [texto codificado bit a bit]

  El arbol se guarda primero para que el descompresor pueda reconstruirlo.
  El texto se reemplaza caracter por caracter con su codigo Huffman.
*/
static int codificar(Arbol T, char* entrada, char* salida, int total_chars) {
    FILE* in = NULL;
    BitStream out = NULL;
    campobits tabla[NUM_CHARS];
    campobits bits_raiz;
    int c, i;

    /* Inicializar tabla */
    memset(tabla, 0, NUM_CHARS * sizeof(struct _campobits));

    /* Construir tabla de codigos */
    bits_raiz.bits = 0;
    bits_raiz.tamano = 0;
    crear_tabla(tabla, T, &bits_raiz);

    /* Abrir archivos */
    in = fopen(entrada, "rb");
    CONFIRM_TRUE(in, 1);

    /* Abrir archivo de salida en modo BINARIO ("wb").
       En Windows el modo texto ("w") inserta 0x0D antes de cada 0x0A,
       corrompiendo el archivo comprimido al escribirlo. */
    out = OpenBitStream(salida, "wb");
    if (!out) { fclose(in); return 1; }

    /* --- Paso 0: escribir total de bytes originales (4 bytes, LSB primero) ---
       Esto permite al decodificador saber exactamente cuando parar,
       evitando el problema de los bits de relleno (padding). */
    PutByte(out, (unsigned char)(total_chars & 0xFF));
    PutByte(out, (unsigned char)((total_chars >> 8) & 0xFF));
    PutByte(out, (unsigned char)((total_chars >> 16) & 0xFF));
    PutByte(out, (unsigned char)((total_chars >> 24) & 0xFF));

    /* --- Paso 1: serializar el arbol en preorden --- */
    arbol_preorden(T, escribir_nodo, out);

    /* --- Paso 2: codificar cada caracter del archivo de entrada --- */
    while ((c = fgetc(in)) != EOF) {
        unsigned char uc = (unsigned char)c;
        for (i = 0; i < tabla[uc].tamano; i++) {
            int bit = 0 != (tabla[uc].bits & (0x1 << i));
            PutBit(out, bit);
        }
    }

    fclose(in);
    CloseBitStream(out);
    return 0;
}

/*
  Reconstruye el arbol de Huffman leyendo el BitStream en preorden.
  Formato:
    bit 0 -> nodo interno: leer recursivamente hijo izq y luego hijo der
    bit 1 -> nodo hoja:    leer el siguiente byte como caracter ASCII
*/
static Arbol leer_arbol(BitStream bs) {
    int bit = GetBit(bs);

    if (bit == 1) {
        /* Nodo hoja */
        unsigned char c = GetByte(bs);
        keyvaluepair* kv = (keyvaluepair*)malloc(sizeof(struct _keyvaluepair));
        CONFIRM_NOTNULL(kv, NULL);
        kv->c = (char)c;
        kv->frec = 0;   /* frecuencia no se necesita para descomprimir */
        return arbol_crear(kv);
    }
    else {
        /* Nodo interno: crear nodo y leer los dos subarboles */
        keyvaluepair* kv = (keyvaluepair*)malloc(sizeof(struct _keyvaluepair));
        Arbol nodo;
        CONFIRM_NOTNULL(kv, NULL);
        kv->c = 0;
        kv->frec = 0;
        nodo = arbol_crear(kv);
        CONFIRM_NOTNULL(nodo, NULL);

        arbol_agregarIzq(nodo, leer_arbol(bs));
        arbol_agregarDer(nodo, leer_arbol(bs));

        return nodo;
    }
}

/*
  Decodifica el stream de bits usando el arbol de Huffman.
  Algoritmo:
    - Empezar en la raiz del arbol.
    - Leer un bit: 0 -> ir a hijo izquierdo, 1 -> ir a hijo derecho.
    - Si se llega a una hoja, escribir su caracter en 'out' y
      volver a la raiz para el siguiente caracter.
    - Repetir hasta que no haya mas bits en 'in'.
*/
static void decodificar(BitStream in, FILE* out, Arbol arbol, int total_chars) {
    Arbol actual = arbol;
    int decodificados = 0;

    /* Caso especial: un unico caracter en el archivo original */
    if (!arbol_izq(arbol) && !arbol_der(arbol)) {
        while (decodificados < total_chars && !IsEmptyBitStream(in)) {
            keyvaluepair* kv;
            GetBit(in);   /* consumir el bit "dummy" del unico caracter */
            kv = (keyvaluepair*)arbol_valor(arbol);
            fputc((unsigned char)kv->c, out);
            decodificados++;
        }
        return;
    }

    /* Navegar el arbol bit a bit, escribir al llegar a una hoja.
       Paramos cuando hayamos decodificado exactamente total_chars bytes,
       evitando que bits de relleno (padding) produzcan caracteres de mas. */
    while (decodificados < total_chars && !IsEmptyBitStream(in)) {
        int bit = GetBit(in);

        actual = (bit == 0) ? arbol_izq(actual) : arbol_der(actual);

        /* Hoja alcanzada: escribir caracter y reiniciar desde la raiz */
        if (!arbol_izq(actual) && !arbol_der(actual)) {
            keyvaluepair* kv = (keyvaluepair*)arbol_valor(actual);
            fputc((unsigned char)kv->c, out);
            decodificados++;
            actual = arbol;
        }
    }
}

/*
  Imprime la informacion de un nodo para depuracion visual.
  Hojas muestran el caracter y su frecuencia.
  Nodos internos muestran solo la frecuencia acumulada.
*/
static void imprimirNodo(Arbol nodo) {
    keyvaluepair* kv;
    if (!nodo) return;
    kv = (keyvaluepair*)arbol_valor(nodo);
    if (!kv) { printf("(null)"); return; }

    if (!arbol_izq(nodo) && !arbol_der(nodo)) {
        /* Hoja: mostrar caracter (con escape si es especial) */
        if (kv->c == '\n') printf("[HOJA] '\\n' frec=%d", kv->frec);
        else if (kv->c == '\r') printf("[HOJA] '\\r' frec=%d", kv->frec);
        else if (kv->c == ' ')  printf("[HOJA] ' '  frec=%d", kv->frec);
        else                    printf("[HOJA] '%c'  frec=%d", kv->c, kv->frec);
    }
    else {
        /* Nodo interno */
        printf("[INT]        frec=%d", kv->frec);
    }
}