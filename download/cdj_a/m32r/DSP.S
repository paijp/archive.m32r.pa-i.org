
;	calc_audioevent(UW *in1, UW *in2, UW *filter, UW *out);
;	                r0       r1       r2          r3

;	mix_audioevent(UW in[], UB da_buf[], UH vol[][]);
;	               r0       r1        r2
;
;	in[x] * vol[y][x] -> out[y]	x:0-3 y:0-3

.global	calc_audioevent
.global	mix_audioevent
.text
.align	2
calc_audioevent:
	st	r8, @-r15
	st	r9, @-r15
	st	r10, @-r15
	st	r11, @-r15
	st	r12, @-r15
	st	r13, @-r15
	st	r14, @-r15
	
	st	r3, @-r15
	st	r2, @-r15	;@r15:filter @(4, r15):out
	
	ld	r14, @r0+
	ld	r13, @r0+
	ld	r12, @r0+
	ld	r11, @r0+
	ld	r10, @r0+
	ld	r9, @r0+
	ld	r8, @r0+
	ld	r7, @r0+
	ld	r6, @r0+
	ld	r5, @r0+
	ld	r4, @r0+
	ld	r3, @r0+
	
	ld24	r2, 0x1ffff
	and	r0, r2
	cmpui	r0, 0x30
	bnc	skip
	
	ld24	r2, #skip
	sub	r2, r0
	jmp	r2
	
	ld	r13, @r1+ || nop
	ld	r12, @r1+ || nop
	ld	r11, @r1+ || nop
	ld	r10, @r1+ || nop
	ld	r9, @r1+ || nop
	ld	r8, @r1+ || nop
	ld	r7, @r1+ || nop
	ld	r6, @r1+ || nop
	ld	r5, @r1+ || nop
	ld	r4, @r1+ || nop
	ld	r3, @r1+ || nop
skip:
	ld	r2, @r15	; r2 = filter
	ld	r0, @r2+
	ld	r1, @r2+
	
	mulwhi	r0, r14
	ld	r0, @r2+
	macwhi	r1, r13
	ld	r1, @r2+
	macwhi	r0, r12
	ld	r0, @r2+
	macwhi	r1, r11
	ld	r1, @r2+
	macwhi	r0, r10
	ld	r0, @r2+
	macwhi	r1, r9
	ld	r1, @r2+
	macwhi	r0, r8
	ld	r0, @r2+
	macwhi	r1, r7
	ld	r1, @r2+
	macwhi	r0, r6
	ld	r0, @r2+
	macwhi	r1, r5
	ld	r1, @r2+
	macwhi	r0, r4
	macwhi	r1, r3
	
	ld	r2, @(4, r15)	; r2 = out
	mvfacmi	r0
	st	r0, @r2
	
	ld	r2, @r15+	; r2 = filter
	ld	r0, @r2+
	ld	r1, @r2+
	
	mulwlo	r0, r14
	ld	r0, @r2+
	macwlo	r1, r13
	ld	r1, @r2+
	macwlo	r0, r12
	ld	r0, @r2+
	macwlo	r1, r11
	ld	r1, @r2+
	macwlo	r0, r10
	ld	r0, @r2+
	macwlo	r1, r9
	ld	r1, @r2+
	macwlo	r0, r8
	ld	r0, @r2+
	macwlo	r1, r7
	ld	r1, @r2+
	macwlo	r0, r6
	ld	r0, @r2+
	macwlo	r1, r5
	ld	r1, @r2+
	macwlo	r0, r4
	macwlo	r1, r3
	
	ld	r2, @r15+	; r2 = out
	mvfacmi	r0
	st	r0, @+r2
	
	ld	r14, @r15+
	ld	r13, @r15+
	ld	r12, @r15+
	ld	r11, @r15+
	ld	r10, @r15+
	ld	r9, @r15+
	ld	r8, @r15+
	
	jmp	r14


mix_audioevent:
	ld	r3, @r0+	; r0=in
	ld	r4, @r0+
	ld	r5, @r0+
	ld	r6, @r0+
	
	ld	r0, @r2+	; r2=volume
	ld	r7, @r2+
	mulwhi	r3, r0
	macwlo	r4, r0
	macwhi	r5, r7
	macwlo	r6, r7
	mvfacmi	r7
	
	stb	r7, @(2, r1)	; r1 = da_buf
	srai	r7, 8
	stb	r7, @(1, r1)
	srai	r7, 8
	and3	r7, r7, 0xf
	stb	r7, @(0, r1)
	
	ld	r0, @r2+
	ld	r7, @r2+
	mulwhi	r3, r0
	macwlo	r4, r0
	macwhi	r5, r7
	macwlo	r6, r7
	mvfacmi	r7
	
	stb	r7, @(8, r1)	; r1 = da_buf
	srai	r7, 8
	stb	r7, @(7, r1)
	srai	r7, 8
	and3	r7, r7, 0xf
	stb	r7, @(6, r1)
	
	ld	r0, @r2+
	ld	r7, @r2+
	mulwhi	r3, r0
	macwlo	r4, r0
	macwhi	r5, r7
	macwlo	r6, r7
	mvfacmi	r7
	
	stb	r7, @(5, r1)	; r1 = da_buf
	srai	r7, 8
	stb	r7, @(4, r1)
	srai	r7, 8
	and3	r7, r7, 0xf
	stb	r7, @(3, r1)
	
	ld	r0, @r2+
	ld	r7, @r2+
	mulwhi	r3, r0
	macwlo	r4, r0
	macwhi	r5, r7
	macwlo	r6, r7
	mvfacmi	r7
	
	stb	r7, @(11, r1)	; r1 = da_buf
	srai	r7, 8
	stb	r7, @(10, r1)
	srai	r7, 8
	and3	r7, r7, 0xf
	stb	r7, @(9, r1)
	
	jmp	r14


