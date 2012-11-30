;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Memory
;
; La memoria utilizzabile dal memory manager è quella successiva allo Stage2. Non è superiore ai
; 0x10000 byte inquanto non è prevista la variazione del data segment (DS).
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2

; --------------------------------------------------------------
; Funzioni esportate
; --------------------------------------------------------------
SysNamespace Bootes.Memory_Namespace, "Memory"
	Export Bootes.Memory.InfoMemory,0, "InfoMemory", Type.Null
	Export Bootes.Memory.Collect,0, "Collect", Type.Null


; --------------------------------------------------------------
; Strutture e virtuals
; --------------------------------------------------------------
STRUC Bootes.Memory.MemoryDescriptor
{
	.Start:
	; Puntatori
	.UsedRoot			DW ?	; Indirizzo alla prima struttura Block utilizzata.
	.FreeRoot			DW ?	; Indirizzo alla prima struttura Block libera.
	.LastBlock			DW ?	; Indirizzo dell'ultima struttura Block creata.
	; Informazioni
	.BlockListAddr		DW ?	; Indirizzo di memoria della lista.
	.BlockListBytes		DW ?	; Numero di strutture block moltiplicato per la loro dimensione.
	.BlockListMaxBytes	DW ? 	; Numero massimo di byte allocabili nella lista.
	.FreeBlockCount		DW 0	; Conteggio blocchi liberi
	.UsedBlockCount		DW 0	; Conteggio blocchi usati
	
	.Length			= $-.Start
}

STRUC Bootes.Memory.BlockDescriptor
{
	.Start:
	.AddrStart		DW ?
	.AddrEnd		DW ?
	.Size			DW ?
	.TaskUID		DW ?
	
	.Previous		DW ?
	.Next			DW ?
	
	.Length			= $-.Start
}

VIRTUAL AT BX
	Block		Bootes.Memory.BlockDescriptor
END VIRTUAL

VIRTUAL AT DI
	NextBlock	Bootes.Memory.BlockDescriptor
END VIRTUAL

VIRTUAL AT SI
	PrevBlock	Bootes.Memory.BlockDescriptor
END VIRTUAL


;====================================================================
; Init
; --------------------------------------------------------------
;	REMARKS
;		Inizializza la memora e le strutture di gestione necessarie.
;====================================================================
align 4
Bootes.Memory.Init:
	PUSH AX
	PUSH DI
	
	; --------------------------------------------------------------
	; Recupero il primo indirizzo utilizzabile
	; --------------------------------------------------------------
	MOV AX,Bootes.Stage2.End
	MOV [Bootes.Memory.Data.BlockListAddr],AX
	MOV [Bootes.Memory.Data.BlockListBytes],Block.Length*4
	MOV WORD [Bootes.Memory.Data.FreeBlockCount],2
	MOV WORD [Bootes.Memory.Data.UsedBlockCount],2

	; --------------------------------------------------------------
	; Azzero la memoria interessata
	; --------------------------------------------------------------
	PUSH AX
	MOV DI,AX
	XOR AX,AX
	MOV CX,(Block.Length*Bootes.Stage2.Info.MaxMemoryBlocks)/2
	REP STOSW
	POP AX
	
	; --------------------------------------------------------------
	; Inserisco il blocco per la lista di blocchi
	; --------------------------------------------------------------
	MOV [Bootes.Memory.Data.BlockUsedFirst],AX
	PUSH AX
	MOV DI,AX
	; AddrStart
	STOSW
	; AddrEnd
	ADD AX,(Block.Length*Bootes.Stage2.Info.MaxMemoryBlocks)-1
	STOSW
	; Size
	MOV AX,(Block.Length*Bootes.Stage2.Info.MaxMemoryBlocks)
	MOV [Bootes.Memory.Data.BlockListMaxBytes],AX
	STOSW
	; TaskUID
	MOV AX,1
	STOSW
	; Previous
	XOR AX,AX
	STOSW
	; Next
	LEA AX,[DI+2]
	STOSW
	; --------------------------------------------------------------
	; Inserisco il blocco di 512 byte per il chainloading
	; --------------------------------------------------------------
	; AddrStart
	MOV AX,Bootes.Stage1.Info.Origin
	STOSW
	; AddrEnd
	ADD AX,(512-1)
	STOSW
	; Size
	MOV AX,512
	STOSW
	; TaskUID
	MOV AX,1
	STOSW
	; Previous
	POP AX
	STOSW
	; Next
	XOR AX,AX
	STOSW
	
	; --------------------------------------------------------------
	; Inserisco il primo blocco per descrivere la memoria libera
	; --------------------------------------------------------------
	MOV [Bootes.Memory.Data.BlockFreeFirst],DI
	PUSH DI
	; AddrStart
	MOV AX,Bootes.Stage2.End+(Block.Length*Bootes.Stage2.Info.MaxMemoryBlocks)
	STOSW
	; AddrEnd
	ADD AX,(Bootes.Stage1.Info.Origin-Bootes.Stage2.End-(Block.Length*Bootes.Stage2.Info.MaxMemoryBlocks)-1)
	STOSW
	; Size
	MOV AX,Bootes.Stage1.Info.Origin-Bootes.Stage2.End-(Block.Length*Bootes.Stage2.Info.MaxMemoryBlocks)
	STOSW
	; TaskUID
	XOR AX,AX
	STOSW
	; Previous
	STOSW
	; Next
	LEA AX,[DI+2]
	STOSW
	; --------------------------------------------------------------
	; Secondo blocco di memoria libera dopo 0x7E00
	; --------------------------------------------------------------
	; AddrStart
	MOV AX,Bootes.Stage1.Info.Origin+512
	STOSW
	; AddrEnd
	ADD AX,0xFFFF-(Bootes.Stage1.Info.Origin+512)
	STOSW
	; Size
	MOV AX,0x10000-(Bootes.Stage1.Info.Origin+512)
	STOSW
	; TaskID
	XOR AX,AX
	STOSW
	; Previous
	POP AX
	STOSW
	; Next
	XOR AX,AX
	STOSW

	; --------------------------------------------------------------
	; Aggiungo il namespace
	; --------------------------------------------------------------
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Memory_Namespace

	; --------------------------------------------------------------
	; Rimappo l'INT 0x05 per l'istruzione BOUND.
	; --------------------------------------------------------------
