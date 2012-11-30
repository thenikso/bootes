;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.IO.FileSystems
;
; Gestore di file systems.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

; --------------------------------------------------------------
; Delegates Infos
; --------------------------------------------------------------
; delegate bool CheckD(byte[] Header);
;	Una implementazione di questa funzione deve controllare se l'header fornito descrive il filesystem selezionato.
;	Ritorna true se l'header corrisponde al filesystem descritto altrimenti false.
;
; delegate WORD MountD(Bootes.Devices.Disk.Partition Partition);
;	Una implementazione di questa funzione deve svolgere le operazioni necessarie per montare il filesystem in modo
;	da poter leggere il contenuto dello stesso. Ritorna 0 nel caso non sia stato possibile completare l'operazione.
;
; delegate string DirD(Bootes.Devices.Disk.Partition Partition, string DirectoryName);
;	Una implementazione di questa funzione deve caricare in memoria le informazioni necessarie della directory
;	specificata (nome singolo di una cartella, non percorso). Se la stringa è nulla carica la root directory.
;	Restituisce una stringa contenente i nomi di tutti i file contenuti nella directory stessa.
;
; delegate byte[] ReadD(Bootes.Devices.Disk.Partition Partition, string FileName, byte[] Buffer, WORD Size);
;	Una implementazione di questa funzione deve caricare il file selezionato nel buffer dati fornito. Il nome del file
;	non è un percorso e si riferisce ad un file contenuto nella cartella raggiunta con l'ultima chiamata alla funzione 
;	di tipo DirD. Restituisce un puntatore ai dati utili del file letto (Su EAX).
;
; delegate WORD WriteD(Bootes.Devices.Disk.Partition Partition, string FileName, byte[] Buffer, WORD Size);
;	Attualmente non necessita di implementazione perchè non è attualmente prevista la funzionalità di scrittura.

; --------------------------------------------------------------
; Strutture e virtuals
; --------------------------------------------------------------
STRUC Bootes.IO.FileSystems.FSHandler ID,Name,CheckD,MountD,DirD,ReadD,WriteD
{
	.Previous		DW 0
	.Next			DW 0
	
	.ID				DW ID
	
	.CheckDelegate	DW CheckD
	.MountDelegate	DW MountD
	.DirDelegate	DW DirD
	.ReadDelegate	DW ReadD
	.WriteDelegate	DW WriteD
	; TODO: inserire altri delegate (?)
	
	.Name		DB Name,0
}

VIRTUAL AT BX
	CurrentFSHandler		Bootes.IO.FileSystems.FSHandler ?,?,?,?,?,?,?
END VIRTUAL

;====================================================================
; Inizlizza i file systems supportati
;====================================================================
Bootes.IO.FileSystems.Init:
	CALL Bootes.IO.FileSystems.FAT.Init
	; Aggiungere altre inizilizzazioni QUI.
	RET



;====================================================================
; Ricava l'handler del file system dall'ID specificato.
; --------------------------------------------------------------
; PARAMS
;	ID		Numero identificativo della partizione.
; RETURN
;	Puntatore all'handler del file system desiderato oppure zero
;	se è avvenuto un errore.
;====================================================================
Function Bootes.IO.FileSystems.GetFSHandlerFromID, ID
Begin
	PUSH BX
	PUSH SI
	
	; --------------------------------------------------------------
	; Recupero l'handler
	; --------------------------------------------------------------
	MOV BX,[Bootes.IO.FileSystems.Data.FSList]
	.lCheckFS:
		OR BX,BX
		JZ .Error
		; Recupero la funzione check del FS corrente
		MOV AX,[CurrentFSHandler.ID]
		CMP AX,[ID]
		JNE .Continue
		; FS trovato
		MOV AX,BX
		JMP .Exit
		.Continue:
		MOV BX,[CurrentFSHandler.Next]
		JMP .lCheckFS
	
	.Error:
	XOR AX,AX
	.Exit:
	POP SI
	POP BX
