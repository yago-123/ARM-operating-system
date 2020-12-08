/*------------------------------------------------------------------------------

	"main.c" : fase 2 / progM-Tests

	Versión test de GARLIC 2.0
	(carga de programas con 2 segmentos, listado de programas, gestión de
	 franjas de memoria)

------------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>
#include <garlic_system.h>	// definición de funciones y variables de sistema

#define NUMERO_FRANJAS 768

extern int * punixTime;		// puntero a zona de memoria con el tiempo real

const short divFreq1 = -33513982/(1024*7);		// frecuencia de TIMER1 = 7 Hz

/*** Juego pruebas _gm_reservarMem ***/ 
typedef struct 
{
	int posicionInicial; 
	int longitud; 
	int zocalo; 
} franja; 

typedef struct {
	int zocalo; 				// Identificador utilizado 
	int numBytes; 				// Numero bytes ocupa segmento 
	int direccionRetorno; 		// Direccion de retorno esperada 
	franja solucion; 			// Franja que se situara solucion _gm_reservaMem 
	franja franjaAuxiliar;  	// Franjas auxiliares para ampliar tests
	franja franjaAuxiliar2; 
} test_struct_reservar; 

test_struct_reservar test_reservarMem[] = 
{
	{1,    128, 0x01002000, {  0,   4, 1}, {0,   0, 0}, { 0,   0, 0}},	// Posiciona 4 segmentos exactos (128) en vector vacio (con espacio)
	{1,    128, 		 0, {  0,   0, 1}, {0, 768, 2}, { 0,   0, 0}},	// Posiciona 768 segmentos auxiliares (sin espacio)  
	{1,    128, 		 0, {  0,   0, 1}, {1, 767, 2}, { 0,   0, 0}}, 	// Posiciona 767 segmentos auxiliares (con espacio)
	{1,     32, 0x01002000, {  0,   1, 1}, {1, 767, 2}, { 0,   0, 0}},	// Posiciona 767 segmentos auxiliares (con espacio) 
	{1,     32, 0x01002020, {  1,   1, 1}, {0,   1, 2}, { 2, 766, 3}}, 	// Posiciona 1 segmento al principio y 766 despues de un hueco (con espacio)
	{1,     32, 0x01007FE0, {767,   1, 1}, {0, 767, 2}, { 0,   0, 0}},  // Posiciona 767 segmentos al principio (con espacio)
	{1,     25, 0x01002000, {  0,   1, 1}, {0,   0, 0}, { 0,   0, 0}},	// Posiciona numero impar bytes en vector vacio (con espacio) 
	{1,    111, 0x01002000, {  0,   4, 1}, {0,   0, 0}, { 0,   0, 0}},	// Posiciona numero impar bytes en vector vacio (con espacio)
	{1,     32,          0, {  0,   0, 1}, {0, 768, 1}, { 0,   0, 0}},	// Llena 768 segmentos auxiliares con zocalo solucion (sin espacio)
	{1, 32*768, 0x01002000, {  0, 768, 1}, {0,   0, 0}, { 0,   0, 0}},	// Ocupa todos segmentos con reservarMem (con espacio) 
	{1, 32*769,          0, {  0,   0, 1}, {0,   0, 0}, { 0,   0, 0}},	// Reserva mas segmentos (769) de los disponibles (sin espacio)
	{3,    320, 0x01002020, {  1,  10, 3}, {0,   1, 4}, {25,   5, 5}}, 	// Diferentes zocalos (con espacio)
	{4,    400, 0x01002180, { 12,  13,  4},{2,  10, 5}, {30,   6, 6}},  
};

typedef struct {
	franja franjaBorrar; 
	franja franjaAuxiliar; 
	franja franjaAuxiliar2; 
} test_struct_liberar;

test_struct_liberar test_liberarMem[] = 
{
	{{  0,  1, 1}, { 0,  0, 0}, { 0,  0, 0}},  // Borra un solo zocalo 
	{{  0, 10, 1}, {10, 20, 2}, { 0,  0, 0}},  // Borra zocalos primeras franjas  
	{{ 10, 20, 1}, { 0, 10, 2}, {10, 20, 3}},  // Borra zocalos entre dos franjas 
	{{767,  1, 1}, { 0,  0, 0}, { 0,  0, 0}},  // Borra zocalo ultima posicion 
	{{  0,  0, 0}, { 0,  0, 0}, { 0,  0, 0}},  // Borra con vector vacio 
	{{  0,  0, 0}, { 0, 10, 2}, {10, 30, 3}},  // Borra vector sin zocalo presente 
}; 

/* Pone a 0 todo el vector */ 
void reestablecerVector(unsigned char vector[], int longitud) {
	int i;  
	for(i = 0; i < longitud; i++) {
		vector[i] = (char)0; 
	}
}

/* Escribe conjunto de franjas en el vector global _gm_zocMem */ 
void escribeFranjaAuxiliar(unsigned char vector[], int longitudVector, franja datos) {
	int i; 
	if((datos.posicionInicial + datos.longitud) <= longitudVector) {
		for(i = datos.posicionInicial; i < (datos.posicionInicial + datos.longitud); i++) {
			vector[i] = (char)datos.zocalo; 
		}
	} else {
		printf("Error, franja fuera de rango!\n"); 
	}
}

