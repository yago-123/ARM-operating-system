/*------------------------------------------------------------------------------

	"main.c" : fase 2 / progM

	Versión final de GARLIC 2.0
	(carga de programas con 2 segmentos, listado de programas, gestión de
	 franjas de memoria)

------------------------------------------------------------------------------*/
#include <nds.h>
#include <garlic_system.h>	// definición de funciones y variables de sistema

#define NUMERO_FRANJAS 768


extern int * punixTime;		// puntero a zona de memoria con el tiempo real

char indicadoresTeclas[8] = {'^', '>', 'v', '<', 'A', 'B', 'L', 'R'};
unsigned short bitsTeclas[8] = {KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_LEFT, KEY_A, KEY_B,
									KEY_L, KEY_R};


const short divFreq1 = -33513982/(1024*7);		// frecuencia de TIMER1 = 7 Hz

/*
		** ESTRUCTURAS TEST ** 
*/ 	 
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
	char funcion[80]; 
} test_struct_reservar; 

typedef struct {
	franja franjaBorrar; 
	franja franjaAuxiliar; 
	franja franjaAuxiliar2; 
	char funcion[80];  
} test_struct_liberar;

/*
		** PROVES UNITARIES ** 
*/ 
test_struct_reservar test_reservarMem[] = 
{
	{1,    128, 0x01002000, {  0,   4, 1}, {0,   0, 0}, { 0,   0, 0}, {"Posiciona 4 segmentos exactos (128) en vector vacio (con espacio)"}},
	{1,    128, 		 0, {  0,   0, 1}, {0, 768, 2}, { 0,   0, 0}, {"Posiciona 768 segmentos auxiliares (sin espacio)" }},
	{1,     32, 0x01002000, {  0,   1, 1}, {1, 767, 2}, { 0,   0, 0}, {"Posiciona 767 segmentos auxiliares (con espacio), reserva primera franja"}},
	{1,     32, 0x01002020, {  1,   1, 1}, {0,   1, 2}, { 2, 766, 3}, {"Posiciona 1 segmento al principio y 766 despues de un hueco (con espacio)"}},
	{1,     32, 0x01007FE0, {767,   1, 1}, {0, 767, 2}, { 0,   0, 0}, {"Posiciona 767 segmentos al principio (con espacio), reserva ultima franja"}},
	{1,    259, 0x01002000, {  0,   9, 1}, {0,   0, 0}, { 0,   0, 0}, {"Posiciona numero impar bytes en vector vacio (con espacio)"}},
	{1,    999, 0x01002000, {  0,  32, 1}, {0,   0, 0}, { 0,   0, 0}, {"Posiciona numero impar bytes en vector vacio (con espacio)"}},
	{1,     32,          0, {  0,   0, 1}, {0, 768, 1}, { 0,   0, 0}, {"Llena 768 segmentos auxiliares con zocalo solucion (sin espacio)"}},
	{1, 32*768, 0x01002000, {  0, 768, 1}, {0,   0, 0}, { 0,   0, 0}, {"Ocupa todos segmentos con reservarMem (con espacio)"}}, 
	{1, 32*769,          0, {  0,   0, 1}, {0,   0, 0}, { 0,   0, 0}, {"Reserva mas segmentos (769) de los disponibles (sin espacio)"}},
	{3,    320, 0x01002020, {  1,  10, 3}, {0,   1, 4}, {25,   5, 5}, {"Diferentes zocalos (con espacio)"}},
	{4,    400, 0x01002180, { 12,  13,  4},{2,  10, 5}, {30,   6, 6}, {"Posiciona segmento entre dos auxiliares"}},  
};


test_struct_liberar test_liberarMem[] = 
{
	{{  0,  100, 1}, { 0,  0, 0}, { 0,  0, 0}, {"Borra un solo zocalo"}}, 
	{{  0, 10, 1}, {10, 20, 2}, { 0,  0, 0}, {"Borra zocalos primeras franjas"}}, 
	{{ 10, 20, 1}, { 0, 10, 2}, {20, 20, 3}, {"Borra zocalos entre dos franjas"}},   
	{{767,  1, 1}, { 0,  0, 0}, { 0,  0, 0}, {"Borra zocalo ultima posicion"}},  
	{{  0,  0, 0}, { 0,  0, 0}, { 0,  0, 0}, {"Borra con vector vacio"}},   
	{{  0,  0, 0}, { 0, 10, 2}, {10, 30, 3}, {"Borra vector sin zocalo presente"}},   
}; 


