
.global	sp
.data
.align 2
sp:	.word	0

.global	trapent
.text
.align	2
trapent:
	ST	R14,@-R15
	ST	R13,@-R15
	ST	R12,@-R15
	ST	R11,@-R15
	ST	R10,@-R15
	ST	R9,@-R15
	ST	R8,@-R15
	ST	R7,@-R15
	ST	R6,@-R15
	ST	R5,@-R15
	ST	R4,@-R15
	ST	R3,@-R15
	ST	R2,@-R15
	ST	R1,@-R15
	ST	R0,@-R15
	
	MVFC	R0,CR0
	MVFC	R1,CR1
	MVFC	R2,CR2
	MVFC	R3,CR3
	MVFC	R6,CR6
	ST	R6,@-R15
	ST	R3,@-R15
	ST	R2,@-R15
	ST	R1,@-R15
	ST	R0,@-R15
	
	SETH	R0,#SHIGH(sp)
	ST	R15,@(LOW(sp),R0)
	
	BL	cli
	
	LD	R0,@R15+
	LD	R1,@R15+
	LD	R2,@R15+
	LD	R3,@R15+
	LD	R6,@R15+
	MVTC	R0,CR6
	MVTC	R1,CR1
	MVTC	R2,CR2
	MVTC	R3,CR3
	MVTC	R6,CR6
	
	LD	R0,@R15+
	LD	R1,@R15+
	LD	R2,@R15+
	LD	R3,@R15+
	LD	R4,@R15+
	LD	R5,@R15+
	LD	R6,@R15+
	LD	R7,@R15+
	LD	R8,@R15+
	LD	R9,@R15+
	LD	R10,@R15+
	LD	R11,@R15+
	LD	R12,@R15+
	LD	R13,@R15+
	LD	R14,@R15+
	
	JMP	R14

