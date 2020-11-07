	.arch armv5te
	.eabi_attribute 23, 1
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 1
	.eabi_attribute 30, 6
	.eabi_attribute 34, 0
	.eabi_attribute 18, 4
	.file	"xf_3.c"
	.text
	.align	2
	.global	len
	.syntax unified
	.arm
	.fpu softvfp
	.type	len, %function
len:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	sub	sp, sp, #16
	str	r0, [sp, #4]
	mov	r3, #0
	str	r3, [sp, #12]
	b	.L2
.L3:
	ldr	r3, [sp, #12]
	add	r3, r3, #1
	str	r3, [sp, #12]
.L2:
	ldr	r3, [sp, #12]
	ldr	r2, [sp, #4]
	add	r3, r2, r3
	ldrb	r3, [r3]	@ zero_extendqisi2
	cmp	r3, #0
	bne	.L3
	ldr	r3, [sp, #12]
	mov	r0, r3
	add	sp, sp, #16
	@ sp needed
	bx	lr
	.size	len, .-len
	.align	2
	.global	xifrat
	.syntax unified
	.arm
	.fpu softvfp
	.type	xifrat, %function
xifrat:
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	sub	sp, sp, #28
	str	r0, [sp, #12]
	str	r1, [sp, #8]
	mov	r3, r2
	strb	r3, [sp, #7]
	mov	r3, #0
	str	r3, [sp, #20]
	b	.L6
.L7:
	ldr	r3, [sp, #20]
	ldr	r2, [sp, #12]
	add	r3, r2, r3
	ldr	r2, [sp, #20]
	ldr	r1, [sp, #8]
	add	r2, r1, r2
	ldrb	r1, [r2]	@ zero_extendqisi2
	ldrb	r2, [sp, #7]
	eor	r2, r2, r1
	and	r2, r2, #255
	strb	r2, [r3]
	ldr	r3, [sp, #20]
	add	r3, r3, #1
	str	r3, [sp, #20]
.L6:
	ldr	r0, [sp, #8]
	bl	len
	mov	r2, r0
	ldr	r3, [sp, #20]
	cmp	r2, r3
	bgt	.L7
	ldr	r0, [sp, #8]
	bl	len
	mov	r3, r0
	mov	r2, r3
	ldr	r3, [sp, #12]
	add	r3, r3, r2
	mov	r2, #0
	strb	r2, [r3]
	nop
	add	sp, sp, #28
	@ sp needed
	ldr	pc, [sp], #4
	.size	xifrat, .-xifrat
	.align	2
	.global	desxifrat
	.syntax unified
	.arm
	.fpu softvfp
	.type	desxifrat, %function
desxifrat:
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	sub	sp, sp, #20
	str	r0, [sp, #12]
	str	r1, [sp, #8]
	mov	r3, r2
	strb	r3, [sp, #7]
	ldrb	r3, [sp, #7]	@ zero_extendqisi2
	mov	r2, r3
	ldr	r1, [sp, #8]
	ldr	r0, [sp, #12]
	bl	xifrat
	nop
	add	sp, sp, #20
	@ sp needed
	ldr	pc, [sp], #4
	.size	desxifrat, .-desxifrat
	.section	.rodata
	.align	2
.LC1:
	.ascii	"Text xifrat: \012\000"
	.align	2
.LC2:
	.ascii	"%s\012\000"
	.align	2
.LC3:
	.ascii	"Text desxifrat: \012\000"
	.align	2
.LC0:
	.ascii	"Lorem ipsum dolor sit amet\000"
	.text
	.align	2
	.global	_start
	.syntax unified
	.arm
	.fpu softvfp
	.type	_start, %function
_start:
	@ args = 0, pretend = 0, frame = 96
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	sub	sp, sp, #100
	str	r0, [sp, #4]
	ldr	r3, [sp, #4]
	and	r3, r3, #255
	mov	r2, r3
	lsl	r2, r2, #2
	add	r3, r2, r3
	lsl	r3, r3, #1
	and	r3, r3, #255
	add	r3, r3, #5
	strb	r3, [sp, #95]
	ldr	r3, .L11
	add	ip, sp, #12
	mov	lr, r3
	ldmia	lr!, {r0, r1, r2, r3}
	stmia	ip!, {r0, r1, r2, r3}
	ldm	lr, {r0, r1, r2}
	stmia	ip!, {r0, r1}
	strh	r2, [ip]	@ movhi
	add	ip, ip, #2
	lsr	r3, r2, #16
	strb	r3, [ip]
	ldr	r0, .L11+4
	bl	GARLIC_printf
	ldrb	r2, [sp, #95]	@ zero_extendqisi2
	add	r1, sp, #12
	add	r3, sp, #68
	mov	r0, r3
	bl	xifrat
	add	r3, sp, #68
	mov	r1, r3
	ldr	r0, .L11+8
	bl	GARLIC_printf
	ldr	r0, .L11+12
	bl	GARLIC_printf
	ldrb	r2, [sp, #95]	@ zero_extendqisi2
	add	r1, sp, #68
	add	r3, sp, #40
	mov	r0, r3
	bl	desxifrat
	add	r3, sp, #40
	mov	r1, r3
	ldr	r0, .L11+8
	bl	GARLIC_printf
	mov	r3, #0
	mov	r0, r3
	add	sp, sp, #100
	@ sp needed
	ldr	pc, [sp], #4
.L12:
	.align	2
.L11:
	.word	.LC0
	.word	.LC1
	.word	.LC2
	.word	.LC3
	.size	_start, .-_start
	.ident	"GCC: (devkitARM release 46) 6.3.0"