;	CALL Bootes.Interrupt.Remap, 0x05, Bootes.Memory.BoundHandler, 0
	
	POP DI
	POP AX
RET


if 0
align 4
;====================================================================
; Handler per l'interrupt 0x05 chiamato dall'istruzione BOUND i,s nel
; caso in cui i non sia compreso tra gli indirizzi specificati da s.
; s punta a una zona di memoria contenente 2 WORD che indicano i 
; limiti per i.
;====================================================================
Bootes.Memory.BoundHandler:
	MOV DX,[GS:Block.AddrEnd]
	SUB DX,AX
	; Espando il blocco in questione
	CALL Bootes.Memory.ExpandBlock, BX, [GS:Block.Size]
	OR AX,AX
	JZ .Fail
	; Sposto il contenuto del nuovo stack
	CLD
	MOV SI,[GS:Block.AddrEnd]
	MOV CX,[GS:Block.Size]
	MOV BX,[GS:Bootes.Memory.Data.BlockCreatedLast]
	MOV DI,[GS:Block.AddrEnd]
	REP MOVSB
	STD
	; Imposto il risultato (nuovo stack pointer)
	MOV AX,[GS:Block.AddrEnd]
	SUB AX,DX
	JMP .Exit
	.Fail:
	; TODO: Far fallire il task corrente
	.Exit:
