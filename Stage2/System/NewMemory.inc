;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Memory
;
;	Il memory manager permette di gestire un massimo di 64KB (non è previsto il cambio di DS).
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2

; --------------------------------------------------------------
; Funzioni esportate
; --------------------------------------------------------------
SysNamespace Bootes.Memory_Namespace, "Memory"
	Export Bootes.Memory.InfoMemory,0, "InfoMemory", Type.Null


; --------------------------------------------------------------
; Strutture e virtuals
; --------------------------------------------------------------
STRUC Bootes.Memory.BlockDescriptor
{
	.Start:
	.ThreadID:				; Se il blocco è occupato continene
	.UsedBlock		DW ?	; l'ID del thread associato, altrimenti 0. 
	.Previous		DW ?	; Puntatore al blocco precedente.
	.Size			DW ?	; Dimensione della memoria utilizzabile.
	
	.Length			= $-.Start
}

VIRTUAL AT BX
	Block		Bootes.Memory.BlockDescriptor
END VIRTUAL


;====================================================================
; Inizializza la memora per il sistema operativo.
;====================================================================
align 4
Bootes.Memory.Init:
	PUSH AX
	PUSH BX
	
	; Setup memoria
	CALL Bootes.Memory.Setup, Bootes.Stage2.End+ProcessDesriptor.Length,0xFFFF-Bootes.Stage2.End-ProcessDescriptor.Length

	; Inserisco il blocco di 512 byte per il chainloading
	CALL Bootes.Memory.SliceBlock, AX,0x7C00-Bootes.Stage2.End-ProcessDesriptor.Length-Block.Length*2
	MOV BX,AX
	MOV WORD [DS:Block.UsedBlock],1
	CALL Bootes.Memory.SliceBlock, BX,512

	; Aggiungo il namespace
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Memory_Namespace

	POP BX
	POP AX
RET



;====================================================================
; Crea le strutture necessarie per la memoria in riferimento a
; DS corrente.
; --------------------------------------------------------------
; PARAMS
;	Addr	Indirizzo di inizio della memoria da gestire.
;	Size	Dimensione della memoria da gestire.
; RETURN
;	Indirizzo del primo blocco libero.
;====================================================================
Function Bootes.Memory.Setup, Addr, Size
Begin
	LockScheduler
	PUSH BX
	MOV BX,[Addr]
	MOV AX,[Size]
	; Primo blocco libero
	MOV WORD [DS:Block.UsedBlock],0
	MOV WORD [DS:Block.Previous],0
	SUB AX,Block.Length*2
	MOV WORD [DS:Block.Size],AX
	; Secondo blocco
	ADD AX,Block.Length
	ADD AX,BX
	XCHG AX,BX
	MOV WORD [DS:Block.UsedBlock],-1
	MOV WORD [DS:Block.Previous],AX
	MOV WORD [DS:Block.Size],-1
	POP BX
	UnlockScheduler
Return



