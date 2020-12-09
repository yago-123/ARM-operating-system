/*------------------------------------------------------------------------------

	"garlic_mem.c" : fase 2 / programador M

	Funciones de carga de un fichero ejecutable en formato ELF, para GARLIC 2.0

------------------------------------------------------------------------------*/
#include <nds.h>
#include <filesystem.h>
#include <dirent.h>			// para struct dirent, etc.
#include <stdio.h>			// para fopen(), fread(), etc.
#include <stdlib.h>			// para malloc(), etc.
#include <string.h>			// para strcat(), memcpy(), etc.

#include <elf.h>
#include <garlic_system.h>	// definición de funciones y variables de sistema


#define INI_MEM 0x01002000		// dirección inicial de memoria para programas
#define LAST_MEM 0x01008000		// direccion final de memria para programas 

typedef struct SegmentInfo{
	Elf32_Addr	p_paddr;	    /* Segment physical address */ 
	unsigned int posicioMem; 
} Segment;

/* _gm_initFS: inicializa el sistema de ficheros, devolviendo un valor booleano
					para indiciar si dicha inicialización ha tenido éxito; */
int _gm_initFS()
{
	return nitroFSInit(NULL);	// inicializar sistema de ficheros NITRO
}


/* _gm_listaProgs: devuelve una lista con los nombres en clave de todos
			los programas que se encuentran en el directorio "Programas".
			 Se considera que un fichero es un programa si su nombre tiene
			8 caracteres y termina con ".elf"; se devuelven sólo los
			4 primeros caracteres de los programas (nombre en clave).
			 El resultado es un vector de strings (paso por referencia) y
			el número de programas detectados */
// opendir(): https://man7.org/linux/man-pages/man3/opendir.3.html
// readdir(): https://www.man7.org/linux/man-pages/man3/readdir.3.html

int _gm_listaProgs(char *progs[]) {	
	int i, numProgs = 0;

    struct dirent *entrada; 	
	// Obrim stream directori 
	DIR *punterDirectori = opendir("/Programas"); 
	if(punterDirectori != NULL) {

		// Recorregut de les entrades 
		entrada = readdir(punterDirectori); 
		while(entrada != NULL) {
			// Filtra longitud de programa i tipus  
			if((strlen(entrada->d_name) == 8) && (entrada->d_type == DT_REG)) {
				// Guarda nom executable  
				progs[numProgs] = malloc(5); 
				for(i = 0; i < 4; i++) {
					progs[numProgs][i] = entrada->d_name[i];
				}

				progs[numProgs][4] = '\0'; 
				numProgs++; 
			}
		    
			entrada = readdir(punterDirectori); 	
		}

		// Tanca stream 
		closedir(punterDirectori); 
	} else {
		printf("Error obrint directori /Programas\n"); 
	}

	return numProgs; 
}


/* _gm_cargarPrograma: busca un fichero de nombre "(keyName).elf" dentro del
				directorio "/Programas/" del sistema de ficheros, y carga los
				segmentos de programa a partir de una posición de memoria libre,
				efectuando la reubicación de las referencias a los símbolos del
				programa, según el desplazamiento del código y los datos en la
				memoria destino;
	Parámetros:
		zocalo	->	índice del zócalo que indexará el proceso del programa
		keyName ->	vector de 4 caracteres con el nombre en clave del programa
	Resultado:
		!= 0	->	dirección de inicio del programa (intFunc)
		== 0	->	no se ha podido cargar el programa
*/
intFunc _gm_cargarPrograma(int zocalo, char *keyName)
{
	FILE *fp; 
	int file_size, i, ret = 0; 
	char *file_content, path_elf[19]; 
	
	Elf32_Ehdr *elfHeader; 
	Elf32_Phdr *programHeader; 
	Segment codeSegment = {0, 0}, dataSegment = {0, 0}; 
	
	// Buscar fitxer keyname.elf 
	sprintf(path_elf, "/Programas/%s.elf", keyName); 
	fp = fopen(path_elf, "rb"); 
	if(fp != NULL) {
		// Carregar fitxer dins de buffer dinamic
		fseek(fp, 0, SEEK_END); 
		file_size = ftell(fp); 
		fseek(fp, 0, SEEK_SET); 
		
		file_content = (char*) malloc(file_size); 
		fread(file_content, file_size, 1, fp); 
		
		// Accedim capçalera ELF per obtenir les dades 
		elfHeader = (Elf32_Ehdr*) file_content; 
		
		// Accedim a la taula de segments 
		for(i = 0; i < elfHeader->e_phnum; i++) {
			programHeader = (Elf32_Phdr*)(file_content + elfHeader->e_phoff + (i*sizeof(Elf32_Phdr))); 
			if(programHeader->p_type == PT_LOAD) {
				//Identifiquem el tipus de segment  
				if(programHeader->p_flags == (PF_R | PF_X)) {
					// Segment lectura i execucio 
					codeSegment.p_paddr = programHeader->p_paddr; 
					codeSegment.posicioMem = (unsigned int) _gm_reservarMem(zocalo, programHeader->p_memsz, 0);
					
					// Copiem segment a memoria 
					if(codeSegment.posicioMem != 0) {
						_gs_copiaMem((void*)file_content + programHeader->p_offset, (void*)codeSegment.posicioMem, programHeader->p_filesz);  
					} else { 
						// No hi ha reserva, nomes retornem error 
						return 0; 
					}
				} else if(programHeader->p_flags == (PF_R | PF_W)) {
					// Segment lectura i escritura 
					dataSegment.p_paddr = programHeader->p_paddr; 
					dataSegment.posicioMem = (unsigned int) _gm_reservarMem(zocalo, programHeader->p_memsz, 1);

					// Copiem segment a memoria 
					if(dataSegment.posicioMem != 0) {
						_gs_copiaMem((void*)file_content + programHeader->p_offset, (void*)dataSegment.posicioMem, programHeader->p_filesz); 
					} else {
						// Hi ha reserva anterior, alliberem segment de codi i retornem 
						_gm_liberarMem(zocalo); 
						return 0; 
					}
				} 
			}
		}
		
		// Reubiquem les posicions de memoria de tots els segments 
		_gm_reubicar(file_content, codeSegment.p_paddr, (unsigned int*)codeSegment.posicioMem, 
							dataSegment.p_paddr, (unsigned int*)dataSegment.posicioMem); 
	
		// Posicio de inici programa en mem.  
		ret = elfHeader->e_entry - codeSegment.p_paddr + codeSegment.posicioMem; 
		
		free(file_content); 
		fclose(fp); 
	} else {
		printf("Error carregant programa\n"); 
	}
	
	return ((intFunc) ret);
}