IRET
end if



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
	PUSH DI
	
	MOV AX,[Size]
	; --------------------------------------------------------------
	; Ricerco un Block libero con dimensione appropriata.
	; --------------------------------------------------------------
	MOV BX,[Bootes.Memory.Data.BlockFreeFirst]
	@@:
		OR BX,BX
		JZ .Error
		CMP AX,[Block.Size]
		JBE .Continue
		MOV BX,[Block.Next]
		JMP @b
	
	; --------------------------------------------------------------
	; Alloco il blocco di memoria
	; --------------------------------------------------------------
	.Continue:
	; Memoria libera residua
	MOV DX,[Block.Size]
	SUB DX,AX
	OR DX,DX
	JZ .eRemain
	.iRemain:
		; --------------------------------------------------------------
		; Verifico che sia possibile allocare un nuovo blocco
		; --------------------------------------------------------------
		MOV AX,[Bootes.Memory.Data.BlockListBytes]
		CMP AX,[Bootes.Memory.Data.BlockListMaxBytes]
		JB .xResize
		.iResize:
			; --------------------------------------------------------------
			; Realloco il doppio dello spazio per la lista
			; --------------------------------------------------------------
			; TODO: Funzione Expand
			JMP .Error
		.xResize:
		
		; --------------------------------------------------------------
		; Inserisco un nuovo blocco libero
		; --------------------------------------------------------------
		; Ricavo l'indirizzo nella lista al quale inserire un nuovo Block
		MOV AX,[Bootes.Memory.Data.BlockListAddr]
		ADD AX,[Bootes.Memory.Data.BlockListBytes]
		PUSH AX
		MOV DI,AX
		; Ricavo l'indirizzo del nuovo blocco di memoria libero
		MOV AX,[Size]
		ADD AX,[Block.AddrStart]
		; AddrStart
		STOSW
		; AddrEnd
		MOV AX,[Block.AddrEnd]
		STOSW
		; Size
		MOV AX,DX
		STOSW
		; TaskUID
		XOR AX,AX
		STOSW
		; Previous
		MOV AX,[Block.Previous]
		STOSW
		OR AX,AX
		POP AX
		JNZ @f
			; da inserire al posto di BlockFreeFirst
			MOV [Bootes.Memory.Data.BlockFreeFirst],AX
		@@:
		; Next
		MOV AX,[Block.Next]
		STOSW
		
		; Incremento i bytes della lista di block
		ADD WORD [Bootes.Memory.Data.BlockListBytes],Block.Length
		JMP .xRemain
	.eRemain:
		; --------------------------------------------------------------
		; Non c'è più memoria libera, sistemo la free list
		; --------------------------------------------------------------
		; Imposto i valori per il blocco libero precedente
		PUSH BX
		MOV AX,[Block.Next]
		MOV BX,[Block.Previous]
		OR BX,BX
		JNZ .ePrevNull
		.iPrevNull:
			MOV WORD [Bootes.Memory.Data.BlockFreeFirst],BX
			JMP .xPrevNull
		.ePrevNull:
			MOV [Block.Next],AX
		.xPrevNull:
		MOV AX,BX
		POP BX
		; Imposto i valori per il blocco libero successivo
		PUSH BX
		MOV BX,[Block.Next]
		OR BX,BX
		JZ @f
			MOV [Block.Previous],AX
		@@:
		POP BX
		; Diminuisco il conteggio di blocchi liberi
		DEC WORD [Bootes.Memory.Data.FreeBlockCount]
	.xRemain:
	; --------------------------------------------------------------
	; Inserisco il blocco libero tra quelli utilizzati
	; --------------------------------------------------------------
	; Imposto la dimensione
	MOV AX,[Size]
	MOV [Block.Size],AX
	; Imposto l'indirizzo di termine blocco
	DEC AX
	ADD AX,[Block.AddrStart]
	MOV [Block.AddrEnd],AX
	; Inserisco
	MOV WORD [Block.Previous],0
	MOV AX,[Bootes.Memory.Data.BlockUsedFirst]
	CMP AX,BX
	JE .eHaveNext
	.iHaveNext:
		MOV [Block.Next],AX
		JMP .xHaveNext
	.eHaveNext:
		MOV WORD [Block.Next],0
	.xHaveNext:
	MOV [Bootes.Memory.Data.BlockUsedFirst],BX
	; TaskID
	if Bootes.Stage2.Info.MultitaskSupport = 1
	CALL Bootes.Task.GetCurrentTaskUID
	else
	MOV AX,1
	end if
	MOV [Block.TaskUID],AX
	; Incremento il numero di blocchi utilizzati
	INC WORD [Bootes.Memory.Data.UsedBlockCount]
	
	; --------------------------------------------------------------
	; Preparo il risualtato
	; --------------------------------------------------------------
	MOV AX,[Block.AddrStart]
	MOV [Bootes.Memory.Data.BlockCreatedLast],BX
	
	JMP .Exit
	.Error:
	ErrorSetup AllocFail
	XOR AX,AX
	.Exit:
	POP DI
	POP DX
	POP BX
	
	UnlockScheduler
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



