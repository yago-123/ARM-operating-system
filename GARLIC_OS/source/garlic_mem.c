/*------------------------------------------------------------------------------

	"garlic_mem.c" : fase 1 / programador M

	Funciones de carga de un fichero ejecutable en formato ELF, para GARLIC 1.0

------------------------------------------------------------------------------*/
#include <nds.h>
#include <filesystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>			// cabecera estructuras elf 
#include <garlic_system.h>	// definición de funciones y variables de sistema

#define INI_MEM 0x01002000		// dirección inicial de memoria para programas
#define LAST_MEM 0x01008000		// direccion final de memria para programas 

void mostraElfHeader(Elf32_Ehdr *elfHeader); 
void mostraProgramHeader(Elf32_Phdr *programHeader, int num); 
void mostraSectionHeader(Elf32_Shdr *sectionHeader, int num); 

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
	char *file_content, path_elf[19]; 
	int file_size, i, ret = 0; 
	
	Elf32_Ehdr *elfHeader; 
	Elf32_Phdr *programHeader; 
	
	// Buscar fitxer keyname.elf 
	sprintf(path_elf, "/Programas/%s.elf", keyName); 
	fp = fopen(path_elf, "rb"); 
	if(fp != NULL) {
		// Carregar fitxer dins de buffer dinamic
		fseek(fp, 0, SEEK_END); 
		file_size = ftell(fp); 
		fseek(fp, 0, SEEK_SET); 
		
		file_content = (char*) malloc(file_size+1); 
		fread(file_content, file_size, 1, fp); 
		
		// Accedim capçalera ELF per obtenir les dades 
		elfHeader = (Elf32_Ehdr*) file_content; 
		
		// Accedim a la taula de segments 
		for(i = 0; i < elfHeader->e_phnum; i++) {
			programHeader = (Elf32_Phdr*)(file_content + elfHeader->e_phoff + (i*sizeof(Elf32_Phdr))); 
			if(programHeader->p_type == PT_LOAD) {
				// Comprobem si tenim espai suficient a memoria 
				if((_gm_mem_lliure + programHeader->p_memsz) >= LAST_MEM) {
					_gm_mem_lliure = INI_MEM; 
				}
				
				// Carreguem el contingut de segments amb tipus PT_LOAD en memoria 
				_gs_copiaMem((void*)file_content + programHeader->p_offset, (void*)_gm_mem_lliure, programHeader->p_memsz);  
			}
		}
		
		// Accedim taula de seccions i efectuem reubicacions
		_gm_reubicar(file_content, programHeader->p_paddr, (void*)_gm_mem_lliure);
		// Posicio de inici programa en mem.  
		ret = elfHeader->e_entry - programHeader->p_paddr + _gm_mem_lliure; 
		
		// Actualitzem variable global, comprobem que sigui multiple de 4  
		if(programHeader->p_memsz%4 != 0) {
			_gm_mem_lliure += programHeader->p_memsz + (4 - programHeader->p_memsz%4);
		} else {
			_gm_mem_lliure += programHeader->p_memsz; 
		}
		
		free(file_content); 
		fclose(fp); 
	} else {
		printf("Error carregant programa\n"); 
	}
	
	return ((intFunc) ret);
}