/*
		** FUNCIONES AUXILIARES ** 
*/

/* Función para gestionar los sincronismos  */
void gestionSincronismos()
{
	int i, mask;
	
	if (_gd_sincMain & 0xFFFE)		// si hay algun sincronismo pendiente
	{
		mask = 2;
		for (i = 1; i <= 15; i++)
		{
			if (_gd_sincMain & mask)
			{	// liberar la memoria del proceso terminado
				_gm_liberarMem(i);
				_gg_escribir("* %d: proceso terminado\n", i, 0, 0);
				_gs_dibujarTabla();
				_gd_sincMain &= ~mask;		// poner bit a cero
			}
			mask <<= 1;
		}
	}
}


/* Función para escoger una opción con un botón (tecla) de la NDS */
int leerTecla(int num_opciones)
{
	int i, j, k;

	i = -1;							// marca de no selección
	do {
		_gp_WaitForVBlank();
		gestionSincronismos();
		scanKeys();
		k = keysDown();				// leer botones
		if (k != 0)
			for (j = 0; (j < num_opciones) && (i == -1); j++)
				if (k & bitsTeclas[j])
					i = j;			// detección de una opción válida
	} while (i == -1);
	return i;
}


/* Función para presentar una lista de opciones de tipo 'string' y escoger una */
int escogerString(char *strings[], int num_opciones)
{
	int j;
	
	for (j = 0; j < num_opciones; j++)
	{								// mostrar opciones
		_gg_escribir("%c: %s\n", indicadoresTeclas[j], (unsigned int) strings[j], 0);
	}
	return leerTecla(num_opciones);
}

/* Función para presentar una lista de opciones de tipo 'número' y escoger una */
int escogerNumero(const unsigned char numeros[], int num_opciones)
{
	int j;

	for (j = 0; j < num_opciones; j++)
	{								// mostrar opciones
		_gg_escribir("%c: %d\n", indicadoresTeclas[j], numeros[j], 0);
	}
	return numeros[leerTecla(num_opciones)];
}



/* Inicializaciones generales del sistema Garlic */
//------------------------------------------------------------------------------
void inicializarSistema() {
//------------------------------------------------------------------------------
	int v;

	_gg_iniGrafA();			// inicializar procesadores gráficos
	_gs_iniGrafB();
	for (v = 0; v < 4; v++)	// para todas las ventanas
		_gd_wbfs[v].pControl = 0;		// inicializar los buffers de ventana
	_gs_dibujarTabla();

	_gd_seed = *punixTime;	// inicializar semilla para números aleatorios con
	_gd_seed <<= 16;		// el valor de tiempo real UNIX, desplazado 16 bits
	
	_gd_pcbs[0].keyName = 0x4C524147;	// "GARL"
	
	if (!_gm_initFS()) {
		_gg_escribir("ERROR: ¡no se puede inicializar el sistema de ficheros!", 0, 0, 0);
		exit(0);
	}

	irqInitHandler(_gp_IntrMain);	// instalar rutina principal interrupciones
	irqSet(IRQ_VBLANK, _gp_rsiVBL);	// instalar RSI de vertical Blank
	irqEnable(IRQ_VBLANK);			// activar interrupciones de vertical Blank
	
	irqSet(IRQ_TIMER1, _gm_rsiTIMER1);
	irqEnable(IRQ_TIMER1);				// instalar la RSI para el TIMER1
	TIMER1_DATA = divFreq1; 
	TIMER1_CR = 0xC3;  	// Timer Start | IRQ Enabled | Prescaler 3 (F/1024)
	
	REG_IME = IME_ENABLE;			// activar las interrupciones en general
}


/*
		FUNCIONES TEST: 
*/



/* Pone a 0 todo el vector */ 
void reestablecerVector(unsigned char vector[], int longitud) {
	int i;  
	for(i = 0; i < longitud; i++) {
		vector[i] = (char)0; 
	}
	
	_gm_pintarFranjas(0, 0, NUMERO_FRANJAS, 0); 
}