;====================================================================
; Espande la zona di memoria specificata della dimensione indicata.
; --------------------------------------------------------------
; PARAMS
;	Point	Indirizzo contenuto nella zona di memoria da espandere.
;	Size	Numero di byte da aggiungere alla zona di memoria.
; RETURN
;	Zero se non è stato possibile espandere la memoria, altrimenti
;	puntatore alla nuova zona di memoria espansa.
;====================================================================
Function Bootes.Memory.Expand, Point,Size
Begin
	LockScheduler
	
	PUSH BX
	PUSH CX
	
	MOV AX,[Point]
	OR AX,AX
	JNZ @f
		; Se non c'è un punto precedente alloco
		CALL Bootes.Memory.Alloc, [Size]
		JMP .Exit
	@@:
	MOV BX,[Bootes.Memory.Data.BlockUsedFirst]
	; --------------------------------------------------------------
	; Cerco il blocco da liberare
	; --------------------------------------------------------------
	.lFindFree:
		; Controllo se ho una struttura
		OR BX,BX
		JZ .Exit
		; Verifico se devo liberarla
		MOV CX,[Block.AddrStart]
		CMP AX,CX
		JAE .Possible
		MOV BX,[Block.Next]
		JMP .lFindFree
		.Possible:
		ADD CX,[Block.Size]
		CMP AX,CX
		JB .xFindFree
		MOV BX,[Block.Next]
		JMP .lFindFree
	.xFindFree:

	; --------------------------------------------------------------
	; Espando il blocco
	; --------------------------------------------------------------
	CALL Bootes.Memory.ExpandBlock, BX,[Size]
	
	.Exit:
	POP CX
	POP BX

	UnlockScheduler
Return



;====================================================================
; Espande il blocco di memoria specificato della dimensione indicata.
; --------------------------------------------------------------
; PARAMS
;	BAddr	Indirizzo del blocco da espandere.
;	Size	Numero di byte da aggiungere alla zona di memoria.
; RETURN
;	Zero se non è stato possibile espandere la memoria, altrimenti
;	puntatore alla nuova zona di memoria espansa.
;====================================================================
Function Bootes.Memory.ExpandBlock, BAddr,Size
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	; --------------------------------------------------------------
	; Verifico che il blocco sia usato
	; --------------------------------------------------------------
	MOV BX,[BAddr]
	CMP WORD [Block.TaskUID],0
	JE .Error
	
	; --------------------------------------------------------------
	; Cerco un blocco di memoria libero continuo a quello usato
	; --------------------------------------------------------------
	MOV AX,[Size]
	MOV DX,[Block.Size]
	MOV CX,[Block.AddrStart]
	ADD CX,DX
	MOV DI,[Bootes.Memory.Data.BlockFreeFirst]
	ADD DX,AX
	.lFindFree:
		; Controllo fine lista
		OR DI,DI
		JZ .eConsecutive
		; Controllo continuità
		CMP CX,[NextBlock.AddrStart]
		JNE @f
			CMP AX,[NextBlock.Size]
			JBE .iConsecutive
		@@:
		; Prossimo blocco
		MOV DI,[NextBlock.Next]
		JMP .lFindFree
	
	; --------------------------------------------------------------
	; Espando
	; --------------------------------------------------------------
	.iConsecutive:
		; Sistemo il blocco libero
		SUB WORD [NextBlock.Size],AX
		ADD WORD [NextBlock.AddrStart],AX
		CMP WORD [NextBlock.Size],0
		JNE @f
			CALL Bootes.Memory.DeleteBlock, DI
		@@:
		; Sistemo il blocco usato
		ADD WORD [Block.Size],AX
		ADD WORD [Block.AddrEnd],AX
		MOV AX,[Block.AddrStart]
		MOV [Bootes.Memory.Data.BlockCreatedLast],BX
		JMP .Exit
	.eConsecutive:
		; Alloco il nuovo blocco
		CALL Bootes.Memory.Alloc, DX
		OR AX,AX
		JZ .Exit
		; Copio il contenuto del vecchio blocco nel nuovo blocco
		MOV DI,AX
		MOV SI,[Block.AddrStart]
		MOV CX,[Block.Size]
		REP MOVSB
		; Elimino il vecchio blocco
		CALL Bootes.Memory.Free, BX
	.xConsecutive:
	
	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Libera zona di memoria che contiene l'indirizzo specificato
; --------------------------------------------------------------
; PARAMS
;	Point	Indirizzo contenuto nella zona di memoria da liberare.
;====================================================================
Function Bootes.Memory.Free, Point
Begin
	LockScheduler
	PUSH AX
	PUSH BX
	
	MOV AX,[Point]
	OR AX,AX
	JZ .Exit
	MOV BX,[Bootes.Memory.Data.BlockUsedFirst]
	; --------------------------------------------------------------
	; Cerco il blocco da liberare
	; --------------------------------------------------------------
	.lFindFree:
		; Controllo se ho una struttura
		OR BX,BX
		JZ .Exit
		; Verifico se devo liberarla
		CMP AX,[Block.AddrStart]
		JAE .Possible
		MOV BX,[Block.Next]
		JMP .lFindFree
		.Possible:
		CMP AX,[Block.AddrEnd]
		JBE .xFindFree
		MOV BX,[Block.Next]
		JMP .lFindFree
	.xFindFree:

	; --------------------------------------------------------------
	; Libero il blocco
	; --------------------------------------------------------------
	CALL Bootes.Memory.FreeBlock, BX
	
	.Exit:
	POP BX
	POP AX
	UnlockScheduler
