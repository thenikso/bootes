;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Interrupt
;
; Mappa gli interrupt alle chiamate di sistema
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



;====================================================================
; Remappa un interrapt in modo da fargli chiamare l'handler 
; specificato.
; --------------------------------------------------------------
; PARAMS
;	Interrupt	Numero dell'interrupt da rimappare.
;	Handler		Indirizzo della nuovo handler.
;	Store		Indirizzo di memoria dove salvare il vecchio 
;				puntatore. Oppure 0.
;====================================================================
Function Bootes.Interrupt.Remap, Interrupt, Handler, Store
Begin
	LockScheduler
	
	PUSH AX
	PUSH BX
	PUSH SI
	PUSH DI
	
	MOV SI,[Interrupt]
	MOV DI,[Store]
	SHL SI,2
	; Salvo l'handler originale
	MOV AX,[SI]
	MOV BX,[SI+2]
	OR DI,DI
	JZ @f
	MOV [DI],AX
	MOV [DI+2],BX
	@@:
	MOV DI,SI
	ADD DI,Bootes.Interrupt.Data.Original
	MOV [DI],AX
	MOV [DI+2],BX
	; Imposto il nuovo handler
	MOV AX,[Handler]
	MOV [SI],AX
	MOV AX,CS
	MOV [SI+2],AX
	
	POP DI
	POP SI
	POP BX
	POP AX
	
	UnlockScheduler
Return



;====================================================================
; Ripristina tutti gli handler originali.
;====================================================================
Function Bootes.Interrupt.RestoreOriginal
Begin
	LockScheduler
	PUSH AX
	PUSH CX
	PUSH SI
	PUSH DI
	
	MOV SI,Bootes.Interrupt.Data.Original
	MOV CX,Bootes.Interrupt.Data.MaxRemappable
	MOV AX,0
	.lRestore:
		CMP DWORD [SI],0
		JE .nRestore
		; Ripristino l'handler originale
		MOV DI,AX
		SHL DI,2
		MOVSD
		JMP @f
		.nRestore:
		ADD SI,4
		@@:
		INC AX
		LOOP .lRestore
	
	POP DI
	POP SI
	POP CX
	POP AX
	UnlockScheduler
Return



; --------------------------------------------------------------
; Data
; --------------------------------------------------------------
align 4
Bootes.Interrupt.Data:
	.MaxRemappable			= 16
	.Original				RD .MaxRemappable
	