Return



;====================================================================
; Ricava l'handler del file system dall'header specificato.
; --------------------------------------------------------------
; PARAMS
;	Partition	Puntatore alla partizione dalla quale ricavare 
;				l'header per determinare il file system.
; RETURN
;	Puntatore all'handler del file system desiderato oppure zero
;	se è avvenuto un errore.
;====================================================================
Function Bootes.IO.FileSystems.GetFSHandlerFromHeader, Partition
Begin
	PUSH BX
	PUSH DX
	PUSH SI
	
	; --------------------------------------------------------------
	; Recupero l'header della partizione
	; --------------------------------------------------------------
	MOV SI,[Partition]
	OR SI,SI
	JZ .Error
	; TODO: Se un disco a settori >512byte ci sono problemi ad usare come buffer lo spazio dello stage1. Controllare anche nel memorymanager(?)
	CALL Bootes.Memory.Alloc, 512
	OR AX,AX
	JZ .Exit
	MOV DX,AX
	CALL Bootes.Devices.Disk.Read, [CurrentPartition.Disk],0,AX,[CurrentPartition.Start+2],[CurrentPartition.Start],1
	OR AX,AX
	JZ .Error
	
	; --------------------------------------------------------------
	; Determino il file system
	; --------------------------------------------------------------
	MOV BX,[Bootes.IO.FileSystems.Data.FSList]
	.lCheckFS:
		OR BX,BX
		JZ .Error
		; Recupero la funzione check del FS corrente
		MOV AX,[CurrentFSHandler.CheckDelegate]
		OR AX,AX
		JZ .Continue
		CALL AX,DX
		OR AX,AX
		JZ .Continue
		; FS trovato
		MOV AX,BX
		JMP .Exit
		.Continue:
		MOV BX,[CurrentFSHandler.Next]
		JMP .lCheckFS
	
	.Error:
	XOR AX,AX
	.Exit:
	CALL Bootes.Memory.Free, DX
	POP SI
	POP DX
	POP BX
Return



;====================================================================
; Aggiunge la FSEntry specificata alla lista.
; --------------------------------------------------------------
; PARAMS
;	FSHandler		Puntatore alla FSHandler da inserire.
;====================================================================
Function Bootes.IO.FileSystems.AddFS, FSHandler
Begin
	PUSH AX
	PUSH BX
	
	MOV AX,[FSHandler]
	MOV BX,AX
	XCHG BX,[Bootes.IO.FileSystems.Data.FSList]
	MOV [CurrentFSHandler.Previous],AX
	XCHG BX,AX
	MOV [CurrentFSHandler.Next],AX
	MOV WORD [CurrentFSHandler.Previous],0
	
	POP BX
	POP AX
Return



;====================================================================
; Ricava la stringa che rappresenta l'FSHandler specificato.
; --------------------------------------------------------------
; PARAMS
;	FSHandler		Puntatore alla FSHandler da utilizzare.
;
; RETURN
;	Puntatore ad una stringa terminata da 0 contenente il nome
;	dell'FSHandler specificato. Oppure una stringa generica se
;	non è stato possibile ricavare tale nome.
;====================================================================
Function Bootes.IO.FileSystems.GetFSHanlerName, FSHandler
Begin
	PUSH BX
	
	MOV BX,[FSHandler]
	OR BX,BX
	JZ .Error
	LEA AX,[CurrentFSHandler.Name]
	JMP .Exit
	
	.Error:
	MOV AX,Bootes.IO.FileSystems.Data.UnknowName
	.Exit:
	POP BX
Return

;====================================================================
; Data
;====================================================================
align 2
Bootes.IO.FileSystems.Data:
	.FSList			DW 0	; Punatore al primo elemento di una lista di FSEntry
	.UnknowName		DB "Unknow",0


;====================================================================
; Implementazioni file systems
;====================================================================
include "FAT.inc"
; Aggiungere altre implementazioni di file system QUI.