Return



;====================================================================
; Libera il blocco attualmente usato specificato.
; --------------------------------------------------------------
; PARAMS
;	BAddr	Indirizzo del blocco da liberare.
;====================================================================
Function Bootes.Memory.FreeBlock, BAddr
Begin
	PUSH AX
	PUSH BX
	PUSH DI
	PUSH SI
	
	MOV BX,[BAddr]
	CMP WORD [Block.TaskUID],0
	JE .Exit
	
	; --------------------------------------------------------------
	; Sistemo i blocchi usati
	; --------------------------------------------------------------
	PUSH BX
	MOV SI,[Block.Previous]
	MOV DI,[Block.Next]
	OR SI,SI
	JZ .iNoPrev
	MOV BX,SI
	MOV [Block.Next],DI
	JMP .xNoPrev
	.iNoPrev:
		; Imposto il successivo blocco come il primo degli usati
		MOV [Bootes.Memory.Data.BlockUsedFirst],DI
	.xNoPrev:
	OR DI,DI
	JZ @f
	MOV BX,DI
	MOV [Block.Previous],SI
	@@:
	POP BX
	
	; --------------------------------------------------------------
	; Cerco la posizione nella FreeList nella quale inserire
	; --------------------------------------------------------------
	MOV AX,[Block.AddrStart]
	PUSH BX
	MOV BX,[Bootes.Memory.Data.BlockFreeFirst]
	XOR SI,SI
	.lFindNext:
		; Controllo fine lista
		OR BX,BX
		JZ .xFindNext
		; Imposto il previous
		MOV SI,[Block.Previous]
		; Controllo l'indirizzo
		CMP AX,[Block.AddrStart]
		JB .xFindNext
		; Prossimo blocco libero
		MOV BX,[Block.Next]
		JMP .lFindNext
	.xFindNext:
	MOV DI,BX
	POP BX
	; Eventuale impostazione di BlockFreeFirst
	OR SI,SI
	JNZ @f
		MOV [Bootes.Memory.Data.BlockFreeFirst],BX
	@@:
	
	; --------------------------------------------------------------
	; Inserisco il blocco trovato tra quelli liberi
	; --------------------------------------------------------------
	MOV [Block.Previous],SI
	MOV [Block.Next],DI
	MOV WORD [Block.TaskUID],0
	INC WORD [Bootes.Memory.Data.FreeBlockCount]
	DEC WORD [Bootes.Memory.Data.UsedBlockCount]
	
	; --------------------------------------------------------------
	; Elimino il blocco se è continuo con i suoi adiacenti
	; --------------------------------------------------------------
	; Prima controllo se è da eliminare il successivo blocco
	OR DI,DI
	JZ .xCompactNext
	.iCompactNext:
		; Controllo se i blocchi sono consecutivi
		MOV AX,[Block.AddrEnd]
		INC AX
		CMP AX,[NextBlock.AddrStart]
		JNE .xCompactNext
		; Compatto
		MOV AX,[NextBlock.AddrEnd]
		MOV [Block.AddrEnd],AX
		MOV AX,[NextBlock.Size]
		ADD [Block.Size],AX
		MOV AX,[NextBlock.Next]
		MOV [Block.Next],AX
		; Libero uno spazio dalla lista
		CALL Bootes.Memory.DeleteBlock, DI
		DEC WORD [Bootes.Memory.Data.FreeBlockCount]
	.xCompactNext:
	; Controllo se devo compattare nel blocco precedente
	OR SI,SI
	JZ .xCompactPrevious
	.iCompactPrevious:
		; Controllo se i blocchi sono consecutivi
		MOV AX,[PrevBlock.AddrEnd]
		INC AX
		CMP AX,[Block.AddrStart]
		JNE .xCompactPrevious
		; Compatto
		MOV AX,[Block.AddrEnd]
		MOV [PrevBlock.AddrEnd],AX
		MOV AX,[Block.Size]
		ADD [PrevBlock.Size],AX
		MOV AX,[Block.Next]
		MOV [PrevBlock.Next],AX
		; Libero uno spazio dalla lista
		CALL Bootes.Memory.DeleteBlock, BX
		DEC WORD [Bootes.Memory.Data.FreeBlockCount]
	.xCompactPrevious:
	
	.Exit:
	POP SI
	POP DI
	POP BX
	POP AX
