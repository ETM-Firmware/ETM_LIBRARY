.ifdef __dsPIC30F
        .include "p30fxxxx.inc"
.endif
.ifdef __dsPIC33F
        .include "p33Fxxxx.inc"
.endif



	

	;; ---------------------------------------------------------------------
	
	.global  _RCFilterNTau
	.text
_RCFilterNTau:
	;; w0 previous value
	;; w1 current reading
	;; w2 filter tau bit shift (if this value is greater than 15, funny things may happen)
	
	;; W3 is used to store the multipler
	;; W4:45 are use as the multiplication results register

	;;  If N < 16, keep going, otherwise set N = 15
	CP 		W2, #16
	BRA 		LT, _RCFilterNTauOK
	MOV		#15, W2
_RCFilterNTauOK:	

	;; If the previous value is zero, then we want to start filtering not from zero, but from the current value
	;; This will make the response at startup much faster
	CP0		W0
	BRA		NZ, _RCFilterNTau_previous_value_not_zero
	MOV		W1, W0
_RCFilterNTau_previous_value_not_zero:	

	;; First Compute the multiplier from N (tau bit shift)
	MOV		#1, W3
	SL		W3, W2, W3 		
	DEC		W3, W3				; W3 now contains 2^N -1
	
	MUL.UU		W3, W0, W4 			; Store results in W4, W5, High Word is W5
	ADD		W1, W4, W4			
        ADDC		#0, W5	                        ; Add W1 to W4:W5

	;; Now need to shift W4:W5 right by N bits and store results in W0
	LSR		W4, W2, W4 			; Shit W4 Right by N bits	
	SUBR		W2, #16, W2					
	SL		W5, W2, W5 			; Shift W5 Left by 16-N bits
	IOR		W4, W5, W0			


	;; Need to provide a little more math to help to filtered value stabilize
	CP		W1, W0
	BRA		Z, _RCFilterNTau_AdjustDone
	BRA		LT, _RCFilterNTau_AdjustNegative
	INC2		W0, W0				; Increment W0 by 2 (it will be decremented 1 by the next command)

_RCFilterNTau_AdjustNegative:	
	DEC		W0, W0

_RCFilterNTau_AdjustDone:		
	RETURN