/* Compara dos vectores y devuelve 1 o 0 si son iguales o no */ 
int assertVector(unsigned char vector[], unsigned char vector2[], int longitud) {
	int i, ret = 1; 
	for(i = 0; i < longitud; i++) {
		if(vector[i] != vector2[i]) {
			ret = 0; 
			printf("%d: %d %d\n", i, vector[i], vector2[i]); 
		}
	}
	
	return ret; 
}

int testLiberarMem() {
	unsigned short num_ok = 0;		// number of right tests
	unsigned short rb, rw;			// returned results
	unsigned char success;			// boolean for assessing test success
	unsigned char test_by_test;		// boolean for runing test by test
	
	printf("********************************");
	printf("*                              *");
	printf("*   Test of  _gm_liberarMem()  *");
	printf("*                              *");
	printf("********************************");
	printf("Press START to run test by test,");
	printf("press SELECT to run all tests.\n\n");
	
	int i, ret,  ok; 
	unsigned char vectorSolucion[NUMERO_FRANJAS]; 
	for(i = 0; i < (sizeof(test_liberarMem) / sizeof(test_struct_liberar)); i++) {
		ok = 1; 
		reestablecerVector(vectorSolucion, NUMERO_FRANJAS); 
		reestablecerVector(_gm_zocMem, NUMERO_FRANJAS); 
		
		// Escribe vector solucion sin la franjaBorrar
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_liberarMem[i].franjaAuxiliar); 
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_liberarMem[i].franjaAuxiliar2); 
		
		// Escribe vectores auxiliares _gm_zocMem completos 
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_liberarMem[i].franjaBorrar); 
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_liberarMem[i].franjaAuxiliar); 
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_liberarMem[i].franjaAuxiliar2);
		 
		// Ejecuta _gm_liberarMem() 
		_gm_liberarMem(test_liberarMem[i].franjaBorrar.zocalo);

		printf("Case %d: ", i); 
		// Comprueba vector 
		if(!assertVector(_gm_zocMem, vectorSolucion, NUMERO_FRANJAS)) {
			printf("error assert\n"); 
			ok = 0; 
		} else {
			printf("OK\n"); 
		}
	}
	
	do								// wait for START or SELECT
	{	swiWaitForVBlank();
		scanKeys();
	} while (!(keysDown() & (KEY_START | KEY_SELECT)));
	test_by_test = (keysDown() & KEY_START);

	return 0; 
}

int testReservarMem() {
	unsigned short num_ok = 0;		// number of right tests
	unsigned short rb, rw;			// returned results
	unsigned char success;			// boolean for assessing test success
	unsigned char test_by_test;		// boolean for runing test by test
	
	printf("********************************");
	printf("*                              *");
	printf("*  Test of  _gm_reservarMem()  *");
	printf("*                              *");
	printf("********************************");
	printf("Press START to run test by test,");
	printf("press SELECT to run all tests.\n\n");

	int i, ret,  ok; 
	unsigned char vectorSolucion[NUMERO_FRANJAS]; 
	for(i = 0; i < (sizeof(test_reservarMem) / sizeof(test_struct_reservar)); i++) {
		ok = 1; 
		reestablecerVector(vectorSolucion, NUMERO_FRANJAS); 
		reestablecerVector(_gm_zocMem, NUMERO_FRANJAS); 
		
		// Escribe vector solucion con los datos completos 
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_reservarMem[i].franjaAuxiliar); 
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_reservarMem[i].franjaAuxiliar2); 
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_reservarMem[i].solucion); 
		
		// Escribe vectores auxiliares _gm_zocMem
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_reservarMem[i].franjaAuxiliar); 
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_reservarMem[i].franjaAuxiliar2);
		 
		// Ejecuta _gm_reservarMem() y comprueba retorno 
		ret = _gm_reservarMem(test_reservarMem[i].zocalo, test_reservarMem[i].numBytes, 0);

		printf("Case %d: ", i); 
		// Comprueba retorno 
		if(ret != test_reservarMem[i].direccionRetorno) {
			printf("error retorno"); 
			ok = 0;  
		} 
		
		// Comprueba vector 
		if(!assertVector(_gm_zocMem, vectorSolucion, NUMERO_FRANJAS)) {
			printf("error assert"); 
			ok = 0;  
		}
		
		if(ok) {
			printf("OK"); 
		} 
		
		printf("\n"); 
	}
	
	do								// wait for START or SELECT
	{	swiWaitForVBlank();
		scanKeys();
	} while (!(keysDown() & (KEY_START | KEY_SELECT)));
	test_by_test = (keysDown() & KEY_START);
		
	return 0; 
} 


/* Inicializaciones generales del sistema Garlic */
//------------------------------------------------------------------------------
void inicializarSistema() {
//------------------------------------------------------------------------------
	
	consoleDemoInit();		// inicializar console, sólo para esta simulación
	
	_gd_seed = *punixTime;	// inicializar semilla para números aleatorios con
	_gd_seed <<= 16;		// el valor de tiempo real UNIX, desplazado 16 bits

	if (!_gm_initFS()) {
		printf("ERROR: ¡no se puede inicializar el sistema de ficheros!");
		exit(0);
	}
}

//------------------------------------------------------------------------------
int main(int argc, char **argv) {
//------------------------------------------------------------------------------
	intFunc start;
	inicializarSistema();
	testReservarMem(); 
	testLiberarMem(); 
	return 0;
}