;====================================================================
; Alloca la memoria.
; PARAMS
;	Size	Quantità di memoria da allocare.
; RETURN
;	Indirizzo della zona di memoria di dimensione 
;	specificata oppure 0 se non è possibile allocare.
;====================================================================
Function Bootes.Memory.Alloc, Size
Begin
	LockScheduler
	
	PUSH BX
	PUSH DX
	
	; --------------------------------------------------------------
	; Ricerco l'inizio della memoria del processo corrente.
	; --------------------------------------------------------------	
	CALL Bootes.Process.GetCurrentProcessMemory
	OR AX,AX
	JZ .Error
	
	MOV BX,AX
	MOV AX,[Size]
	; --------------------------------------------------------------
	; Ricerco un Block libero con dimensione appropriata.
	; --------------------------------------------------------------
	.lFindBlock:
		MOV DX,[DS:Block.UsedBlock]
		; Controllo se sto esaminando ultimo blocco
		CMP DX,-1
		JE .Error
		; Controllo se blocco libero
		OR DX,DX
		MOV DX,[DS:Block.Size]
		JNZ .cFindBlock
		; Controllo se dimendione appropriata
		CMP AX,DX
		JBE .xFindBlock
		; Recupero prossimo blocco
		.cFindBlock:
		ADD BX,DX
		ADD BX,Block.Length
		JMP .lFindBlock
	.xFindBlock
	
	; --------------------------------------------------------------
	; Alloco il blocco di memoria
	; --------------------------------------------------------------
	; Memoria libera residua
	SUB DX,AX
	CMP DX,Block.Length
	JBE .xRemain
	.iRemain:
		CALL Bootes.Memory.SliceBlock, BX,AX
	.xRemain:
	CALL Bootes.Thread.GetCurrentThreadID
	MOV WORD [DS:Block.UsedBlock],AX
	
	; --------------------------------------------------------------
	; Preparo il risualtato
	; --------------------------------------------------------------
	MOV AX,Block.Length
	ADD AX,BX
	
	JMP .Exit
	.Error:
	ErrorSetup AllocFail
	XOR AX,AX
	.Exit:
	POP DX
	POP BX
	
	UnlockScheduler
Return



;====================================================================
; Libera zona di memoria indicata dall'indirizzo specificato.
; --------------------------------------------------------------
; PARAMS
;	Addr	Indirizzo restituito da una chiamata ad Alloc.
;====================================================================
Function Bootes.Memory.Free, Addr
Begin
	LockScheduler
	
	PUSH AX
	PUSH BX
	
	MOV BX,[Addr]
	SUB BX,Block.Length
	
	; --------------------------------------------------------------
	; Verifico che l'indirizzo sia corretto
	; --------------------------------------------------------------
	CALL Bootes.Memory.CheckBlock, BX
	OR AX,AX
	JNE .Exit
	
	; --------------------------------------------------------------
	; Dealloco
	; --------------------------------------------------------------
	MOV WORD [DS:Block.UsedBlock],0
	CALL Bootes.Memory.JoinNextBlock, BX
	MOV BX,[DS:Block.Previous]
	CMP WORD [DS:Block.UsedBlock],0
	JNE .Exit
	CALL Bootes.Memory.JoinNextBlock, BX
	
	.Exit:
	POP BX
	POP AX
	
	UnlockScheduler
Return



;====================================================================
; Espande la zona di memoria specificata della dimensione indicata.
; --------------------------------------------------------------
; PARAMS
;	Addr	Indirizzo della zona di memoria da espandere.
;	Size	Numero di byte da aggiungere alla zona di memoria.
; RETURN
;	Zero se non è stato possibile espandere la memoria, altrimenti
;	puntatore alla nuova zona di memoria espansa.
;====================================================================
Function Bootes.Memory.Expand, Addr,Size
Begin
	LockScheduler
	
	PUSH BX
	PUSH CX
	
	MOV BX,[Addr]
	SUB BX,Block.Length
	
	; --------------------------------------------------------------
	; Verifico che l'indirizzo sia corretto
	; --------------------------------------------------------------
	CALL Bootes.Memory.CheckBlock, BX
	OR AX,AX
	JNE .Error
	
	; --------------------------------------------------------------
	; Verifico disponibilità spazio
	; --------------------------------------------------------------
	PUSH BX
	ADD BX,[DS:Block.Size]
	MOV AX,[Size]
	MOV CX,Block.Length
	ADD BX,CX
	ADD CX,[DS:Block.Size]
	CMP WORD [DS:Block.UsedBlock],0
	JNE .iNewAlloc
	CMP AX,CX
	JBE .eNewAlloc
	.iNewAlloc:
		POP BX
		; Realloco
		ADD CX,AX
		CALL Bootes.Memory.Alloc, CX
		OR AX,AX
		JZ .Error
		ADD BX,Block.Length
		CALL Bootes.Memory.Copy, BX,AX,CX
		CALL Bootes.Memory.Free, BX
		MOV AX,BX
		JMP .Exit
	.eNewAlloc:
		; Fondo con blocco successivo
		SUB CX,AX
		CMP CX,Block.Length*2
		JBE .xRemain
		.iRemain:
			CALL Bootes.Memory.SliceBlock, BX,AX
		.xRemain:
		POP BX
		CALL Bootes.Memory.JoinNextBlock, BX
		MOV AX,BX
		ADD AX,Block.Length
		JMP .Exit
	.xNewAlloc:

	.Error:
	; TODO: impostare errore?
	XOR AX,AX
	.Exit:
	POP CX
	POP BX

	UnlockScheduler
