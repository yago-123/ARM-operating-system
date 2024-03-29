﻿@;==============================================================================
@;
@;	"garlic_itcm_mem.s":	código de rutinas de soporte a la carga de
@;							programas en memoria (version 1.0)
@;
@;==============================================================================

.include "../include/garlic_system.i"

NUM_FRANJAS = 768
INI_MEM_PROC = 0x01002000
MAX_ZOCALOS = 4

.section .dtcm,"wa",%progbits
	.align 2
	.global _gm_zocMem
_gm_zocMem: .space NUM_FRANJAS @; vector de ocupación de franjas mem.

quo: .space 4
mod: .space 4

.section .itcm,"ax",%progbits
	.arm
	.align 2

	.global _gm_reubicar
	@; rutina para interpretar los 'relocs' de un fichero ELF y ajustar las
	@; direcciones de memoria correspondientes a las referencias de tipo
	@; R_ARM_ABS32, restando la dirección de inicio de segmento y sumando
	@; la dirección de destino en la memoria;
	@;Parámetros:
	@; R0: dirección inicial del buffer de fichero (char *fileBuf)
	@; R1: dirección de inicio de segmento (unsigned int pAddr)
	@; R2: dirección de destino en la memoria (unsigned int *dest)
	@;Resultado:
	@; cambio de las direcciones de memoria que se tienen que ajustar
