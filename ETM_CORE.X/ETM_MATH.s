.ifdef __dsPIC30F
        .include "p30fxxxx.inc"
.endif
.ifdef __dsPIC33F
        .include "p33Fxxxx.inc"
.endif

	
.global  _ETMMath16Add
	;; uses and does not restore W0->W1	
	;; value_1 is in W0
	;; value_2 is in W1
	.text
_ETMMath16Add:
	ADD	W0, W1, W0			; W0 = W0+W1
	BRA	C, _ETMMath16Add_Saturation	; There was on overflow durring the addition
	RETURN
	
_ETMMath16Add_Saturation:
	MOV	#0xFFFF, W0
	RETURN


		
.global  _ETMMath16Sub
	;; uses and does not restore W0->W1	
	;; value_1 is in W0
	;; value_2 is in W1
	.text
_ETMMath16Sub:
	SUB	W0, W1, W0
	BRA	LTU, _ETMMath16Sub_Saturation	; value_2 was greater than value_1
	RETURN
	
_ETMMath16Sub_Saturation:
	MOV	#0x0000, W0
	RETURN
	
	
