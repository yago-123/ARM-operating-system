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

#include <garlic_system.h>	// definici�n de funciones y variables de sistema

#define INI_MEM 0x01002000		// direcci�n inicial de memoria para programas

void mostraElfHeader(Elf32_Ehdr *elfHeader); 

/* _gm_initFS: inicializa el sistema de ficheros, devolviendo un valor booleano
					para indiciar si dicha inicializaci�n ha tenido �xito; */
int _gm_initFS()
{
	return nitroFSInit(NULL);
}



/* _gm_cargarPrograma: busca un fichero de nombre "(keyName).elf" dentro del
					directorio "/Programas/" del sistema de ficheros, y
					carga los segmentos de programa a partir de una posici�n de
					memoria libre, efectuando la reubicaci�n de las referencias
					a los s�mbolos del programa, seg�n el desplazamiento del
					c�digo en la memoria destino;
	Par�metros:
		keyName ->	vector de 4 caracteres con el nombre en clave del programa
	Resultado:
		!= 0	->	direcci�n de inicio del programa (intFunc)
		== 0	->	no se ha podido cargar el programa
*/
intFunc _gm_cargarPrograma(char *keyName)
{
	FILE *fp; 
	char *file_content; 
	int file_size; 
	
	Elf32_Ehdr *elfHeader; 
	Elf32_Phdr *programHeader; 
	Elf32_Shdr *sectionHeader; 
	
	// Pas 1: Buscar fitxer keyname.elf 
	fp = fopen(strcat(keyName, ".elf"), "rb"); 
	if(fp != NULL) {
		// Pas 2: Carregar fitxer dins de buffer dinamic
		fseek(fp, 0, SEEK_END); 
		file_size = ftell(fp); 
		fseek(fp, 0, SEEK_SET); 
		
		file_content = (char*) malloc(file_size+1); 
		fread(file_content, file_size, 1, fp); 
		
		// Pas 3: Accedir cap�alera ELF per obtenir les dades 
		elfHeader = (Elf32_Ehdr*) file_content; 
		mostraElfHeader(elfHeader); 
		
		
		free(file_content); 
		fclose(fp); 
	} else {
		printf("Error carregant programa\n"); 
	}
	
	return ((intFunc) 0);
}


void mostraElfHeader(Elf32_Ehdr *elfHeader) {
	printf("*** Elf header info ***\n"); 
	printf("Entry point: %x\n", elfHeader->e_entry); 
	printf("Entry program header: %x\n", elfHeader->e_phoff); 
	printf("Entry section header: %x\n", elfHeader->e_shoff); 
}
