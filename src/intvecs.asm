
;PUSH  
push    .macro    reg 
        stw    reg,*B15--[1] 
        .endm 

;POP 
pop    .macro    reg,delay 
        ldw    *++B15[1],reg 
        .if    delay>0 
        nop    delay 
        .endif 
        .endm 

	.sect	".intvecs"
	.align	32*8*4				; must be aligned on 256 word boundary
	.def RESET_V
    .ref	_buffer1_ready_isr
    .ref	_buffer2_ready_isr
    .ref _c_buffer1_ready_isr
    .ref _c_buffer2_ready_isr
    .ref	_edma3ComplHandler0
    .ref	_edma3CCErrHandler0

    .def    rst
RESET_V:			; Reset
rst:
	.ref	_c_int00			; program reset address
	MVKL 	_c_int00, B3
	MVKH	_c_int00, B3
	B		B3
	MVC 	PCE1, B0			; get address of interrupt vectors
	MVC 	B0, ISTP			; set table to point here
	NOP 	
	NOP 	
	NOP 	
	
	.align	32
	.def NMI_V
NMI_V:				; Non-maskable interrupt Vector
	B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	
	.align	32
	.def AINT_V
AINT_V:				; Analysis Interrupt Vector (reserved)
	B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	
	.align	32
	.def MSGINT_V
MSGINT_V:			; Message Interrupt Vector (reserved)
	B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	
	.align	32
	.def INT4_V
INT4_V:				; BUFFER 1 COMPLETE
	push	b0 
    mvkl    .s2     _buffer1_ready_isr,b0
    mvkh    .s2     _buffer1_ready_isr,b0
    b       .s2     b0
    pop		b0, 4
    nop
    nop 	
	
	.align	32
	.def INT5_V
INT5_V:				; BUFFER 2 COMPLETE
	push	b0 
    mvkl    .s2     _buffer2_ready_isr,b0
    mvkh    .s2     _buffer2_ready_isr,b0
    b       .s2     b0
    pop		b0, 4
    nop
    nop 	
	
	.align	32
	.def INT6_V
INT6_V:				; EDMA COMPLETE
    push	b0 
    mvkl    .s2     _edma3ComplHandler0,b0
    mvkh    .s2     _edma3ComplHandler0,b0
    b       .s2     b0
    pop		b0, 4
    nop
    nop
    	
	.align	32
	.def INT7_V
INT7_V:				; EDMA CC ERROR
    push	b0 
    mvkl    .s2     _edma3CCErrHandler0,b0
    mvkh    .s2     _edma3CCErrHandler0,b0
    b       .s2     b0
    pop		b0, 4
    nop
    nop 	
	
	.align	32
	.def INT8_V
INT8_V:              ; EDMA TC0 ERROR
    push	b0 
	  mvkl    .s2     _c_buffer1_ready_isr,b0
    mvkh    .s2     _c_buffer1_ready_isr,b0
    b       .s2     b0
    pop		b0, 4
    nop
    nop  	
	
	.align	32
	.def INT9_V
INT9_V:				; EDMA TC1 ERROR
	  push	b0 
	  mvkl    .s2     _c_buffer2_ready_isr,b0
    mvkh    .s2     _c_buffer2_ready_isr,b0
    b       .s2     b0
    pop		b0, 4
    nop
    nop  	
	
	.align	32
	.def INT10_V
INT10_V: 			; Maskable Interrupt #10
	B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	
	.align	32
	.def INT11_V
INT11_V: 			; Maskable Interrupt #11
	B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	
	.align	32
	.def INT12_V
INT12_V: 			; Maskable Interrupt #12
	B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	
	
	.align	32
	.def INT13_V
INT13_V: 			; Maskable Interrupt #13
int13:
    B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP

;	push	b0 
;    mvkl    .s2     _PerformIsr3,b0
;    mvkh    .s2     _PerformIsr3,b0
;    b       .s2     b0
;    pop		b0, 4
;    nop
;    nop

	.align	32
	.def INT14_V
INT14_V: 			; Maskable Interrupt #14
int14:
    B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP

;	push	b0 
;    mvkl    .s2     _PerformIsr1,b0
;    mvkh    .s2     _PerformIsr1,b0
;    b       .s2     b0
;    pop		b0, 4
;    nop
;    nop

	.align	32
	.def INT15_V
INT15_V: 			; Maskable Interrupt #15 timerIsr
int15:
    B		$
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP 	
	NOP

;	push	b0 
;    mvkl    .s2     _PerformIsr2,b0
;    mvkh    .s2     _PerformIsr2,b0
;    b       .s2     b0
;    pop		b0, 4
;    nop
;    nop	
; the remainder of the vector table is reserved
	.end
