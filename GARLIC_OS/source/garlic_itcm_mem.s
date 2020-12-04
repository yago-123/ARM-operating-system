@;==============================================================================
@;
@;	"garlic_itcm_mem.s":	código de rutinas de soporte a la carga de
@;							programas en memoria (version 1.0)
@;
@;==============================================================================

.include "../include/garlic_system.i"

NUM_FRANJAS = 768
INI_MEM_PROC = 0x01002000

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
	push {r1-r12, lr} 
		ldr r1, =_gm_zocMem
		mov r2, #0
		@; for(i = 0; i < len(_gm_zocMem); i++) {
	.LforLiberarMem: 
		cmp r2, #NUM_FRANJAS 
		bge .LstopForLiberar 
		
		ldrb r3, [r1, r2] 
		cmp r3, r0 				@; if (_gm_zocMem[i] == z) {
		bne .LfiIfLiberarMem 
		
		mov r4, #0
		strb r4, [r1, r2] 		@; 		_gm_zocMem[i] = 0; 
		
	.LfiIfLibrearMem: 			@; }
		add r2, #1
	.LstopForLiberarMem: 
		@; }
	
	pop {r1-r12, pc}



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
		strb r6, [r3, r2]		@; 		_gm_zocMem[contador] = z; 
		add r2, #1 
		b .LforReservaEspais 
 
	.LfiForReservaEspais:		@; }
		mov r1, #32
		ldr r6, =INI_MEM_PROC
		mla r0, r5, r1, r6   
								@; return _gm_zocMem[dir. inicial] 
		b .Lretorna
	.LelseSenseEspai:	
		mov r0, #0 			@; else return 0; 
		
	.Lretorna:
 
	pop {r1-r7, pc}
	
	.global _gm_rsiTIMER1 
_gm_rsiTIMER1: 
	push {r0-r12, lr} 
	
	pop {r0-r12, pc}
	
.end


