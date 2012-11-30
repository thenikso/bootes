;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Scheduler
;
; Esegue lo scheduling di processi e thread tramite l'handling dell'interrupt del timer.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



; --------------------------------------------------------------
; Macro
; --------------------------------------------------------------
MACRO LockScheduler
{
	if Bootes.Stage2.Info.MultitaskSupport = 1
	LOCK INC BYTE [GS:Bootes.Scheduler.Handler.LockOperation]
	end if
}

MACRO UnlockScheduler
{
	if Bootes.Stage2.Info.MultitaskSupport = 1
	LOCK DEC BYTE [GS:Bootes.Scheduler.Handler.LockOperation]
	end if
}



;====================================================================
; Inizializza lo scheduler
;====================================================================
align 4
Bootes.Scheduler.Init:
	if Bootes.Stage2.Info.MultitaskSupport = 1
	; Reimposto l'interrupt del timer
	CALL Bootes.Interrupt.Remap, 0x08, Bootes.Scheduler.Handler, Bootes.Scheduler.Handler.OriginalHandler
	end if
RET



if Bootes.Stage2.Info.MultitaskSupport = 1
align 4
;====================================================================
; Gestisce il timer, switch dei processi e conteggio tempo.
;====================================================================
Bootes.Scheduler.Handler:
	; --------------------------------------------------------------
	; Controllo se è possibile eseguire lo scheduling
	; --------------------------------------------------------------
	CMP BYTE [GS:.LockOperation],0
	JNE .Resume

	; --------------------------------------------------------------
	; Blocco l'operazione di scheduling
	; --------------------------------------------------------------
	MOV BYTE [GS:.LockOperation],1

	; --------------------------------------------------------------
	; Salvo lo stato corrente
	; --------------------------------------------------------------
	PUSH AX
	PUSH CX
	PUSH DX
	PUSH BX
	PUSH BP
	PUSH SI
	PUSH DI
	PUSH ES
	PUSH DS
	
	; --------------------------------------------------------------
	; Task switching
	; --------------------------------------------------------------
	; Ricavo il task al quale ridare il controllo
	MOV AX,[GS:Bootes.Task.Data.CurrentTaskID]
	MOV BX,AX
	INC AX
	SHL BX,1
	MOV WORD [GS:Bootes.Task.Data.StackPointers+BX],SP
	CMP AX,[GS:Bootes.Task.Data.Count]
	JB @f
	XOR AX,AX
	@@:
	MOV [GS:Bootes.Task.Data.CurrentTaskID],AX
	; Preparo lo stack pointer
	MOV BX,AX
	SHL BX,1
	MOV SP,[GS:Bootes.Task.Data.StackPointers+BX]
	
	; --------------------------------------------------------------
	; Cambio stato
	; --------------------------------------------------------------
	POP DS
	POP ES
	POP DI
	POP SI
	POP BP
	POP BX
	POP DX
	POP CX
	POP AX

	; --------------------------------------------------------------
	; Sblocco l'operazione
	; --------------------------------------------------------------
	MOV BYTE [GS:.LockOperation],0
	
	; --------------------------------------------------------------
	; Chiamata all'handler originale
	; --------------------------------------------------------------
	.Resume:
	.OriginalHandler.Jump		DB 0xEA		; Jump Far instruction
	.OriginalHandler:
	.OriginalHandler.AddrLo		DW ?
	.OriginalHandler.AddrHi		DW ?

	.LockOperation				DB 0		; Impostato se si vuole impedire lo switch dei task
end if
