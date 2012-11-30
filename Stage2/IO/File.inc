;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.IO.File
;
; Gestore della lettura dei file da disco.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

SysNamespace Bootes.IO.File_Namespace,"IO.File"
	Export Bootes.IO.File.Read,1, "Read", Type.String



;====================================================================
; Carica il file specificato in memoria.
; --------------------------------------------------------------
; PARAMS
;	FilePath	Puntatore alla stringa terminata da 0 contenente
;				il percorso assoluto del file da caricare.
; RETURN
;	Puntatore ai dati caricati oppure 0 se è avvenuto un errore.
;====================================================================	
Function Bootes.IO.File.Read, FilePath
Begin
	CALL Bootes.Memory.Alloc, Bootes.Stage2.Info.MaxFileSize
	OR AX,AX
	JZ .Error
	PUSH AX
	CALL Bootes.IO.File.ExtendedRead, [FilePath],0,AX,Bootes.Stage2.Info.MaxFileSize
	OR AX,AX
	JNZ @f
		POP AX
		CALL Bootes.Memory.Free, AX
		MOV AX,0
		JMP .Error
	@@:
	ADD SP,2
	.Error:
Return



;====================================================================
; Carica il file specificato in memoria.
; --------------------------------------------------------------
; PARAMS
;	FilePath	Puntatore alla stringa terminata da 0 contenente
;				il percorso assoluto del file da caricare.
;	Buffer		Puntatore al buffer di memoria sul quale scrivere
;				i dati letti.
;	Size		Numero di byte massimi da leggere.
; RETURN
;	Puntatore ai dati caricati oppure 0 se è avvenuto un errore.
;====================================================================
Function Bootes.IO.File.ExtendedRead, FilePath,BufferHi,BufferLo,Size
Begin
;	LockScheduler
	
	PUSH DI
	PUSH SI
	PUSH DX
	PUSH CX
	PUSH BX
	; --------------------------------------------------------------
	; Pre-elaborazione stringa
	; --------------------------------------------------------------
	MOV DI,[FilePath]
	OR DI,DI
	JZ .Error
	CALL Bootes.String.Split, DI,Bootes.IO.File.Data.Slash
	CMP AX,1
	JBE .Error
	MOV CX,AX
	DEC CX
	; --------------------------------------------------------------
	; Ricavo la partizione
	; --------------------------------------------------------------
	; SI è mappato come [Disk.inc]CurrentPartition
	CALL Bootes.String.Trim, DI,Bootes.IO.File.Data.Brakes
	MOV DI,AX
	CALL Bootes.Devices.Disk.GetPartition, DI
	OR AX,AX
	JZ .Error
	MOV SI,AX
	; --------------------------------------------------------------
	; Controllo se la partizione ha un driver di filesystem
	; --------------------------------------------------------------
	; BX è mappato come [FileSystems.inc]CurrentFSHandler
	MOV BX,[CurrentPartition.FSHandler]
	OR BX,BX
	JZ .Error
	; --------------------------------------------------------------
	; Controllo se la partizione è montata
	; --------------------------------------------------------------
	CMP BYTE [CurrentPartition.Mounted],1
	JE .Mounted
		; Monto la partizione se possibile
		MOV AX,[CurrentFSHandler.MountDelegate]
		OR AX,AX
		JZ .Error
		CALL AX,SI
		OR AX,AX
		JZ .Exit
	.Mounted:
	; --------------------------------------------------------------
	; Avanzo nelle directory
	; --------------------------------------------------------------
	MOV DX,[CurrentFSHandler.DirDelegate]
	OR DX,DX
	JZ .Error
	CALL DX,SI,0	; Root directory
	.lDir:
		CALL Bootes.String.GetNext, DI
		MOV DI,AX
		CMP CX,1
		JBE .xDir
		CALL DX,SI,DI
		OR AX,AX
		JZ .Exit
		DEC CX
		JMP .lDir
	.xDir:
	; --------------------------------------------------------------
	; Carico il file
	; --------------------------------------------------------------
	MOV DX,[CurrentFSHandler.ReadDelegate]
	OR DX,DX
	JZ .Error
	CALL DX,SI,DI,[BufferHi],[BufferLo],[Size]
	JMP .Exit
	
	.Error:
	XOR AX,AX
	.Exit:
	POP BX
	POP CX
	POP DX
	POP SI
	POP DI
	
;	UnlockScheduler
Return

;====================================================================
; Data
;====================================================================
Bootes.IO.File.Data:
	.Brakes				DB "()[]",0
	.Slash				DB "\/",0