Return



;====================================================================
; Inserisce un blocco libero dopo il gap specificato.
; --------------------------------------------------------------
; PARAMS
;	BAddr	Blocco dopo il quale aggiungerne un'altro.
;	Size	Dimensione in byte da assegnare al blocco fornito.
; RETURN
;	Puntatore al nuovo blocco creato o 0 se è avvenuto un errore.
;====================================================================
Function Bootes.Memory.SliceBlock, BAddr,Size
Begin
	PUSH BX
	PUSH DX
	
	MOV BX,[BAddr]
	MOV AX,[Size]
	MOV DX,[DS:Block.Size]
	; TODO: Controlli correttezza
	SUB DX,AX
	; Imposto nuova dimensione blocco corrente
	MOV WORD [DS:Block.Size],AX
	; Mi muovo al prossimo blocco libero da creare
	ADD AX,BX
	ADD AX,Block.Length
	XCHG AX,BX
	; Imposto nuovo blocco libero
	SUB DX,Block.Length
	MOV WORD [DS:Block.UsedBlock],0
	MOV WORD [DS:Block.Previous],AX
	MOV WORD [DS:Block.Size],DX
	; Collego il blocco appena creato con il successivo
	MOV AX,BX
	ADD BX,DX
	ADD BX,Block.Size
	MOV WORD [DS:Block.Previous],AX

	POP DX
	POP BX
Return



;====================================================================
; Fonde il blocco corrente con il successivo se quest'ultimo è libero
; --------------------------------------------------------------
; PARAMS
;	BAddr	Blocco da fondere con il successivo.
; RETURN
;	Nuova dimensione impostata al blocco indicato oppure 0 se non
;	è stato possibile concatencare i blocchi.
;====================================================================
Function Bootes.Memory.JoinNextBlock, BAddr
Begin
	PUSH BX

	MOV BX,[BAddr]
	MOV AX,Block.Length

	; --------------------------------------------------------------
	; Eseguo fusione
	; --------------------------------------------------------------
	PUSH BX
	ADD AX,[DS:Block.Size]
	ADD BX,AX
	CMP WORD [DS:Block.UsedBlock],0
	JNE .eJoin
	.iJoin:
		ADD AX,[DS:Block.Size]
		POP BX
		MOV WORD [DS:Block.Size],AX
		; Aggiorno previous prossimo blocco
		PUSH AX
		ADD AX,BX
		ADD AX,Block.Length
		XCHG AX,BX
		MOV DWORD [DS:Block.Prevous],AX
		POP AX
		JMP .xJoin
	.eJoin:
		POP BX
		XOR AX,AX
	.xJoin:
	
	POP BX
Return



;====================================================================
; Controlla se l'indirizzo fornito è un blocco valido.
; --------------------------------------------------------------
; PARAMS
;	BAddr	Blocco da controllare.
; RETURN
;	Valore booleano che indica se il blocco è corretto o meno.
;====================================================================
Function Bootes.Memory.CheckBlock, BAddr
Begin
	PUSH BX
	
	MOV AX,[BAddr]
	MOV BX,AX
	ADD AX,[DS:Block.Size]
	ADD AX,Block.Length
	XCHG AX,BX
	CMP AX,[DS:Block.Previous]
	JNE .Error
	MOV BX,AX
	MOV BX,[DS:Block.Previous]
	ADD BX,[DS:Block.Size]
	ADD BX,Block.Length
	CMP AX,BX
	JNE .Error
	MOV AX,1
	JMP .Exit
	
	.Error:
	XOR AX,AX
	.Exit:
	POP BX
