@;==============================================================================
@;
@;	"garlic_itcm_mem.s":	código de rutinas de soporte a la carga de
@;							programas en memoria (version 1.0)
@;
@;==============================================================================

.include "../include/garlic_system.i"

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
	
	pop {r0-r12, pc}

	.global _gm_rsiTIMER1 
_gm_rsiTIMER1: 
	push {r0-r12, lr} 
	
	pop {r0-r12, pc}
	
.end


