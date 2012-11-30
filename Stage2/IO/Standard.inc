;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.IO.Standard
;
; Gestisce le funzioni standard di input e output.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

;====================================================================
; Imposta la funzione di input standard.
; --------------------------------------------------------------
; PARAMS
;	InDelegate	Puntatore ad una funzione da considerare funzione
;				standard di input.
;====================================================================
Function Bootes.IO.Standard.SetInput, InDelegate
Begin
	PUSH AX
	MOV AX,[InDelegate]
	OR AX,AX
	JZ .Exit
	MOV WORD [Bootes.IO.Standard.Data.InputDelegate],AX
	.Exit:
	POP AX
Return



;====================================================================
; Chiama la funzione di input standard.
; --------------------------------------------------------------
; RETURN
;	Puntatore al buffer di dati letti oppure 0 se è avvenuto un 
;	errore di lettura.
;====================================================================
Bootes.IO.Standard.Input:
	MOV AX,WORD [Bootes.IO.Standard.Data.InputDelegate]
	OR AX,AX
	JZ .Exit
	CALL AX
	.Exit:
	RET



;====================================================================
; Imposta la funzione di output standard.
; --------------------------------------------------------------
; PARAMS
;	OutDelegate	Puntatore ad una funzione da considerare funzione
;				standard di output.
;====================================================================
Function Bootes.IO.Standard.SetOutput, OutDelegate
Begin
	PUSH AX
	MOV AX,[OutDelegate]
	OR AX,AX
	JZ .Exit
	MOV WORD [Bootes.IO.Standard.Data.OutputDelegate],AX
	.Exit:
	POP AX
Return



;====================================================================
; Chiama la funzione di output standard.
; --------------------------------------------------------------
; PARAMS
;	Data	Puntatore al buffer di dati da inviare alla funzione di
;			output standard.
;====================================================================
Function Bootes.IO.Standard.Output, Data
Begin
	CMP WORD [Bootes.IO.Standard.Data.OutputDelegate],0
	JZ .Exit
	CALL WORD [Bootes.IO.Standard.Data.OutputDelegate], [Data]
	.Exit:
Return


;====================================================================
; Imposta la funzione di error output standard.
; --------------------------------------------------------------
; PARAMS
;	ErrDelegate	Puntatore ad una funzione da considerare funzione
;				standard di error output.
;====================================================================
Function Bootes.IO.Standard.SetError, ErrDelegate
Begin
	PUSH AX
	MOV AX,[ErrDelegate]
	OR AX,AX
	JZ .Exit
	MOV WORD [Bootes.IO.Standard.Data.ErrorDelegate],AX
	.Exit:
	POP AX
Return



;====================================================================
; Chiama la funzione di error output standard.
; --------------------------------------------------------------
; PARAMS
;	Data	Puntatore al buffer di dati da inviare alla funzione di
;			error output standard.
;====================================================================
Function Bootes.IO.Standard.Error, Data
Begin
	CMP WORD [Bootes.IO.Standard.Data.ErrorDelegate],0
	JZ .Exit
	CALL WORD [Bootes.IO.Standard.Data.ErrorDelegate], [Data]
	.Exit:
Return



;====================================================================
; Data
;====================================================================
align 2	
Bootes.IO.Standard.Data:
	.InputDelegate				DW 0
	.OutputDelegate				DW 0
	.ErrorDelegate				DW 0