Return



;====================================================================
; Copia una zona di memoria in un'altra posizione
; --------------------------------------------------------------
; PARAMS
;	Src		Indirizzo sorgente.
;	Dst		Indirizzo di destinazione.
;	Size	Quantità di memoria copiare.
;====================================================================
Function Bootes.Memory.Copy, Src,Dest,Size
Begin
	PUSH CX
	PUSH SI
	PUSH DI
	
	; TODO: Incrementare performance
	MOV CX,[Size]
	MOV SI,[Src]
	MOV DI,[Dest]
	SHR CX,1
	REP MOVSW
	
	POP DI
	POP SI
	POP CX
Return



;====================================================================
; Alloca uno spazio in memoria superiore ai 64KB iniziali.
; La funzione cambia il DS (Data Segment). Le dimensioni allocate 
; sono multipli di 8KB.
; --------------------------------------------------------------
; PARAMS
;	Size	Quantità di memoria da allocare. Verrà arrotondata a 8KB
;			in eccesso.
; RETURN
;	Il DS verrà impostato al nuovo valore. Sarà necessario 
;	reimpostarlo a 0 una volta terminata l'operazione.
;	Viene restituito l'indirizzo della zona di memoria di dimensione 
;	specificata oppure -1 se non è possibile allocare.
;====================================================================
Function Bootes.Memory.DataAlloc, Size
Begin
	LockScheduler
	PUSH BX
	PUSH CX
	PUSH DX
	
	; Ricavo numero di cluster da 8KB da allocare
	MOV AX,[Size]
	DEC AX
	SHR AX,13
	INC AX
	; Creo la bit mask
	MOV AH,1
	MOV CX,7
	.lMask:
		TEST AH,AL
		JNZ .xMask
		AND AL,AH	; TODO: rivedere (?)
		SHL AH,1
		LOOP .lMask
	.xMask:
	; Cerco la disponibilità
	PUSH CX
	MOV CX,15
	MOV BX,Bootes.Memory.Data.DSStatus
	.lSegment:
		INC BX
		POP DX
		MOV AH,[BX]
		PUSH DX
		.lCluster:
			TEST AH,AL
			JZ .xSegment
			OR DX,DX
			JZ .xCluster
			SHR AH,1
			DEC DX
			JMP .lCluster
		.xCluster:
		LOOP .lSegment
	.xSegment:
	; Alloco lo spazio
	POP AX
	; TODO: Completare (?)
	; Imposto il segmento
	OR CX,CX
	JZ .Error
	SUB BX,Bootes.Memory.Data.DSStatus
	SHL BX,12
	MOV DS,BX
	; Imposto l'offset
	SUB AX,DX
	SHL AX,13
	JMP .Exit
	
	.Error:
	ErrorSetup AllocFail
	MOV AX,-1
	.Exit:
	POP DX
	POP CX
	POP BX
	UnlockScheduler
Return



;====================================================================
; Libera cluster da 8KB allocati con DataAlloc.
; --------------------------------------------------------------
; PARAMS
;	Segment	Valore del segmento DS della memoria da liberare.
;	Offset	Valore dell'offset della memoria da liberare.
;	Size	Quantità di memoria da liberare. Verrà arrotondata a 8KB
;			in eccesso.
;====================================================================
Function Bootes.Memory.DataFree, Segment, Offset, Size
Begin
	LockScheduler
	PUSH AX
	PUSH BX
	PUSH CX
	
	; Ricavo il segmento
	MOV BX,[Segment]
	TEST BX,0xF
	JNZ @f
		SHR BX,12
	@@:
	ADD BX,Bootes.Memory.Data.DSStatus
	; Ricavo la maschera
	MOV AX,[Size]
	DEC AX
	SHR AX,13
	INC AX
	MOV AH,1
	MOV CX,7
	.lMask:
		TEST AH,AL
		JNZ .xMask
		AND AL,AH
		SHL AH,1
		LOOP .lMask
	.xMask:
	; Sposto la maschera
	MOV CX,[Offset]
	SHR CX,13
	SHL AL,CL
	NOT AL
	; Libero la memoria allocata
	MOV AH,[BX]
	AND AH,AL
	MOV [BX],AH
	
	POP CX
	POP BX
	POP AX
	UnlockScheduler