Return



;====================================================================
; Cancella il blocco specificato dalla lista.
; --------------------------------------------------------------
; PARAMS
;	BAddr	Indirizzo del blocco da cancellare.
;====================================================================
Function Bootes.Memory.DeleteBlock, BAddr
Begin
	PUSH CX
	PUSH SI
	PUSH DI

	; --------------------------------------------------------------
	; Ricavo il blocco sorgente
	; --------------------------------------------------------------
	MOV SI,[Bootes.Memory.Data.BlockListAddr]
	ADD SI,[Bootes.Memory.Data.BlockListBytes]
	MOV CX,Block.Length
	SUB SI,CX
	SHR CX,1
	PUSH SI
	
	; --------------------------------------------------------------
	; Sposto il blocco
	; --------------------------------------------------------------
	MOV DI,[BAddr]
	PUSH DI
	REP MOVSW
	
	
	; --------------------------------------------------------------
	; Sistemo i riferimenti
	; --------------------------------------------------------------
	POP CX
	MOV SI,CX
	MOV DI,[PrevBlock.Next]
	MOV SI,[PrevBlock.Previous]
	OR SI,SI
	JZ @f
		MOV [PrevBlock.Next],CX
	@@:
	OR DI,DI
	JZ @f
		MOV [NextBlock.Previous],CX
	@@:
	; Sistemo le First references
	POP SI
	CMP [Bootes.Memory.Data.BlockFreeFirst],SI
	JNE @f
		MOV [Bootes.Memory.Data.BlockFreeFirst],CX
		JMP .Exit
	@@:
	CMP [Bootes.Memory.Data.BlockUsedFirst],SI
	JNZ @f
		MOV [Bootes.Memory.Data.BlockUsedFirst],CX
	@@:
	
	.Exit:
	; Diminuisco la dimensione della lista
	SUB WORD [Bootes.Memory.Data.BlockListBytes],Block.Length
	POP DI
	POP SI
	POP CX
Return


;====================================================================
; Colleziona le zone marcate come usate il quale task non è
; più attivo.
;====================================================================
align 4
Bootes.Memory.Collect:
if Bootes.Stage2.Info.MultitaskSupport = 1
	CLI
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	
	; --------------------------------------------------------------
	; Colleziono i Block di memoria non usati
	; --------------------------------------------------------------
	MOV BX,[Bootes.Memory.Data.BlockUsedFirst]
	.lCheck:
		OR BX,BX
		JZ .Exit
		; Prossimo blocco
		MOV DX,[Block.Next]
		; Verifico la presenza del task tra quelli attivi
		MOV SI,Bootes.Task.Data.TasksUID
		MOV CX,[Bootes.Task.Data.Count]
		MOV AX,[Block.TaskUID]
		@@:
		CMP AX,[SI]
		JE .Continue
		ADD SI,2
		LOOP @b
		
		; Libero il blocco
		CALL Bootes.Memory.FreeBlock, BX 
		DEC WORD [Bootes.Memory.Data.UsedBlockCount]
	
		; Prossimo Block usato
		.Continue:
		MOV BX,DX
		JMP .lCheck
	
	.Exit:
	POP SI
	POP DX
	POP CX
	POP BX
	POP AX
	STI
end if
RET



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
	; --------------------------------------------------------------
	; Block list
	; --------------------------------------------------------------
	; Puntatori
	.BlockUsedFirst		DW 0	; Indirizzo alla prima struttura Block utilizzata.
	.BlockFreeFirst		DW 0	; Indirizzo alla prima struttura Block libera.
	.BlockCreatedLast	DW ?	; Indirizzo dell'ultima struttura Block creata.
	; Informazioni
	.BlockListAddr		DW ?	; Indirizzo di memoria della lista.
	.BlockListBytes		DW ?	; Numero di strutture block moltiplicato per la loro dimensione.
	.BlockListMaxBytes	DW ? 	; Numero massimo di byte allocabili nella lista.
	.FreeBlockCount		DW 0	; Conteggio blocchi liberi
	.UsedBlockCount		DW 0	; Conteggio blocchi usati
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