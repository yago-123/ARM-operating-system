/*------------------------------------------------------------------------------

	"garlic_mem.c" : fase 1 / programador M

	Funciones de carga de un fichero ejecutable en formato ELF, para GARLIC 1.0

------------------------------------------------------------------------------*/
#include <nds.h>
#include <filesystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include"elf.h"

#include <garlic_system.h>	// definición de funciones y variables de sistema

#define INI_MEM 0x01002000		// dirección inicial de memoria para programas



/* _gm_initFS: inicializa el sistema de ficheros, devolviendo un valor booleano
					para indiciar si dicha inicialización ha tenido éxito; */
int _gm_initFS()
{
	return nitroFSInit(NULL);
}



/* _gm_cargarPrograma: busca un fichero de nombre "(keyName).elf" dentro del
					directorio "/Programas/" del sistema de ficheros, y
					carga los segmentos de programa a partir de una posición de
					memoria libre, efectuando la reubicación de las referencias
					a los símbolos del programa, según el desplazamiento del
					código en la memoria destino;
	Parámetros:
		keyName ->	vector de 4 caracteres con el nombre en clave del programa
	Resultado:
		!= 0	->	dirección de inicio del programa (intFunc)
		== 0	->	no se ha podido cargar el programa
*/
intFunc _gm_cargarPrograma(char *keyName)
{
	FILE *fp; 
	char *file_content; 
	int file_size; 
	
	// Pas 1: Buscar fitxer keyname.elf 
	fp = fopen(strcat(keyName, ".elf"), "rb"); 
	if(fp != NULL) {
		// Pas 2: Carregar fitxer dins de buffer dinamic
		fseek(fp, 0, SEEK_END); 
		file_size = ftell(fp); 
		fseek(fp, 0, SEEK_SET); 
		
		file_content = (char*) malloc(file_size+1); 
		fread(file_content, file_size, 1, fp); 
		
		
		
		free(file_content); 
		fclose(fp); 
	} else {
		printf("Error carregant programa\n"); 
	}
	
	

	return ((intFunc) 0);
}

