;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Time
;
; Questa classe fa riferimenti a Bootes.Task.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

SysNamespace Bootes.Time_Namespace, "Time"
	Export Bootes.Time.GetTime, 0, "GetTime";, Type.Object
	Export Bootes.Time.PrintTime, 0, "PrintTime", Type.Null



; --------------------------------------------------------------
; Strutture
; --------------------------------------------------------------
STRUC Bootes.Time.Time
{
	.Start:
	; Ticks
	.Ticks:
	.TicksLo	DW ?
	.TicksHi	DW ?
	; Ora
	.Hour		DB ?
	.Minutes	DB ?
	.Seconds	DB ?
	.Format		DB ?
	; Stirnga ora
	.String		DB "00:00:00",0
	.Size		= $-.Start
}


; --------------------------------------------------------------
; Macro
; --------------------------------------------------------------
MACRO GetTime
{
	if Bootes.Stage2.Info.MultitaskSupport = 0
		CALL Bootes.Time.GetTime
	end if
}



;====================================================================
; Inizializza l'handler del timer.
;====================================================================
align 4
Bootes.Time.Init:
	; Inizializzo il namespace
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Time_Namespace
RET



;====================================================================
; Restituisce l'ora corrente.
; --------------------------------------------------------------
; RETURN
;	Puntatore a una struttura Time oppure zero se è avvenuto un 
;	errore.
;====================================================================
align 4
Bootes.Time.GetTime:
	PUSH CX
	PUSH DX
	
	; Interrupt per recuperare il tempo
	MOV AH,0x02
	CLC
	INT 0x1A
	JC .Error
	; Imposto la struttura time
	MOV BYTE [Bootes.Time.Data.Time.Hour],CH
	MOV BYTE [Bootes.Time.Data.Time.Minutes],CL
	MOV BYTE [Bootes.Time.Data.Time.Seconds],DH
	MOV BYTE [Bootes.Time.Data.Time.Format],DL
	; Imposto la stringa
	; Min
	MOV AL,CL
	AND AX,0xF
	CALL Bootes.String.FromNumber, Bootes.Time.Data.Time.String+4,0,AX,1,10
	MOV AL,CL
	SHR AX,4
	AND AX,0xF
	CALL Bootes.String.FromNumber, Bootes.Time.Data.Time.String+3,0,AX,1,10
	; Hour
	SHR CX,8
	MOV AL,CL
	AND AX,0xF
	CALL Bootes.String.FromNumber, Bootes.Time.Data.Time.String+1,0,AX,1,10
	MOV AL,CL
	SHR AX,4
	AND AX,0xF
	CALL Bootes.String.FromNumber, Bootes.Time.Data.Time.String,0,AX,1,10
	; Sec
	MOV AL,DH
	AND AX,0xF
	CALL Bootes.String.FromNumber, Bootes.Time.Data.Time.String+7,0,AX,1,10
	MOV AL,DH
	SHR AX,4
	AND AX,0xF
	CALL Bootes.String.FromNumber, Bootes.Time.Data.Time.String+6,0,AX,1,10
	; Prendo i ticks
	MOV AH,0
	INT 0x1A
	MOV [Bootes.Time.Data.Time.TicksLo],DX
	MOV [Bootes.Time.Data.Time.TicksHi],CX
	; Risultato
	MOV AX,Bootes.Time.Data.Time

	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	POP DX
	POP CX
RET



;====================================================================
; Scrive l'ora corrente sullo standard output.
;====================================================================
align 4
Bootes.Time.PrintTime:
	CALL Bootes.Time.GetTime
	CALL Bootes.IO.Standard.Output, Bootes.Time.Data.Time.String
	CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
RET



;====================================================================
; Attende un predeterminato numero di ticks.
; 10sec=500ticks -> 1sec=50ticks -> 0,1sec=5ticks -> 0,02sec=1ticks
; --------------------------------------------------------------
; PARAMS
;	Ticks	Numero di ticks da attendere.
;====================================================================
Function Bootes.Time.Wait, Ticks
Begin
	PUSH BX
	PUSH CX
	MOV AX,[Ticks]
	if Bootes.Stage2.Info.MultitaskSupport = 0
	CALL Bootes.Time.GetTime
	end if
	MOV BX,[Bootes.Time.Data.Time.Ticks]
	.lWait:
		if Bootes.Stage2.Info.MultitaskSupport = 0
		CALL Bootes.Time.GetTime
		end if
		MOV CX,[Bootes.Time.Data.Time.Ticks]
		SUB CX,BX
		CMP CX,AX
		JAE .Exit
		JMP .lWait
	.Exit:
	POP CX
	POP BX

Return



;====================================================================
; Task di genstione temporizzazioni
;====================================================================
if Bootes.Stage2.Info.MultitaskSupport = 1
align 4
Bootes.Time.Timer:
	; Inizializzazione
;	MOV AH,0
;	MOV SI,Bootes.Time.Data.Time.TicksHi
;	MOV DI,Bootes.Time.Data.Time.TicksLo
	.lTimer:
		; Prendo il tempo
		CALL Bootes.Time.GetTime
		JMP .lTimer
end if



;====================================================================
; Data
;====================================================================
align 4
Bootes.Time.Data:
	.Time			Bootes.Time.Time	; Tempo