Return



Bootes.Memory.InfoMemory:
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DI
	
	CALL Bootes.IO.Standard.Output, Bootes.Memory.Data.Stats
	; --------------------------------------------------------------
	; Used blocks
	; --------------------------------------------------------------
	CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Used+20,0,[Bootes.Memory.Data.UsedBlockCount],1,10
	;MOV BYTE [Bootes.Memory.Data.Stats.Used+6],' '
	CALL Bootes.IO.Standard.Output, Bootes.Memory.Data.Stats.Used
	CALL Bootes.IO.Standard.Output, Bootes.Memory.Data.Stats.UBlock
	MOV BX,[Bootes.Memory.Data.BlockUsedFirst]
	XOR CX,CX
	.lWriteUsed:
		OR BX,BX
		JZ .xWriteUsed
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+2,0,CX,2,10
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+5,0,[Block.AddrStart],16,16
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+10,0,[Block.AddrEnd],16,16
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+15,0,[Block.Size],5,10
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+21,0,[Block.TaskUID],1,10
		CALL Bootes.IO.Standard.Output, Bootes.Memory.Data.Stats.Temp
		; Prossimo blocco
		INC CX
		MOV BX,[Block.Next]
		JMP .lWriteUsed
	.xWriteUsed:
	; --------------------------------------------------------------
	; Free blocks
	; --------------------------------------------------------------
	CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Free+20,0,[Bootes.Memory.Data.FreeBlockCount],1,10
	CALL Bootes.IO.Standard.Output, Bootes.Memory.Data.Stats.Free
	CALL Bootes.IO.Standard.Output, Bootes.Memory.Data.Stats.FBlock
	MOV BX,[Bootes.Memory.Data.BlockFreeFirst]
	XOR CX,CX
	.lWriteFree:
		OR BX,BX
		JZ .xWriteFree
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+2,0,CX,2,10
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+5,0,[Block.AddrStart],16,16
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+10,0,[Block.AddrEnd],16,16
		CALL Bootes.String.FromNumber, Bootes.Memory.Data.Stats.Temp+15,0,[Block.Size],5,10
		MOV DWORD [Bootes.Memory.Data.Stats.Temp+20],'    '
		CALL Bootes.IO.Standard.Output, Bootes.Memory.Data.Stats.Temp
		; Prossimo blocco
		INC CX
		MOV BX,[Block.Next]
		JMP .lWriteFree
	.xWriteFree:
	
	POP DI
	POP CX
	POP BX
	POP AX
RET

;====================================================================
; Data
;====================================================================
Bootes.Memory.Data:
	; Occupazione DataSegment
	.DSStatus		DB 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
					   0xFF, \		; 000A:0000-000A:FFFF	Graphics Mode Video Card
					   0xFF, \		; 000B:0000-000B:7FFF	Monocrome text
					   		 \		; 000B:8000-000B:FFFF	Colored text
					   0xFF, \		; 000C:0000-000C:7FFF	Video RAM
					   0xFF, \		; 000C:8000-000D:FFFF	Special adapter
					   0xFF, 0xFF	; 000E:0000-000F:FFFF	BIOS
	; Statistiche
	.Stats			DB "Memory Manager Statistics",13,10,0
	.Stats.Used		DB "Used memory blocks:    ",13,10,0
	.Stats.Free		DB "Free memory blocks:    ",13,10,0
	.Stats.UBlock	DB "  ID Addr End  Size  Task",13,10,0
	.Stats.FBlock	DB "  ID Addr End  Size ",13,10,0
	.Stats.Temp		DB "                         ",13,10,0