/* Escribe conjunto de franjas en el vector global _gm_zocMem */ 
void escribeFranjaAuxiliar(unsigned char vector[], int longitudVector, franja datos) {
	int i; 
	if((datos.posicionInicial + datos.longitud) <= longitudVector) {
		for(i = datos.posicionInicial; i < (datos.posicionInicial + datos.longitud); i++) {
			vector[i] = (char)datos.zocalo; 
		}
	} else {
		_gg_escribir("Error, franja fuera de rango!\n", 0, 0, 0); 
	}
}

/* Compara dos vectores y devuelve 1 o 0 si son iguales o no */ 
int assertVector(unsigned char vector[], unsigned char vector2[], int longitud) {
	int i, ret = 1; 
	for(i = 0; i < longitud; i++) {
		if(vector[i] != vector2[i]) {
			ret = 0; 
			_gg_escribir("%d: %d", i, vector[i], 0); 
			_gg_escribir(" %d\n", vector2[i], 0, 0); 
		}
	}
	
	return ret; 
}

int testLiberarMem() {
	unsigned short num_ok = 0;		// number of right tests
	unsigned short rb, rw;			// returned results
	unsigned char success;			// boolean for assessing test success
	unsigned char test_by_test;		// boolean for runing test by test
	
	_gg_escribir("********************************", 0, 0, 0);
	_gg_escribir("*                              *", 0, 0, 0);
	_gg_escribir("*   Test of  _gm_liberarMem()  *", 0, 0, 0);
	_gg_escribir("*                              *", 0, 0, 0);
	_gg_escribir("********************************", 0, 0, 0);
	_gg_escribir("Press START to run test by test,", 0, 0, 0);
	_gg_escribir("press SELECT to run all tests.\n\n", 0, 0, 0);
	
	int i, ret,  ok; 
	unsigned char vectorSolucion[NUMERO_FRANJAS]; 
	for(i = 0; i < (sizeof(test_liberarMem) / sizeof(test_struct_liberar)); i++) {
		ok = 1; 
		reestablecerVector(vectorSolucion, NUMERO_FRANJAS); 
		reestablecerVector(_gm_zocMem, NUMERO_FRANJAS); 

		_gg_escribir("Case %d: %s\n", i, test_liberarMem[i].funcion, 0);
		
		// Escribe vector solucion sin la franjaBorrar
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_liberarMem[i].franjaAuxiliar); 
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_liberarMem[i].franjaAuxiliar2); 
		
		// Escribe vectores auxiliares _gm_zocMem completos 
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_liberarMem[i].franjaBorrar);
		_gm_pintarFranjas(test_liberarMem[i].franjaBorrar.zocalo, test_liberarMem[i].franjaBorrar.posicionInicial, 
					test_liberarMem[i].franjaBorrar.longitud, 0); 
		
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_liberarMem[i].franjaAuxiliar); 
		_gm_pintarFranjas(test_liberarMem[i].franjaAuxiliar.zocalo, test_liberarMem[i].franjaAuxiliar.posicionInicial, 
					test_liberarMem[i].franjaAuxiliar.longitud, 0); 
		
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_liberarMem[i].franjaAuxiliar2);
		_gm_pintarFranjas(test_liberarMem[i].franjaAuxiliar2.zocalo, test_liberarMem[i].franjaAuxiliar2.posicionInicial, 
					test_liberarMem[i].franjaAuxiliar2.longitud, 0); 
		 
		leerTecla(4); 

		// Ejecuta _gm_liberarMem() 
		_gm_liberarMem(test_liberarMem[i].franjaBorrar.zocalo);
 
		// Comprueba vector 
		if(!assertVector(_gm_zocMem, vectorSolucion, NUMERO_FRANJAS)) {
			_gg_escribir("error assert\n", 0, 0, 0); 
			ok = 0; 
		} else {
			_gg_escribir("OK\n", 0, 0, 0); 
		}
		
		leerTecla(4); 
	}

	return 0; 
} 

