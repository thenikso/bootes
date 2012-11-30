;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Process
;
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2

; --------------------------------------------------------------
; Strutture e virtuals
; --------------------------------------------------------------
STRUC Bootes.Process.ProcessDescriptor
{
	.Start:
	.ProcessID			DW ?	; Identificativo del processo
	.MemoryAddr			DW ?	; Indica il primo indirizzo di memoria utilizzabile.
	.MemorySize			DW ?
	.StackSegment		DW ?
	.StackPointer		DW ?
	
	.Next				DW ?
	
	.Length				= $-.Start
}

VIRTUAL AT SI
	ProcessDescriptor	Bootes.Process.ProcessDescriptor
END VIRTUAL



;====================================================================
; Init
; --------------------------------------------------------------
; REMARKS
;	Inizializza il processo primario.
;====================================================================
Bootes.Process.Init:
	RET



;====================================================================
; Setup
; --------------------------------------------------------------
; REMARKS
;	Installa un processo al DS corrente.
;====================================================================
Bootes.Process.Setup:
	RET



;====================================================================
; GetCurrentProcessDescriptor
; --------------------------------------------------------------
; REMARKS
;	Restituisce il descrittore del processo corrente oppure -1 se non
;	è possibile rinvenirlo.
;====================================================================
Bootes.Process.GetCurrentProcessDescriptor:
	LockScheduler
	MOV AX,DS
	OR AX,AX
	JNZ .eKernelProcess
	.iKernelProcess:
		MOV AX,[GS:Bootes.Process.Data.KernelProcessDescriptor]
		JMP .Exit
	.eKernelProcess:
		XOR AX,AX
		; TODO: controllare validità descrittore.
	.xKernelProcess:
	JMP .Exit
	.Error:
	MOV AX,-1
	.Exit:
	UnlockScheduler
	RET



;====================================================================
; GetCurrentProcessMemory
; --------------------------------------------------------------
; REMARKS
;	Restutuisce un puntatore all'indirizzo di memoria utilizzabile dal
;	processo corrente oppure 0 se non è stato rinvenuto.
;====================================================================
Bootes.Process.GetCurrentProcessMemory:
	LockScheduler
	PUSH SI
	; Recupero il descrittore del processo corrente
	CALL Bootes.Process.GetCurrentProcessDescriptor
	OR AX,AX
	JE .Error
	; Recupero il descrittore del MemoryDescriptor
	MOV SI,AX
	MOV AX,[ProcessDescriptor.MemoryAddr]
	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	POP SI
	UnlockScheduler
	RET



;====================================================================
; ScheduleProcess
; --------------------------------------------------------------
; REMARKS
;	Effettua il passaggio al prossimo processo attivo impostando
;	l'ambiente in modo da avviarne l'esecuzione alla prossima
;	istruzione IRET.
;====================================================================
Bootes.Process.ScheduleProcess:
	; Salvare stato corrente
	; Recuperare processo corrente
	; Recuperare processo Next
	; Recuperare e impostare segmenti
	; Impostazione processo corrente
	; Ripristino processo
	RET



;====================================================================
; Data
;====================================================================
Bootes.Process.Data:
	.CurrentProcessUID				DW 0	; Identificativo del processo corrente.
	.ProcessCount					DW 0	; Conteggio del numero di processi attivi.
	.KernelProcessDescriptor		DW 0	; Descrittore del processo kernel.