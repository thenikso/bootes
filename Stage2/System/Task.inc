;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Task manager
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



;====================================================================
; Aggiunge un task
; --------------------------------------------------------------
; PARAMS
;	Entry		Entrypoint del nuovo task.
;	StackSize	Dimensioni utitli desiderate dello stack
; RETURN
;	Zero se è avvenuto un errore altrimenti lo SP del task.
;====================================================================
Function Bootes.Task.Start, Entry, StackSize
Begin
	if Bootes.Stage2.Info.MultitaskSupport = 1
	LockScheduler
	PUSHF
	CLI
	PUSH BX
	PUSH DX
	
	; Salvo l'ID del task corrente
	MOV DX,[Bootes.Task.Data.CurrentTaskID]

	; --------------------------------------------------------------
	; Cerco l'ID del nuovo task
	; --------------------------------------------------------------
	MOV AX,[Bootes.Task.Data.Count]
	CMP AX,Bootes.Stage2.Info.MaxTasks
	JAE .Error

	; --------------------------------------------------------------
	; Inserisco il nuovo task
	; --------------------------------------------------------------
	; Imposto il task in via di creazione come quello corrente
	MOV BX,AX
	MOV [Bootes.Task.Data.CurrentTaskID],AX
	; Inserisco il riferimento dello UID
	SHL BX,1
	MOV AX,[Bootes.Task.Data.NextUID]
	MOV [Bootes.Task.Data.TasksUID+BX],AX
	INC WORD [Bootes.Task.Data.NextUID]
	; Alloco e inserisco il riferimento dello stack
	MOV AX,[StackSize]
	CMP AX,Bootes.Stage2.Info.TaskMaxStackSize
	JA .Error
	ADD AX,24	; Numero di byte necessari al task switch
	CALL Bootes.Memory.Alloc, AX
	OR AX,AX
	JZ .Error
	ADD AX,[StackSize]
	DEC AX
	MOV [Bootes.Task.Data.StackPointers+BX],AX
	PUSH AX
	MOV AX,[Bootes.Memory.Data.BlockCreatedLast]
	MOV [Bootes.Task.Data.StacksMemoryBlock+BX],AX
	POP BX
	; Data e ext segment
	MOV WORD [BX],0
	MOV WORD [BX+2],0
	; Code offset
	MOV AX,[Entry]
	MOV [BX+18],AX
	; Code segment
	MOV AX,CS
	MOV [BX+20],AX
	; EFLAGS
	PUSHF
	POP AX
	OR AX,0x200		; Abilito gli IRQ tramite gli EFLAGS
	MOV [BX+22],AX
	; Risultato
	;MOV AX,[Bootes.Task.Data.Count]
	MOV AX,BX
	INC WORD [Bootes.Task.Data.Count]

	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	; Ripristino il task originale
	MOV [Bootes.Task.Data.CurrentTaskID],DX
	
	POP DX
	POP BX
	POPF	; Recupero gli EFLAGS e riabilito gli IRQ
	UnlockScheduler
	end if
Return

;====================================================================
; Ferma l'esecuzione del task corrente
;====================================================================
align 4
Bootes.Task.Stop:
	if Bootes.Stage2.Info.MultitaskSupport = 1
	LockScheduler
	CLI
	; Ricavo sorgente e destinazione dello stack
	MOV AX,Bootes.Stage2.Info.Origin
	MOV SI,AX
	MOV DI,AX
	; Controllo se il task da eliminare è l'ultimo
	DEC WORD [Bootes.Task.Data.Count]
	MOV AX,[Bootes.Task.Data.Count]
	CMP AX,[Bootes.Task.Data.CurrentTaskID]
	JE .StopExit
	; Ricavo indirizzo dello stack sorgente
	MOV BX,AX
	SHL BX,1
	SHL AX,10
	SUB SI,AX
	; Cambio i riferimenti dello stack pointer e dell'UID
	MOV DX,[Bootes.Task.Data.StackPointers+BX]
	MOV CX,[Bootes.Task.Data.TasksUID+BX]
	MOV [Bootes.Task.Data.TasksUID+BX],0
	MOV BX,[Bootes.Task.Data.CurrentTaskID]
	SHL BX,1
	MOV [Bootes.Task.Data.StackPointers+BX],DX
	MOV [Bootes.Task.Data.TasksUID+BX],CX
	; Ricavo indirizzo dello stack da eliminare
	SHL AX,10
	SUB DI,AX
	; Copio
	MOV CX,512/2
	REP MOVSW
	; Attivo gli interrupt e aspetto il timer
	.StopExit:
	STI
	UnlockScheduler
	JMP $
	end if
RET


if Bootes.Stage2.Info.MultitaskSupport = 1
;====================================================================
; Ricava l'ID univoco del task corrente.
; --------------------------------------------------------------
; RETURN
;	Identificatore univoco del task corrente.
;====================================================================
align 4
Bootes.Task.GetCurrentTaskUID:
	PUSH BX
	MOV BX,[Bootes.Task.Data.CurrentTaskID]
	SHL BX,1
	MOV AX,[Bootes.Task.Data.TasksUID+BX]
	POP BX
	RET


;====================================================================
; Data
;====================================================================
align 4
Bootes.Task.Data:
	; Informazioni generali
	.CurrentTaskID				DW 0
	.Count						DW 0
	
	.StackPointers				RW Bootes.Stage2.Info.MaxTasks
	.StacksMemoryBlock			RW Bootes.Stage2.Info.MaxTasks
	.NextUID					DW 1
	.TasksUID					RW Bootes.Stage2.Info.MaxTasks
end if