int testReservarMem() {
	unsigned short num_ok = 0;		// number of right tests
	unsigned short rb, rw;			// returned results
	unsigned char success;			// boolean for assessing test success
	unsigned char test_by_test;		// boolean for runing test by test
	
	_gg_escribir("********************************", 0, 0, 0);
	_gg_escribir("*                              *", 0, 0, 0);
	_gg_escribir("*  Test of  _gm_reservarMem()  *", 0, 0, 0);
	_gg_escribir("*                              *", 0, 0, 0);
	_gg_escribir("********************************", 0, 0, 0);
	_gg_escribir("Press START to run test by test,", 0, 0, 0);
	_gg_escribir("press SELECT to run all tests.\n\n", 0, 0, 0);

	int i, ret,  ok; 
	unsigned char vectorSolucion[NUMERO_FRANJAS]; 
	for(i = 0; i < (sizeof(test_reservarMem) / sizeof(test_struct_reservar)); i++) {
		ok = 1; 
		reestablecerVector(vectorSolucion, NUMERO_FRANJAS); 
		reestablecerVector(_gm_zocMem, NUMERO_FRANJAS); 

		_gg_escribir("Case %d: %s\n", i, test_reservarMem[i].funcion, 0); 
		// Escribe vector solucion con los datos completos 
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_reservarMem[i].franjaAuxiliar); 
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_reservarMem[i].franjaAuxiliar2); 
		escribeFranjaAuxiliar(vectorSolucion, NUMERO_FRANJAS, test_reservarMem[i].solucion); 
		
		// Escribe vectores auxiliares _gm_zocMem
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_reservarMem[i].franjaAuxiliar); 
		_gm_pintarFranjas(test_reservarMem[i].franjaAuxiliar.zocalo, test_reservarMem[i].franjaAuxiliar.posicionInicial, 
				test_reservarMem[i].franjaAuxiliar.longitud, 0); 
				
		escribeFranjaAuxiliar(_gm_zocMem, NUMERO_FRANJAS, test_reservarMem[i].franjaAuxiliar2);
		_gm_pintarFranjas(test_reservarMem[i].franjaAuxiliar2.zocalo, test_reservarMem[i].franjaAuxiliar2.posicionInicial, 
				test_reservarMem[i].franjaAuxiliar2.longitud, 0); 

		leerTecla(4); 
		 
		// Ejecuta _gm_reservarMem() y comprueba retorno 
		ret = _gm_reservarMem(test_reservarMem[i].zocalo, test_reservarMem[i].numBytes, 0);

		// Comprueba retorno 
		if(ret != test_reservarMem[i].direccionRetorno) {
			_gg_escribir("error retorno\n", 0, 0, 0); 
			ok = 0;  
		} 
		
		// Comprueba vector 
		if(!assertVector(_gm_zocMem, vectorSolucion, NUMERO_FRANJAS)) {
			_gg_escribir("error assert\n", 0, 0, 0); 
			ok = 0;  
		}
		
		if(ok) {
			_gg_escribir("OK\n", 0, 0, 0); 
		} 
		
		leerTecla(4); 
	}
		
	return 0; 
} 

int testReady() {
	
	return 0; 
}

int testBlock() {

	return 0; 
}

int testPila() {

	return 0; 
}

//------------------------------------------------------------------------------
int main(int argc, char **argv) {
//------------------------------------------------------------------------------
	intFunc start;
	char *progs[8];
	unsigned char zocalosDisponibles[3];
	int num_progs, ind_prog, zocalo;
	int i, j;

	inicializarSistema();
	
	_gg_escribir("********************************", 0, 0, 0);
	_gg_escribir("*                              *", 0, 0, 0);
	_gg_escribir("* Sistema Operativo GARLIC 2.0 *", 0, 0, 0);
	_gg_escribir("*                              *", 0, 0, 0);
	_gg_escribir("********************************", 0, 0, 0);
	_gg_escribir("*** Inicio fase 2_M\n", 0, 0, 0);
	
	num_progs = _gm_listaProgs(progs);
	if (num_progs == 0)
		_gg_escribir("ERROR: |NO hay programas disponibles!\n", 0, 0, 0);
	else
	{	
			_gp_WaitForVBlank();
			gestionSincronismos();
			if ((_gd_pcbs[1].PID == 0) || (_gd_pcbs[2].PID == 0)
													|| (_gd_pcbs[3].PID == 0))
			{
				// Liberar mem tests 
				testLiberarMem(); 
				
				// Reservar mem tests 
				testReservarMem(); 
				
				// Posar estat ready diferents procesos  
				testReady(); 
				
				// Posar estat block diferents procesos 
				testBlock(); 
				
				// Pintar pila amb diferents nivells  
				testPila(); 
			}
	}
	return 0;
}
