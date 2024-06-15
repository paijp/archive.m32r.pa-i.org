/* for small-model */
.global	__startup
.global	ei_hdr_asm
.global	trap1_hdr_asm
.global	itron_syscall
.section .startup, "ax"
/*.text*/
.align	2
__startup:
	ld24	r0, 0
	mvtc	r0, psw
	ld24	r15, 0xf10000		; int / system-call
	
	ld24	r0, 0x80
	mvtc	r0, psw
	ld24	r15, 0xf0e400		; idle task
	
	ld24	r0, 0
	ld24	r1, __bss_start
	ld24	r2, __bss_end
loop_bss:
	cmpu	r1, r2
	bnc	exit_bss
	st	r0, @-r2	; *(--R2) = 0;
	bra	loop_bss
exit_bss:
	
	bra	main

ei_hdr_asm:
	and3	r15, r15, 0xffff
	bnez	r15, ei_fromint
	
ei_fromtsk:
	mvfc	r15, spu	;spi=0x00f10000
	
	st	r0, @-r15	;spu+0x48
	st	r1, @-r15
	st	r2, @-r15	;spu+0x40
	st	r3, @-r15
	st	r4, @-r15
	st	r5, @-r15
	st	r6, @-r15	;spu+0x30
	st	r7, @-r15
	st	r8, @-r15
	st	r9, @-r15
	st	r10, @-r15	;spu+0x20
	st	r11, @-r15
	st	r12, @-r15
	st	r13, @-r15
	st	r14, @-r15	;spu+0x10
	
	mvfachi	r0
	st	r0, @-r15	;spu+0xc
	mvfaclo	r0
	st	r0, @-r15	;spu+8
	
	ld24	r0, 0xeff004	;ICUISTS
	ld	r14, @r0
	
	mvfc	r0, bpc
	st	r0, @-r15	;spu+4
	
	mvfc	r0, psw
	or	r0, r14
	or3	r1, r0, 0x80	;SM=1
	st	r1, @-r15	;spu+0
	
	mvtc	r15, spu
	ld24	r15, 0x00f10000	;spi
	
	st	r1, @-r15	;(spi)
	
	bra	ei_common
	
ei_fromint:
	slli	r15, 12
	not	r15, r15
	srli	r15, 4
	not	r15, r15
	srli	r15, 8		;r15:0000 0000 1111 0000 ---- ---- ---- ----
	
	st	r0, @-r15	;spu+0x48
	st	r1, @-r15
	st	r2, @-r15	;spu+0x40
	st	r3, @-r15
	st	r4, @-r15
	st	r5, @-r15
	st	r6, @-r15	;spu+0x30
	st	r7, @-r15
	st	r8, @-r15
	st	r9, @-r15
	st	r10, @-r15	;spu+0x20
	st	r11, @-r15
	st	r12, @-r15
	st	r13, @-r15
	st	r14, @-r15	;spu+0x10
	
	mvfachi	r0
	st	r0, @-r15	;spu+0xc
	mvfaclo	r0
	st	r0, @-r15	;spu+8
	
	ld24	r0, 0xeff004	;ICUISTS
	ld	r14, @r0
	
	mvfc	r0, bpc
	st	r0, @-r15	;spu+4
	
	mvfc	r0, psw
	or	r0, r14
	
	st	r0, @-r15	;spu+0
	st	r0, @-r15	;(spi)
ei_common:
	
	or3	r0, r0, 0x0040	;EI
	mvtc	r0, psw
	
	srli	r14, 0x14
	ld24	r0, 0x00f00000
	or	r14, r0
	ld	r14, @r14
	jl	r14
	
	ld	r7, @r15	;(spi)
	ld24	r0, 0xeff01c	;ICUIMASK
	st	r7, @r0
	
	ldi16	r8, 0x80
	and	r8, r7		;SM
	beqz	r8, trap1_fromint
	
	bl	reschedule
	bra	trap1_fromint

trap1_hdr_asm:
	st	r0, @-r15	;spu+0x48
	st	r1, @-r15
	st	r2, @-r15	;spu+0x40
	st	r3, @-r15
	st	r4, @-r15
	st	r5, @-r15
	st	r6, @-r15	;spu+0x30
	st	r7, @-r15
	st	r8, @-r15
	st	r9, @-r15
	st	r10, @-r15	;spu+0x20
	st	r11, @-r15
	st	r12, @-r15
	st	r13, @-r15
	st	r14, @-r15	;spu+0x10
	
	mvfachi	r7
	st	r7, @-r15	;spu+0xc
	mvfaclo	r7
	st	r7, @-r15	;spu+8
	
	mvfc	r7, bpc
	st	r7, @-r15	;spu+4
	
	mvfc	r7, psw
	st	r7, @-r15	;spu+0
	
	mv	r9, r7
	ldi16	r8, ~0x80
	and	r7, r8		;SM=0
	mvtc	r7, psw
	
	st	r9, @-r15	;(spi)
	
;;;;	or3	r7, r7, 0x0040	;EI=1
;;;;	mvtc	r7, psw
	
	ld24	r14, itron_syscall_hdr
	not	r7, r0
	and3	r7, r7, 0x7f
	slli	r7, 2
	add	r14, r7
	ld	r14, @r14	;r14 = itron_syscall_hdr[-r0];
	mv	r0, r1
	mv	r1, r2
	mv	r2, r3
	jl	r14
	
	ld	r7, @r15	;(spi)
	ld24	r8, 0x8000
	and	r8, r7		;BSM
	beqz	r8, trap1_fromint
	ldi16	r8, 0x4000
	and	r8, r7		;BEI
	beqz	r8, trap1_fromint
	
	mvfc	r7, spu
	addi	r7, 0x48
	st	r0, @r7		;r0 to stack
	bl	reschedule
trap1_fromint:
	ld	r14, @r15+	;(spi)
	mvtc	r14, psw
	
	ld	r14, @r15+	;spu+0
	mvtc	r14, psw
	
	ld	r7, @r15+	;spu+4
	mvtc	r7, bpc
	
	ld	r7, @r15+	;spu+8
	mvtaclo	r7
	ld	r7, @r15+
	mvtachi	r7
	
	ld	r14, @r15+	;spu+0x10
	ld	r13, @r15+
	ld	r12, @r15+
	ld	r11, @r15+
	ld	r10, @r15+	;spu+0x20
	ld	r9, @r15+
	ld	r8, @r15+
	ld	r7, @r15+
	ld	r6, @r15+	;spu+0x30
	ld	r5, @r15+
	ld	r4, @r15+
	ld	r3, @r15+
	ld	r2, @r15+	;spu+0x40
	ld	r1, @r15+
	ld	r0, @r15+
	
	rte

itron_syscall:
	trap 	1
	jmp	r14