_gm_reubicar:
	push {r0-r12, lr}
	
		ldr r4, [r13, #56] 				@; r4 = (13 regs + lr)*4 
	
		ldr r12, [r0, #DESP_E_SHOFF]	@; r12 = e_shoff 
		add r12, r12, r0 				@; r12 = e_shoff + file_content
		
		ldrh r5, [r0, #DESP_E_SHNUM]	 @; r5 = e_shnum
		sub r5, r5, #1 					 @; r5 = e_shnum - 1, restamos 1 para dec. hasta 0 
		
		@; do while r5 >= 0 {
	.L_recorre_seccions: 
		ldrh r7, [r0, #DESP_E_SHENTSIZE] @; r7 = e_shentsize 
		mla r6, r5, r7, r12			@; r6 = file_content + e_shoff + (r5*sizeof(Elf32_Shdr))
		
		ldr r7, [r6, #DESP_SH_TYPE]	@; r7 = sectionHeader->sh_type 
		cmp r7, #TYPE_SHT_REL				
		bne .L_fi_recorre_seccio 
									@; if(sectionHeader->sh_type == SHT_REL) {
		
		ldr r7, [r6, #DESP_SH_SIZE]		@; r7 = sectionHeader->sh_size
		sub r7, #SIZE_REL				@; 	r7 = r7 - sizeof(Elf32_Rel), restamos 1 para dec. hasta 0  
		
		ldr r8, [r6, #DESP_SH_OFFSET]	@; 	r8 = sectionHeader->offset 
	.L_recorre_seccio_SHT_REL: 
										@; while (r7 >= 0) {
		add r9, r8, r7						@; r9 = sectionHeader->offset + (r7*sizeof(Elf32_Rel)
		add r9, r0							@; r9 = r9 + file_content 
		
		ldr r10, [r9, #DESP_R_INFO]			@; r10 = relocator->r_info 
		and r10, #0xFF						@; Filtramos bits bajos
		cmp r10, #TYPE_R_ARM				@; Mascara 00000010
		bne .L_fi_recorre_seccio_SHT_REL
											@; if(relocator_r_info & 2) {
	.L_reubica_ARM_ABS32: 
		ldr r10, [r9]							@; r10 = relocator->r_offset 
		sub r10, r1								@; r10 = relocator->r_offset - p_paddr 
		add r10, r2								@; r10 = relocator->r_offset - p_paddr + INI_MEM 
		
		ldr r11, [r10]							@; r11 = Mem[r10]
			
												@; if(r3 == 0 || Mem[r10] < r3) {
		cmp r3, #0								@; 		.L_reubica_segment_codi
		beq .L_reubica_segment_codi
		
		cmp r11, r3
		blo .L_reubica_segment_codi 			
												@; } else .L_reubica_segment_dades 
	.L_reubica_segment_dades:
		sub r11, r3 							@; r11 = r11 - p_paddr_dades
		add r11, r4 							@; r11 = r11 + INI_MEM
		b .L_guarda_reubicacio 
		
	.L_reubica_segment_codi: 
		sub r11, r1								@; r11 = r11 - p_paddr_codi
		add r11, r2								@; r11 = r11 + INI_MEM	
	
	.L_guarda_reubicacio:
		str r11, [r10]							@; Mem[r10] = r11 
	
	.L_fi_recorre_seccio_SHT_REL: 
		sub r7, #SIZE_REL		@; Restem mida realocador
		cmp r7, #0
		bge .L_recorre_seccio_SHT_REL 
		
	.L_fi_recorre_seccio: 
		sub r5, #1				@; r5--; 
		cmp r5, #0
		bge .L_recorre_seccions	@; r5 >= 0
	pop {r0-r12, pc}


	.global _gm_liberarMem
	
_gm_liberarMem: 
	push {r0-r12, lr} 
							@; R0: Numero zocalo 
		mov r1, #0 			@; R1: Indice inicial 
		mov r2, #0			@; R2: Longitud franjas 
		mov r3, #0 			@; R3: Tipo franja 
	
		ldr r4, =_gm_zocMem @; R4: _gm_zocMem[]
		mov r5, #0  		@; R5 = i
		@; for(i = 0; i < len(_gm_zocMem); i++) {
	.LforLiberarMem: 
		cmp r5, #NUM_FRANJAS 
		bge .LstopForLiberarMem
		
		ldrb r6, [r4, r5]
		cmp r6, r0 
		bne .LelseLiberarMem
		
		mov r7, #0				@; if(_gm_zocMem[i] == zocalo) { 
		strb r7, [r4, r5]		@; 		_gm_zocMem[i] = 0;
		add r2, #1 				@; 		longitud++; 
		
		cmp r2, #1 				@; 		if(longitud == 1) {
		bne .LcontinuaLiberarMem
		mov r1, r5				@; 			inici = i; 
								@; 		}
		b .LcontinuaLiberarMem
	.LelseLiberarMem: 			@; } else {
		cmp r2, #0 
		beq .LcontinuaLiberarMem
								@; 		if(longitud != 0) {
		mov r8, r0				@; 			// Salvem zocalo 
		mov r0, #0				@; 			// Posem zocalo a 0 per pintar 
		bl _gm_pintarFranjas	@; 			_gm_pintarFranjas(0, inici, longitud, 0);
		mov r0, r8
		mov r2, #0				@; 			longitud = 0; 
								@; 		}
	.LcontinuaLiberarMem: 		@; }
		add r5, #1
		b .LforLiberarMem
	.LstopForLiberarMem: 
		@; }
			
		cmp r2, #0 				@; if(longitud != 0) { 
		beq .LfinalLiberarMem
		mov r0, #0 
		bl _gm_pintarFranjas 	@; 		_gm_pintarFranjas(0, inici, longitud, 0);  
		
	.LfinalLiberarMem: 
					
	pop {r0-r12, pc}

	.global _gm_reservarMem
	@; rutina para reservar franjas de memoria consecutivas que 
	@; proporcionen espacio suficiente para un segmento 
	@;Parámetros:
	@; R0: numero de zocalo (int z)
	@; R1: tamaño en bytes del segmento a cargar (int tam)
	@; R2: tipo de segmento a cargar (unsigned char tipo_seg)
	@;Resultado:
	@; Devuelve direccion del espacio en caso de tener segmentos
	@; consecutivos, o 0 en caso contrario 
_gm_reservarMem: 
	push {r1-r7, lr}
		mov r6, r0		@; Guardem r0 i r1  
		mov r7, r2 
		
		mov r0, r1 		@; Cridem funcio divmod 
		mov r1, #32 
		ldr r2, =quo 
		ldr r3, =mod 
		bl _ga_divmod
		
		ldr r0, [r2]   @; r0 = Num franja necesitada
		ldr r2, [r3]   @; r2 = Obtenim residu 
		cmp r2, #0 
		beq .Lresidu0  @; if(residu != 0) Num franja necesitada++; 
					   @; Afegeix franja extra necesaria 
		add r0, #1 
		
	.Lresidu0: 		   @; }
		mov r1, #0 	   @; r1 = Num franja disponibles 
		mov r2, #0	   @; r2 = contador 
		
		
		ldr r3, =_gm_zocMem  @; r3 = direccio base vector _gm_zocMem 
		
		@; while ((contador < NUM_FRANGES) && (franjDisponibles < franjNecesitats))
	.Lwhile: 
		cmp r2, #NUM_FRANJAS
		bge .LfiWhile 
 
		cmp r1, r0
		bge .LfiWhile 
		
		ldrb r4, [r3, r2]
		cmp r4, #0 			@; if(_gm_zocMem[i] == 0) {
		bne .LelseNoDisponible 
		
		cmp r1, #0 				@; if(franjDisponibles == 0) {
		bne .LcontinuaAntigaDir 
		
		mov r5, r2					@; dir. inicial = contador
								@; }
	.LcontinuaAntigaDir: 
		add r1, #1				@; franjDisponibles++
		b .LcontinuaFranja
		
	.LelseNoDisponible:		@; } else franjDisponibles = 0;  
		mov r1, #0				
						
	.LcontinuaFranja:		
		add r2, #1			@; contador++	
		b .Lwhile
		@; }
	.LfiWhile:
	
		@; if(franjDisponibles == franjNecesitats) {
		cmp r0, r1 
		bne .LelseSenseEspai
		
		mov r2, r5				@; contador = dir. inicial 
		add r4, r2, r0 			@; r4 = franjNecesitats + dir.inicial 
								@; for(i = dir. inicial; i < (franjNecesitats + dir.inicial); i++) {			
	.LforReservaEspais:		
		cmp r2, r4 			
		bge .LfiForReservaEspais 
		strb r6, [r3, r2]		@; 		_gm_zocMem[contador] = zocalo; 
		add r2, #1 
		b .LforReservaEspais 
 
	.LfiForReservaEspais:		@; }
	
		@; Pintamos franjas reservadas 
		mov r2, r0 				@; R2: longitud_franjas
		mov r0, r6 				@; R0: numero_zocalo 
		mov r1, r5 				@; R1: indice_inicial  
		mov r3, r7 				@; R3: tipo_franja 
		bl _gm_pintarFranjas
		
		mov r1, #32
		ldr r6, =INI_MEM_PROC
		mla r0, r5, r1, r6   
								@; return _gm_zocMem[dir. inicial] 
		b .Lretorna
	.LelseSenseEspai:	
		mov r0, #0 			@; else return 0; 
		
	.Lretorna:
 
	pop {r1-r7, pc}
	
	
	.global _gm_pintarFranjas
	@; rutina para reservar franjas de memoria reservadas
	@;Parámetros:
	@; R0: numero de zocalo (unsigned char zocalo)
	@; R1: indice incial franja (unsigned short index_ini)
	@; R2: numero de franjas a pintar (unsigned short num_franjas)
	@; R3: tipo de segmento: 0 -> codigo, 1-> datos (unsigned char tipo_seg)
	@;Resultado:
	@; Devuelve direccion del espacio en caso de tener segmentos
	@; consecutivos, o 0 en caso contrario 
_gm_pintarFranjas: 
	push {r0-r12, lr} 
		ldr r5, =_gs_colZoc		@; Obtenim color 
		add r5, r0
		ldrb r5, [r5] 

		mov r11, r2 			@; Salvem registres a escriure  
		mov r12, r3 

		@; Direccio = dir_base + (index_ini/8)*64 + index_ini % 8 
		mov r0, r1 				@; Cridem funcio divmod
		mov r1, #8 
		ldr r2, =quo 
		ldr r3, =mod 
		bl _ga_divmod
								
		ldr r2, [r2]   			@; Obtenim divisor
		ldr r3, [r3]   			@; Obtenim residu 
		mov r4, #64				@; Mida franja 
		
		mla r0, r2, r4, r3 		@; (index_ini/8)*64 + index_ini % 8 

								@; Obtenim direccio inicial (dir_base)
		mov r4, #0x06200000		@; Base mem. video  
		add r4, #0x4000			@; Base contingut baldoses 
		add r4, #0x8000			@; Base baldoses gestio mem, offset 512*64 
		
		add r0, r4 				@; Direccio completa 

		mov r4, r3 				@; Guardem contador_linia dins baldosa(residu) 
		
		mov r2, r11				@; Restaura registres 
		mov r3, r12 
		
	.LwhilePintaFranges: 	@; while(num_franjas > 0) {
		cmp r2, #0
		ble .LfiWhilePintaFranges 
			
		@; Crear color 
		ldrh r6, [r0, #16]
							@; 		if(direccio & 1) { 
		and r7, r0, #1 
		cmp r7, #1 
		bne .LcolorEsquerra
 
		and r6, #0x00ff  
		lsl r7, r5, #8
		orr r1, r6, r7		@; 			r1 = Color escriure (mante bits baixos, esquerra)
		
		b .LpintaColorCreat
	.LcolorEsquerra:		@; 		} else {
		and r6, #0xff00
		orr r1, r5, r6		@; 			r1 = Color escriure (mante bits alts, dreta)		
							@; 		}
	.LpintaColorCreat: 
	
		cmp r3, #0 			@; 		if(tipoSeg == 0) { // codi 
		bne .LpintaPatroEscacs
		
		bl _gm_liniaSolida	@; 			Pinta color solid 
		b .LcontinuaFiPinta
		
	.LpintaPatroEscacs:		@; 		} else {
		bl _gm_liniaAjedrez	@; 			Pinta patro 
							@; 		}
	.LcontinuaFiPinta:
	
		
		add r4, #1  		@; 	contador_linia dins baldosa++; 
		add r0, #1 			@; 	direccio pintar++;
			
		cmp r4, #8 			@; 	if(contador_linia >= 8 {
		blt .LcontinuaSenseAumentarBaldosa
		mov r4, #0 			@; 		contador_linia = 0; 
		add r0, #56			@; 		direccio += 56   (64 - 8 seguent baldosa)
							@; 	}
	.LcontinuaSenseAumentarBaldosa: 
		sub r2, #1 			@; 	num_franjas--;
		b .LwhilePintaFranges
		
	.LfiWhilePintaFranges:	@; }
			
	pop {r0-r12, pc}
	
	
	.global _gm_liniaSolida
	@; pinta las lineas verticales de las baldosas con todos los pixelex
	@;Parámetros:
	@; R0: dirección a pintar (unsigned int direccion_pintar)
	@; R1: color (teniendo en cuenta que son dos) (unsigned short color)
_gm_liniaSolida: 
	push {r0-r12, lr}
		strh r1, [r0, #16]
		strh r1, [r0, #24] 
		strh r1, [r0, #32]
		strh r1, [r0, #40]
	pop {r0-r12, pc} 
	
	.global _gm_liniaAjedrez
	@; pinta las lineas verticales en patron ajedrezado
	@;Parámetros:
	@; R0: dirección a pintar (unsigned int direccion_pintar)
	@; R1: color (teniendo en cuenta que son dos) (unsigned short color) 
_gm_liniaAjedrez: 
	push {r0-r12, lr}
		and r2, r0, #1
		cmp r2, #1 			@; if(direccio & 1) {
		bne .LpatroInvers
	
		strh r1, [r0, #16]
		strh r1, [r0, #32] 
		b .LfiPatro 
	.LpatroInvers:			@; } else {
		strh r1, [r0, #24]
		strh r1, [r0, #40]
							@; }
	.LfiPatro:
	
	pop {r0-r12, pc}

	.global _gm_rsiTIMER1 
_gm_rsiTIMER1: 
	push {r0-r12, lr} 
		@; f(y) = 0x06200000 + COL_ESPECIFICA + 256 + y*64
		@; 	*COL_ESPECIFICA_LETRAS = 52 
		@; 	*COL_ESPECIFICA_PILAS = 46
		
		mov r0, #0x06200000	@; Inicio mapa 
		add r0, #256 		@; Fila 0 
	
		add r0, #52		@; Columna especifica letras    

		@; Muestra zocalo en run
		ldr r1, =_gd_pidz 
		ldr r1, [r1] 
		
		and r1, #0xF	@; Filtramos zocalo 
		bl _gm_pintarPila
		
		mov r3, #64
		mul r2, r1, r3 	@; Calculamos posicion 

		mov r3, #178 	@; Guarda R azul 
		strh r3, [r0, r2]
		
		@; Muestra zocalo en ready  
		ldr r2, =_gd_nReady 
		ldrb r2, [r2]
		
		ldr r3, =_gd_qReady 
		mov r1, #0 		@; Contador 

	.LbucleRDY: 
		cmp r2, r1 
		beq .LfiBucleRDY
		
		ldrb r5, [r3, r1]
		
		mov r6, #64 
		mul r5, r6, r5		@; Calculamos posicion zocalo*64 

		mov r6, #57			@; Tabla ASCII
		strh r6, [r0, r5]	@; Guardamos Y en fila zocalo 
		bl _gm_pintarPila
		
		add r1, #1
		b .LbucleRDY 
 
	.LfiBucleRDY: 
		
		@; Muestra zocalo en block   
		ldr r2, =_gd_nDelay 
		ldrb r2, [r2]
		
		ldr r3, =_gd_qDelay 
		mov r1, #0 		@; Contador 

	.LbucleBLK: 
		cmp r2, r1 
		beq .LfiBucleBLK
		
		mov r5, #4 
		mul r5, r1, r5	
		ldr r5, [r3, r5]	@; _gd_qDelay[zocalo]
								
		and r5, #0xFF000000	@; Filtramos  8 bits altos (zocalo)
		lsr r5, #24	
		
		mov r6, #64 
		mul r5, r6, r5		@; Calculamos posicion zocalo*64 

		mov r6, #34			@; Valor ASCII baldosa 
		strh r6, [r0, r5]	@; Guardamos B en fila zocalo 
		
		add r1, #1
		b .LbucleBLK 
	.LfiBucleBLK: 
	
	pop {r0-r12, pc} 
	
	.global _gm_pintarPila
	@; pinta la pila del zocalo concret (R1)
	@;Parámetros:
	@; R0: dirección a pintar, cal restar per ajustar columna 
	@; R1: zocalo a analitzar i pintar 
_gm_pintarPila: 
	push {r0-r12, lr} 
		sub r0, #6			@; Restem per utilitzar columna 46 
 
		ldr r2, =_gd_pcbs
		ldr r3, =_gd_stacks
	
		cmp r1, #0 			@; Zocalo
		bne .LPilaProgUsuari
	.LpilaSistema: 			@; if(zocalo == zocaloSistema) {
		mov r5, sp			@; 		Top pila programa sistema 
		ldr r7, =#0x0B003D00 @; 	Inici pila sistema (dtcm) 

		b .LcalculaPila 
	.LPilaProgUsuari:		@; }else{
		mov r5, #24 
		mla r5, r1, r5, r2 	@; 		r5 = _gd_pcbs[zocalo] 
		ldr r5, [r5, #8]	@; 		r5 = _gd_pcbs[zocalo].SP 
		
		mov r6, r1 
		sub r6, #1			@; 		r6 = zocalo-1  
		
		mov r7, #512 		@; 		512 bytes por pila 
		mla r7, r6, r7, r3 	@; 		r7 = _gd_stack[zocalo-1] = 512*zocalo-1 + _gd_stacks 
							@; }
	.LcalculaPila:
		sub r7, r5, r7		@; bytesPila = _gd_pcbs[zocalo].SP - _gd_stack[zocalo-1] 
	
		mov r5, #119 		@; Baldosa 1 
		mov r8, #119 		@; Baldosa 2
	
		cmp r7, #256 
		bgt .LelseCalculaBaldosa 
							@; if (bytesPila <= 256) {
		mov r7, r7, lsr #5
		add r5, r7			@; 		Baldosa 1 += bytesPila/32; 
		b .LpintaBaldosa
	.LelseCalculaBaldosa: 	@; } else {
		mov r5, #127		@; 		Baldosa 1 = 127	(maxim)
		mov r6, #256 
		sub r7, r6 			
		
		mov r7, r7, lsr #5
		add r8, r7 			@; 		Baldosa 2 += (bytesPila-256)/32; 
							@; }
	.LpintaBaldosa: 
		mov r7, #64 
		mla r7, r1, r7, r0 	@; Calculem posicio escriure 
		
		strh r5, [r7]		@; Guardem baldoses 
		strh r8, [r7, #2] 
 
	pop {r0-r12, pc} 

.end 