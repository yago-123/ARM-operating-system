/*------------------------------------------------------------------------------

	"xf_3.c" : programa de test para cifrar texto mediante XOR;
				(versión 1.0)
	
	Imprime diversos mensajes por ventana, comprobando el funcionamiento de
	la inserción de valores con distintos formatos, con o sin traspaso del
	límite de la última columna de la ventana.

------------------------------------------------------------------------------*/

#include <GARLIC_API.h>
#define TEXT_SIZE 27

int len(char str[]){
    int i = 0; 
    while(str[i] != '\0') {
        i++; 
    }
    
    return i; 
}

void xifrat(char cipher_text[], char plain_text[], char key) {
	int i; 
    for(i = 0; i < len(plain_text); i++) {
        cipher_text[i] = plain_text[i] ^ key; 
    }
	
	cipher_text[len(plain_text)] = '\0'; 
}

void desxifrat(char uncipher_text[], char cipher_text[], char key) {
    xifrat(uncipher_text, cipher_text, key); 
}

int _start(int arg)
{   
	char cipher_text[TEXT_SIZE];
	char uncipher_text[TEXT_SIZE]; 

	char key = (char)5+(arg*10);  
	char plain_text[] = {"Lorem ipsum dolor sit amet"}; 
	
	GARLIC_printf("Text xifrat: \n"); 
	xifrat(cipher_text, plain_text, key); 
	GARLIC_printf("%s\n", cipher_text);
	
	GARLIC_printf("Text desxifrat: \n"); 
	desxifrat(uncipher_text, cipher_text, key); 
	GARLIC_printf("%s\n", uncipher_text);
	
    return 0;
}
