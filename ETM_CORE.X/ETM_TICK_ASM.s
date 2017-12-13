.ifdef __dsPIC30F
        .include "p30fxxxx.inc"
.endif
.ifdef __dsPIC33F
        .include "p33Fxxxx.inc"
.endif


.section .nbss, bss, near    	
	_etm_tick_low_previous:	.space 2
	_etm_tick:              .space 4	
.text	

	
	;; ----------------------------------------------------------


.global  _ETMTickGet			
	;; uses and does not restore W0->W1
	.text
_ETMTickGet:
	MOV		#0x00FF, W0	
	CP		_etm_tick_timer_select
	BRA             Z, _ETMTickGet_TIMER_NOT_SELECTED 		; etm_tick_timer was equal to 0xFF, branch to Timer not selected

	;; Update etm_tick
	MOV            	_etm_tick_TMR_ptr, W1 				; move the timer pointer to W1
	MOV            	[W1], W0 					; move the timer value to W0
	MOV            	W0, _etm_tick 					; copy the TMR reading to the low word of etm_tick
	CP 		_etm_tick_low_previous 				; etm_tick_previous - TMR reading
	BRA		LEU, _ETMTickGet_HighByteDone 			; branch if ((etm_tick_previous - WO{TMR reading}) <= 0) 
	INC		(_etm_tick + 2)					; increment the high work of etm_tick if previous > current
	
_ETMTickGet_HighByteDone:	
	MOV            	W0, _etm_tick_low_previous			; copy the TMR reading to the stored value

	;; copy high byte of etm_tick for return (low byte is already in W0)
	MOV		(_etm_tick+2), W1
	RETURN
	
_ETMTickGet_TIMER_NOT_SELECTED:
	CLR		W0
	CLR		W1
	RETURN

	
