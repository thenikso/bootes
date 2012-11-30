;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Devices.Disk
;	using Bootes.IO.FileSystems
;
; Gestore di dischi e partizioni.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2


; --------------------------------------------------------------
; Funzioni esportate
; --------------------------------------------------------------
SysNamespace Bootes.Devices.Disk_Namespace, "Devices.Disk"
	Export Bootes.Devices.Disk.InfoDisk,0, "InfoDisk", Type.Null
	Export Bootes.Devices.Disk.RenamePartition,2,"RenamePartition", Type.Null, Type.String, Type.String



; --------------------------------------------------------------
; Strutture
; --------------------------------------------------------------
STRUC Bootes.Devices.Disk.Disk
{
	.Start:
	; Diskinfo
	.DiskID			DW ?	; Numero disco in formato INT 0x13.
	.FirstPartition	DW ?	; Prima partizione del disco.
	.PartitionCount	DW ?	; Numero di partizioni del disco (oppure -1 se non ha partizioni).

	; Delegates
	.ReadDelegate	DW ?	; Puntatore alla funzione di lettura.
	.WriteDelegate	DW ?	; Puntatore alla funzione di scrittura.
	
	; Parametri del disco
	.Parameters:
	.BufferSize		DW ?
	.Flags			DW ?
	.Cylinders		DW ?,?	; Geometria
	.Heads			DW ?,?
	.Sectors		DW ?,?
	.TotalSectors	DW ?,?,?,?
	.BytesPerSector	DW ?
	; v2.0+
	.EDD			DD ?
	; v3.0+
	.PathInfoSign	DW 0xBEDD
	.PathInfoSize	DB ?
					RB 3
	.HostBus		DB "PCI",0
	.Interface		DB "ATA",0,0,0,0,0
	.InterfacePath:
		.InterfacePath.PCIBus		DB ?
		.InterfacePath.PCIDevice	DB ?
		.InterfacePath.PCIFunction	DB ?
									RB 5
	.DevicePath		RB 8
					DB ?
	.CheckSum		DB ?
					RB 16
	
	.Size			= $-.Start
	.ParametersSize	= $-.Parameters
}

STRUC Bootes.Devices.Disk.Partition
{
	.Previous		DW ?	; Partizione precedente nello stesso disco.
	.Next			DW ?	; Prossima partizione nello stesso disco.
	;
	.Disk			DW ?	; Puntatore alla struttura Disk contenente la partizione.
	.Index			DW ?	; Indice nella tabella delle partizioni:
							;	-1 se è una partizione fittizia.
	;
	.Start			DW ?,?	; Settore di inizio (LBA)
	.Length			DW ?,?	; Numero di settori
	;
	.Type			DB ?
	.Name			RB .MaxNameChar	; Nome di riconoscimento della partizione terminato da 0
	; File System Handler
	.Mounted		DB 1
	.FSHandler		DW ?
	.FSInfo			DW ?	; Puntatore a una struttura definita dal file system usato.
	
	.Size			= $-.Previous
	.MaxNameChar	= 10
}

STRUC Bootes.Devices.Disk.PartitionEntry
{
	.Flag			DB ?	; 0x80 - Partizione attiva, 0x00 - Partizione non attiva
	
	.Start.Head		DB ?
	.Start.Sector	DB ?	; Contato da 1
	.Start.Cylinder	DB ?

	.Type			DB ?
	
	.End.Head		DB ?
	.End.Sector		DB ?
	.End.Cylinder	DB ?
	
	.Start			DW 0,0	; Settore di inizio in base 0 (LBA)
	.Length			DW 0,0	; Numero di settori per la partizione
	
	.Size			= $-.Flag
}


VIRTUAL AT DI
	CurrentDisk			Bootes.Devices.Disk.Disk
END VIRTUAL

VIRTUAL AT SI
	CurrentPartition	Bootes.Devices.Disk.Partition
END VIRTUAL

VIRTUAL AT BX
	CurrentPEntry		Bootes.Devices.Disk.PartitionEntry
END VIRTUAL


;====================================================================
; Inizializza i dischi presenti
;====================================================================
align 4
Bootes.Devices.Disk.Init:
	LockScheduler
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	
	; Reset del sistema dischi
	XOR AX,AX
	XOR DX,DX
	INT 0x13
	
	; --------------------------------------------------------------
	; Controllo il numero di dischi disponibili
	; --------------------------------------------------------------
	XOR BX,BX
	; Controllo il numero di floppy
	.lCountFloppy:
		; Ricavo il tipo del disco
		MOV AH,0x15
		INT 0x13
		JC .xCountFloppy
		; Vedo se esiste
		TEST AH,0x03
		JZ .xCountFloppy
		; Prossimo disco
		INC BL
		CMP BL,2
		MOV DL,BL
		JNE .lCountFloppy
	.xCountFloppy:
	;DEC BL
	XCHG BH,BL
	; Controllo il numero di hard disk
	XOR DX,DX
	.lCountHD:
		OR DL,0x80
		; Ricavo il tipo del disco
		MOV AH,0x15
		INT 0x13
		JC .xCountHD
		; Vedo se esiste
		TEST AH,0x03
		JZ .xCountHD
		; Prossimo disco
		INC BL
		CMP BL,0xF
		MOV DL,BL
		JNE .lCountHD
	.xCountHD:
	; Salvo le informazioni
	MOV [Bootes.Devices.Disk.Data.FloppyCount],BH
	MOV [Bootes.Devices.Disk.Data.HDCount],BL

	; --------------------------------------------------------------
	; Alloco la memoria iniziale
	; --------------------------------------------------------------
	; Per i dischi
	MOV CX,CurrentDisk.Size
	MOVZX AX,BL
	ADD AL,BH
	MOV [Bootes.Devices.Disk.Data.DiskArray.FreeBlocks],AX
	MUL CX
	CALL Bootes.Memory.Alloc, AX
	OR AX,AX
	JZ .Error
	MOV [Bootes.Devices.Disk.Data.DiskArray],AX
	; Per le partizioni
	MOV CX,CurrentPartition.Size
	MOV AX,[Bootes.Devices.Disk.Data.DiskArray.FreeBlocks]
	MOV [Bootes.Devices.Disk.Data.PartitionArray.FreeBlocks],AX
	MUL CX
	CALL Bootes.Memory.Alloc, AX
	OR AX,AX
	JNZ @f
		CALL Bootes.Memory.Free, [Bootes.Devices.Disk.Data.DiskArray]
		JMP .Error
	@@:
	MOV [Bootes.Devices.Disk.Data.PartitionArray],AX

	; --------------------------------------------------------------
	; Monto i floppy
	; --------------------------------------------------------------
	MOVZX CX,BH
	OR CX,CX
	JZ .xMountFloppy
	XOR DX,DX
	.lMountFloppy:
		CALL Bootes.Devices.Disk.Mount, DX
		OR AX,AX
		JZ .xMountFloppy
		INC DX
		LOOP .lMountFloppy
	.xMountFloppy:

	; --------------------------------------------------------------
	; Monto gli hard disk
	; --------------------------------------------------------------
	MOVZX CX,BL
	OR CX,CX
	JZ .xMountHD
	MOV DX,0x80
	.lMountHD:
		CALL Bootes.Devices.Disk.Mount, DX
		OR AX,AX
		JZ .xMountHD
		INC DX
		LOOP .lMountHD
	.xMountHD:

	; --------------------------------------------------------------
	; Imposto il namespace
	; --------------------------------------------------------------
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Devices.Disk_Namespace
	
	JMP .Exit
	.Error:
	ErrorSetup DiskMountError
	.Exit:
	POP DX
	POP CX
	POP BX
	POP AX
	UnlockScheduler
RET



;====================================================================
; Monta il disco e lo rende disponibile per la lettura con tutte le 
; sue partizioni.
; --------------------------------------------------------------
; PARAMS
;	DiskID	Identificativo del disco da montare.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Devices.Disk.Mount, DiskID
Begin
	LockScheduler
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	; TODO: Verificare che il disco non sia già stato montato e che DiskID sia valido
	; --------------------------------------------------------------
	; Alloco lo spazio per la nuova struttura Disk
	; --------------------------------------------------------------
	CMP [Bootes.Devices.Disk.Data.DiskArray.FreeBlocks],0
	JNE .eExpand
	.iExpand:
		CALL Bootes.Memory.Expand, [Bootes.Devices.Disk.Data.DiskArray],CurrentDisk.Size
		OR AX,AX
		JZ .Exit
		MOV [Bootes.Devices.Disk.Data.DiskArray],AX
		JMP .xExpand
	.eExpand:
		DEC WORD [Bootes.Devices.Disk.Data.DiskArray.FreeBlocks]
		MOV AX,[Bootes.Devices.Disk.Data.DiskArray]
	.xExpand:
	MOV DI,[Bootes.Devices.Disk.Data.DiskArray.Size]
	ADD DI,AX
	ADD WORD [Bootes.Devices.Disk.Data.DiskArray.Size],CurrentDisk.Size
	
	; --------------------------------------------------------------
	; Parametri comuni del disco
	; --------------------------------------------------------------
	; Tipo del disco
	MOV DX,[DiskID]
	; Imposto l'ID del disco
	MOV [CurrentDisk.DiskID],DX
	; Imposto la prima partizione
	MOV WORD [CurrentDisk.FirstPartition],0
	MOV WORD [CurrentDisk.PartitionCount],0
	
	; --------------------------------------------------------------
	; Testo il tipo del disco
	; --------------------------------------------------------------
	TEST DL,0x80
	JZ .eHardDisk
	.iHardDisk:
		; Verifico che LBA sia supportato
		PUSH DX
		MOV AX,0x4100
		MOV BX,0x55AA
		INT 0x13
		POP DX
		JC .eHDLBAMode
		CMP BX,0xAA55
		JNE .eHDLBAMode
		; Verifico che INT 0x13 AH=0x42 sia supportato
		AND CX,1
		JZ .eHDLBAMode
		.iHDLBAMode:
			; Tento una lettura
			MOV WORD [Bootes.Devices.Disk.Data.DiskAddressPacket.Blocks],1
			MOV WORD [Bootes.Devices.Disk.Data.DiskAddressPacket.Buffer],Bootes.Stage1.Info.Origin
			MOV WORD [Bootes.Devices.Disk.Data.DiskAddressPacket.Start1],0
			MOV WORD [Bootes.Devices.Disk.Data.DiskAddressPacket.Start2],0
			MOV SI,Bootes.Devices.Disk.Data.DiskAddressPacket
			MOV AX,0x4200
			INT 0x13
			JC .eHDLBAMode
			; Imposto i delegates
			MOV WORD [CurrentDisk.ReadDelegate],Bootes.Devices.Disk.ReadLBA
			MOV WORD [CurrentDisk.WriteDelegate],Bootes.Devices.Disk.WriteLBA
			; Azzero i parametri
			PUSH DI
			LEA DI,[CurrentDisk.Parameters]
			MOV CX,(CurrentDisk.ParametersSize/2)
			MOV AX,0
			REP STOSW
			POP DI
			; Recupero i parametri del disco
			MOV AH,0x48
			LEA SI,[CurrentDisk.Parameters]
			INT 0x13
			JMP .xHDLBAMode
		.eHDLBAMode:
			; Provo a determinare con l'INT 0x13 ah=0x08
			PUSH ES
			PUSH DI
			MOV AH,0x08
			INT 0x13
			POP DI
			POP ES
			JC .Error
			; Heads
			MOVZX AX,DH
			INC AX
			MOV [CurrentDisk.Heads],AX
			; Cylinders
			MOV AX,CX
			SHL AX,2
			MOV AL,CH
			AND AH,011b
			INC AX
			MOV WORD [CurrentDisk.Cylinders],AX
			; Sectors
			AND CX,00111111b
			MOV WORD [CurrentDisk.Sectors],CX
			; Numero totale di settori
			XOR DX,DX
			MUL WORD [CurrentDisk.Heads]
			XOR DX,DX
			MUL CX
			MOV WORD [CurrentDisk.TotalSectors],AX
			MOV WORD [CurrentDisk.TotalSectors+2],DX
			; Imposto i delegates
			MOV WORD [CurrentDisk.ReadDelegate],Bootes.Devices.Disk.ReadCHS
			MOV WORD [CurrentDisk.WriteDelegate],Bootes.Devices.Disk.WriteCHS
		.xHDLBAMode:
		; Inizializzo le partizioni
		CALL Bootes.Devices.Disk.MountPartitions, DI,0,0
		OR AX,AX
		JNZ @f
			; Imposto la partizione fittizia
			MOV WORD [CurrentDisk.PartitionCount], -1
			CALL Bootes.Devices.Disk.AddPartition, DI,0,0,[CurrentDisk.TotalSectors+2],[CurrentDisk.TotalSectors],0,0
		@@:
		; Incremento il numero di hardDisk
		INC BYTE [Bootes.Devices.Disk.Data.HDCount]
		JMP .xHardDisk
	.eHardDisk:
		; Provo a determinare con l'INT 0x13 ah=0x08
		PUSH ES
		PUSH DI
		MOV AH,0x08
		INT 0x13
		POP DI
		POP ES
		JNC .FloppySaveProbe
		; Se non ha avuto successo procedo con un test diretto
		.FloppyProbe:
			MOV SI,Bootes.Devices.Disk.Data.FloppyProbeValues
			MOV CL,0
			@@:
			; Resetta il floppy
			MOV AX,0
			INT 0x13
			; Prossima ProbeValue
			MOV CL,[SI]
			INC SI
			CMP CL,0
			JE .Error
			; Tento una lettura
			MOV DX,[DiskID]
			MOV BX,Bootes.Stage1.Info.Origin
			MOV AX,0x0201
			MOV CH,0
			MOV DH,0
			INT 0x13
			JC @b
			MOV DH,1
		.FloppySaveProbe:
		; Heads
		MOVZX AX,DH
		INC AX
		MOV [CurrentDisk.Heads],AX
		; Cylinders
		MOV AX,CX
		SHL AX,2
		MOV AL,CH
		AND AH,011b
		INC AX
		MOV WORD [CurrentDisk.Cylinders],AX
		; Sectors
		AND CX,00111111b
		MOV WORD [CurrentDisk.Sectors],CX
		; Numero totale di settori
		XOR DX,DX
		MUL WORD [CurrentDisk.Heads]
		XOR DX,DX
		MUL CX
		MOV WORD [CurrentDisk.TotalSectors],AX
		MOV WORD [CurrentDisk.TotalSectors+2],DX
		; Imposto i delegates
		MOV WORD [CurrentDisk.ReadDelegate],Bootes.Devices.Disk.ReadCHS
		MOV WORD [CurrentDisk.WriteDelegate],Bootes.Devices.Disk.WriteCHS
		; Imposto la partizione fittizia
		MOV WORD [CurrentDisk.PartitionCount],-1
		CALL Bootes.Devices.Disk.AddPartition, DI,0,0,[CurrentDisk.TotalSectors+2],[CurrentDisk.TotalSectors],[Bootes.IO.FileSystems.FAT12.ID],0
		; Incremento il numero di floppy
		INC BYTE [Bootes.Devices.Disk.Data.FloppyCount]
	.xHardDisk:
	; Incremento il numero totale di dischi
	INC WORD [Bootes.Devices.Disk.Data.DiskCount]
	
	JMP .Exit
	.Error:
	INC WORD [Bootes.Devices.Disk.Data.DiskArray.FreeBlocks]
	SUB WORD [Bootes.Devices.Disk.Data.DiskArray.Size],CurrentDisk.Size
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
	UnlockScheduler
Return



;====================================================================
; Monta le partizioni del disco specificato.
; --------------------------------------------------------------
; PARAMS
;	Disk		Struttura Disk a cui montare le partizioni.
;	TableAddr	Indirizzo LBA della tabella di partizioni da montare.
; RETURN
;	Il numero di partizioni montate oppure zero se è avvenuto un 
;	errore.
;====================================================================
Function Bootes.Devices.Disk.MountPartitions, Disk,TableSectorHi,TableSectorLo
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH DI
	PUSH SI

	MOV DI,[Disk]
	XOR DX,DX
	; --------------------------------------------------------------
	; Verifico che il disco sia un HD
	; --------------------------------------------------------------
	TEST WORD [CurrentDisk.DiskID],0x80
	JZ .Error

	; --------------------------------------------------------------
	; Leggo la tabella delle partizioni
	; --------------------------------------------------------------
	MOV BX,Bootes.Stage1.Info.Origin
	CALL Bootes.Devices.Disk.Read,DI,0,BX,[TableSectorHi],[TableSectorLo],1
	OR AX,AX
	JZ .Error

	; --------------------------------------------------------------
	; Verifico la presenza della siganture
	; --------------------------------------------------------------
	CMP WORD [BX+510],Bootes.Stage1.Info.BootSignature
	JNE .Error

	; --------------------------------------------------------------
	; Conteggio partizioni estese
	; --------------------------------------------------------------
	PUSH WORD 0
	
	; --------------------------------------------------------------
	; Processo la tabella delle partizioni
	; --------------------------------------------------------------
	ADD BX,(512-2-4*CurrentPEntry.Size)
	MOV CX,4
	.lAddPartition:
		; Sistemo l'indirizzo di inizio della partizione
		CLC
		MOV AX,[TableSectorLo]
		ADD [CurrentPEntry.Start],AX
		MOV AX,[TableSectorHi]
		ADC [CurrentPEntry.Start+2],AX
		
		MOV AL,[CurrentPEntry.Type]
		; Controllo che la partizione sia presente
		CMP AL,0
		JE .Continue
		; Controllo se è una partizione estesa
		CMP AL,0x05
		JE .iExtPartition
		CMP AL,0x0F
		JE .iExtPartition
		CMP AL,0x85
		JE .iExtPartition
		JMP .xExtPartition
		.iExtPartition:
			POP AX
			PUSH [CurrentPEntry.Start+2]
			INC AX
			PUSH [CurrentPEntry.Start]
			PUSH AX
			JMP .Continue
		.xExtPartition:
		; Aggiungo la partizione
		CALL Bootes.Devices.Disk.AddPartition,	DI,[CurrentPEntry.Start+2],[CurrentPEntry.Start], \
												[CurrentPEntry.Length+2],[CurrentPEntry.Length], \
												AX,0
		OR AX,AX
		JZ .Continue
		; Numero di partizioni montate per il disco
		INC DX
		.Continue:
		ADD BX,CurrentPEntry.Size
		LOOP .lAddPartition
		
	; --------------------------------------------------------------
	; Partizione estese
	; --------------------------------------------------------------
	POP CX
	OR CX,CX
	JZ .Exit
	.lExtended:
		POP BX
		POP AX
		CALL Bootes.Devices.Disk.MountPartitions,DI,AX,BX
		ADD DX,AX
		LOOP .lExtended

	.Error:
	.Exit:
	MOV AX,DX
	
	POP SI
	POP DI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Legge il numero di settori specificato dalla partizione 
; specificata.
; --------------------------------------------------------------
; PARAMS
;	Partition	Puntatore alla struttura Partition contenente le 
;				informazioni della partizione dalla quale leggere.
;	Buffer	Puntatore alla zona di memoria nella quale salvare i 
;			dati letti.
;	StartSector	Settore dal quale iniziare a leggere (LBA a 32 bit).
;	SectorsCount Numero di settori da leggere.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Devices.Disk.PartitionRead, Partition,BufferHi,BufferLo,StartSectorHi,StartSectorLo,SectorsCount
Begin
	PUSH BX
	PUSH SI
	
	MOV SI,[Partition]
	OR SI,SI
	JZ .Error
	MOV AX,[StartSectorHi]
	SHL EAX,16
	MOV AX,[StartSectorLo]
	ADD EAX,DWORD [CurrentPartition.Start]
	MOV BX,AX
	SHR EAX,16
	CALL Bootes.Devices.Disk.Read, [CurrentPartition.Disk],[BufferHi],[BufferLo],AX,BX,[SectorsCount]
	JMP .Exit
	
	.Error:
	XOR AX,AX
	.Exit:
	POP SI
	POP BX
Return



;====================================================================
; Legge il numero di settori specificato dal disco specificato.
; --------------------------------------------------------------
; PARAMS
;	Disk	Puntatore alla struttura Diks contenente le informazioni
;			del disco dal quale leggere.
;	Buffer	Puntatore alla zona di memoria nella quale salvare i 
;			dati letti.
;	StartSector	Settore dal quale iniziare a leggere (LBA a 32 bit).
;	SectorsCount Numero di settori da leggere.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Devices.Disk.Read, Disk,BufferHi,BufferLo,StartSectorHi,StartSectorLo,SectorsCount
Begin
	LockScheduler
	
	PUSH DI
	
	MOV DI,[Disk]
	OR DI,DI
	JZ .Error
	CALL [CurrentDisk.ReadDelegate], DI,[BufferHi],[BufferLo],[StartSectorHi],[StartSectorLo],[SectorsCount]
	JMP .Exit
	
	.Error:
	XOR AX,AX
	.Exit:	
	POP DI
	
	UnlockScheduler
Return



;====================================================================
; Legge il numero di settori specificato dal disco specificato
; utilizzando la modalità CHS.
; --------------------------------------------------------------
; PARAMS
;	Disk	Puntatore alla struttura Diks contenente le informazioni
;			del disco dal quale leggere.
;	Buffer	Puntatore alla zona di memoria nella quale salvare i 
;			dati letti.
;	StartSector	Settore dal quale iniziare a leggere (LBA a 32 bit).
;	SectorsCount Numero di settori da leggere.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Devices.Disk.ReadCHS, Disk,BufferHi,BufferLo,StartSectorHi,StartSectorLo,SectorsCount
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH DI
	
	; Inizializzo registri
	MOV DI,[Disk]
	OR DI,DI
	JZ .Error
	MOV BX,[BufferLo]
	MOV AX,[BufferHi]
	TEST AX,0xFFF0
	JNZ .Error
	SHL AX,12
	MOV ES,AX
	MOV AX,[StartSectorLo]
	; ricavo settore
	XOR DX,DX
	DIV WORD [CurrentDisk.Sectors]
	INC DL
	MOV CL,DL	; Settore ricavato!
	; Ricavo traccia e testina
	XOR DX,DX
	DIV	WORD [CurrentDisk.Heads]
	MOV CH,AL	; Traccia
	MOV AL,DL	; Head
	MOV DX,[CurrentDisk.DiskID]
	MOV DH,AL
	; Per preparare la funzione di lettura con l'INT 0x13
	MOV AX,[SectorsCount]
	AND AX,0xFF
	OR AX,0x0200
	INT 0x13
	JC .Error
	XOR AX,AX
	MOV ES,AX
	INC AX
	JMP .Exit
	.Error:
	XOR AX,AX
	MOV ES,AX
	.Exit:
	POP DI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Legge il numero di settori specificato dal disco specificato
; utilizzando la modalità CHS.
; --------------------------------------------------------------
; PARAMS
;	Disk	Puntatore alla struttura Diks contenente le informazioni
;			del disco dal quale leggere.
;	Buffer	Puntatore alla zona di memoria dalla quale leggere i dati
;			da scrivere sul disco.
;	StartSector	Settore dal quale iniziare a scrivere (LBA a 32 bit).
;	SectorsCount Numero di settori da scrivere.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Devices.Disk.WriteCHS, Disk,BufferHi,BufferLo,StartSectorHi,StartSectorLo,SectorsCount
Begin
Return



;====================================================================
; Legge il numero di settori specificato dal disco specificato
; utilizzando la modalità LBA.
; --------------------------------------------------------------
; PARAMS
;	Disk	Puntatore alla struttura Diks contenente le informazioni
;			del disco dal quale leggere.
;	Buffer	Puntatore alla zona di memoria nella quale salvare i 
;			dati letti.
;	StartSector	Settore dal quale iniziare a leggere (LBA).
;	SectorsCount Numero di settori da leggere.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Devices.Disk.ReadLBA, Disk,BufferHi,BufferLo,StartSectorHi,StartSectorLo,SectorsCount
Begin
	PUSH DX
	PUSH SI
	PUSH DI
	
	; Controllo disco
	MOV DI,[Disk]
	OR DI,DI
	JZ .Error
	; Imposto disco
	MOV DX,[CurrentDisk.DiskID]
	; Imposto buffer di destinazione
	MOV AX,[BufferHi]
	TEST AX,0xFFF0
	JNZ .Error
	SHL AX,12
	MOV [Bootes.Devices.Disk.Data.DiskAddressPacket.Buffer+2],AX
	MOV AX,[BufferLo]
	MOV [Bootes.Devices.Disk.Data.DiskAddressPacket.Buffer],AX
	; Imposto settore di inizio lettura
	MOV AX,[StartSectorLo]
	MOV [Bootes.Devices.Disk.Data.DiskAddressPacket.Start1],AX
	MOV AX,[StartSectorHi]
	MOV [Bootes.Devices.Disk.Data.DiskAddressPacket.Start2],AX
	XOR AX,AX
	MOV [Bootes.Devices.Disk.Data.DiskAddressPacket.Start3],AX
	MOV [Bootes.Devices.Disk.Data.DiskAddressPacket.Start4],AX
	; Imposto numero di settori da leggere
	MOV AX,[SectorsCount]
	MOV [Bootes.Devices.Disk.Data.DiskAddressPacket.Blocks],AX
	; Leggo
	MOV SI,Bootes.Devices.Disk.Data.DiskAddressPacket
	MOV AX,0x4200
	INT 0x13
	JC .Error
	MOV AX,[BufferLo]
	OR AX,AX
	JNZ .Exit
	MOV AX,-1
	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
Return



;====================================================================
; Scrive il numero di settori specificato dal disco specificato
; utilizzando la modalità LBA.
; --------------------------------------------------------------
; PARAMS
;	Disk	Puntatore alla struttura Diks contenente le informazioni
;			del disco nel quale scrivere.
;	Buffer	Puntatore alla zona di memoria dalla quale leggere i dati
;			da scrivere sul disco.
;	StartSector	Settore dal quale iniziare a scrivere (LBA).
;	SectorsCount Numero di settori da scrivere.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Devices.Disk.WriteLBA, Disk,BufferHi,BufferLo,StartSectorHi,StartSectorLo,SectorsCount
Begin
	; TODO
Return



;====================================================================
; Aggiunge una partizione alla lista.
; --------------------------------------------------------------
; PARAMS
;	Disk	Disco dove la partizione risiede.
;	Start	Settore di inizio della partizione nel disco (LBA).
;	Length	Lunghezza in settori della partizione.
;	Type	Tipo della partizione.
;	Name	Puntatore alla stringa terminata da 0 contenente il nome
;			da attribuire alla partizione oppure 0 per impostare il
;			nome di default.
; RETURN
;	Puntatore alla struttura Partition creata oppure zero se è 
;	avvenuto un errore.
;====================================================================
Function Bootes.Devices.Disk.AddPartition, Disk,StartHi,StartLo,LengthHi,LengthLo,Type,Name
Begin
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI
	
	; --------------------------------------------------------------
	; Imposto il disco
	; --------------------------------------------------------------
	MOV DI,[Disk]
	OR DI,DI
	JZ .Error
	
	; --------------------------------------------------------------
	; Alloco lo spazio per la nuova struttura Partition
	; --------------------------------------------------------------
	CMP [Bootes.Devices.Disk.Data.PartitionArray.FreeBlocks],0
	JNE .eExpand
	.iExpand:
		CALL Bootes.Memory.Expand, [Bootes.Devices.Disk.Data.PartitionArray],CurrentPartition.Size
		OR AX,AX
		JZ .Exit
		MOV [Bootes.Devices.Disk.Data.PartitionArray],AX
		JMP .xExpand
	.eExpand:
		DEC WORD [Bootes.Devices.Disk.Data.PartitionArray.FreeBlocks]
		MOV AX,[Bootes.Devices.Disk.Data.PartitionArray]
	.xExpand:
	MOV SI,[Bootes.Devices.Disk.Data.PartitionArray.Size]
	ADD SI,AX
	ADD WORD [Bootes.Devices.Disk.Data.PartitionArray.Size],CurrentPartition.Size
	INC WORD [Bootes.Devices.Disk.Data.PartitionArray.Count]

	; --------------------------------------------------------------
	; Inserisco la nuova partizione
	; --------------------------------------------------------------
	; StartLo
	MOV AX,[StartLo]
	MOV [CurrentPartition.Start],AX
	; StartHi
	MOV AX,[StartHi]
;	CMP AX,[CurrentDisk.TotalSectors+2]
;	JA .Error
	MOV [CurrentPartition.Start+2],AX
	; LengthLo
	MOV AX,[LengthLo]
	MOV [CurrentPartition.Length],AX
	; LengthHi
	MOV AX,[LengthHi]
	MOV [CurrentPartition.Length+2],AX
	; Disk
	MOV [CurrentPartition.Disk],DI
	; Index
	MOV AX,[CurrentDisk.PartitionCount]
	MOV [CurrentPartition.Index],AX
	; Mounted
	MOV BYTE [CurrentPartition.Mounted],0
	; Type
	MOV BX,[Type]
	OR BX,BX
	JNZ .eFindID
	.iFindID:
		CALL Bootes.IO.FileSystems.GetFSHandlerFromHeader, SI
		JMP .xFindID
	.eFindID:
		CALL Bootes.IO.FileSystems.GetFSHandlerFromID, BX
		OR AX,AX
		JZ .iFindID
	.xFindID:
	MOV [CurrentPartition.FSHandler],AX
	MOV [CurrentPartition.Type],BL
	
	; --------------------------------------------------------------
	; Aggiungo la partizione alla linked list
	; --------------------------------------------------------------
	MOV AX,[CurrentDisk.FirstPartition]
	OR AX,AX
	JNZ .eFirst
	.iFirst:
		MOV [CurrentDisk.FirstPartition],SI
		MOV WORD [CurrentPartition.Previous],0
		JMP .xFirst
	.eFirst:
		XCHG AX,SI
		@@:
		CMP WORD [CurrentPartition.Next],0
		JE @f
		MOV SI,[CurrentPartition.Next]
		JMP @b
		@@:
		MOV [CurrentPartition.Next],AX
		XCHG AX,SI
		MOV [CurrentPartition.Previous],AX
	.xFirst:		
	; Next
	MOV WORD [CurrentPartition.Next],0
	; Incremento il numero di partizioni nel disco
	INC WORD [CurrentDisk.PartitionCount]
	
	; --------------------------------------------------------------
	; Imposto il nome della partizione
	; --------------------------------------------------------------
	; Recupero il nome
	MOV AX,[Name]
	OR AX,AX
	JNZ @f
		CALL Bootes.Devices.Disk.GetPartitionDefaultName, SI
	@@:
	; Copio il nome
	MOV BX,SI
	LEA DI,[CurrentPartition.Name]
	MOV SI,AX
	CALL Bootes.String.Length, AX
	MOV CX,AX
	CMP AX,9
	JA .Error
	INC CX
	REP MOVSB
	
	; --------------------------------------------------------------
	; Ritorno
	; --------------------------------------------------------------
	MOV AX,BX
	
	JMP .Exit
	.Error:
	INC WORD [Bootes.Devices.Disk.Data.PartitionArray.FreeBlocks]
	SUB WORD [Bootes.Devices.Disk.Data.PartitionArray.Size],CurrentPartition.Size
	DEC WORD [Bootes.Devices.Disk.Data.PartitionArray.Count]
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP CX
	POP BX
Return



;====================================================================
; Controlla se una partizione è montata o meno.
; --------------------------------------------------------------
; PARAMS
;	Name	Puntatore alla stringa terminata da 0 contenente il nome
;			della partizione.
; RETURN
;	Valore booleano TRUE se la partizione è montata FALSE altrimenti.
;====================================================================
Function Bootes.Devices.Disk.IsMounded, Name
Begin
	PUSH SI
	
	; --------------------------------------------------------------
	; Ricavo la partizione
	; --------------------------------------------------------------
	CALL Bootes.Devices.Disk.GetPartition, [Name]
	OR AX,AX
	JZ .Error
	MOV SI,AX
	
	; --------------------------------------------------------------
	; Carico il valore
	; --------------------------------------------------------------
	MOVZX AX, BYTE [CurrentPartition.Mounted]
	
	POP SI
Return



;====================================================================
; Ritorna la partizione indicata.
; --------------------------------------------------------------
; PARAMS
;	Name	Puntatore alla stringa terminata da 0 contenente il nome
;			della partizione.
; RETURN
;	Puntatore alla struttura partition desiderata oppure zero se non
;	è stata trovata alcuna partizione corrispondente.
;====================================================================
Function Bootes.Devices.Disk.GetPartition, Name
Begin
	PUSH CX
	PUSH SI
	PUSH DI
	; --------------------------------------------------------------
	; Verifica validità stringa
	; --------------------------------------------------------------
	MOV DI,[Name]
	OR DI,DI
	JZ .Error
	CALL Bootes.String.Length, DI
	OR AX,AX
	JZ .Error
	MOV CX,AX
	
	; --------------------------------------------------------------
	; Ricerca partizione desiderata
	; --------------------------------------------------------------
	MOV SI,[Bootes.Devices.Disk.Data.PartitionArray]
	MOV AX,[Bootes.Devices.Disk.Data.PartitionArray.Count]
	.lFindPartition:
		; Controllo fine ciclo
		OR AX,AX
		JZ .Error
		; Confronto il nome
		PUSH DI
		PUSH SI
		PUSH CX
		LEA SI,[CurrentPartition.Name]
		REPE CMPSB
		POP CX
		POP SI
		POP DI
		JE .xFindPartition
		; Prossima partizione
		ADD SI,CurrentPartition.Size
		DEC AX
		JMP .lFindPartition
	.xFindPartition:
	
	MOV AX,SI
	JMP .Exit
	.Error:
	ErrorSetup UnknowPartition,DI
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP CX
Return



;====================================================================
; Rimuove una partizione dalla lista.
; --------------------------------------------------------------
; PARAMS
;	Name	Puntatore alla stringa terminata da 0 contenente il nome
;			della partizione.
;====================================================================
Function Bootes.Devices.Disk.DeletePartition, Name
Begin
	; TODO
Return



;====================================================================
; Rinomina la partizione selezionata.
; --------------------------------------------------------------
; PARAMS
;	Name	Puntatore alla stringa terminata da 0 contenente il nome
;			della partizione.
;	NewName	Puntatore alla stringa terminata da 0 contenente il nome
;			da assegnare alla partizione.
;====================================================================
Function Bootes.Devices.Disk.RenamePartition, Name,NewName
Begin
	PUSH SI

	; --------------------------------------------------------------
	; Controllo validità stringhe
	; --------------------------------------------------------------
	MOV SI,[NewName]
	CALL Bootes.String.Length,SI
	OR AX,AX
	JZ .Error
	CMP AX,CurrentPartition.MaxNameChar
	JAE .Error
	CALL Bootes.Devices.Disk.GetPartition, SI
	OR AX,AX
	JNZ .Error
	CALL Bootes.Devices.Disk.GetPartition, [Name]
	OR AX,AX
	JZ .Exit
	
	; --------------------------------------------------------------
	; Rinomino
	; --------------------------------------------------------------
	PUSH SI
	MOV SI,AX
	LEA AX,[CurrentPartition.Name]
	POP SI
	CALL Bootes.String.Move, SI,AX
	MOV AX,SI
	
	JMP .Exit
	.Error:
	ErrorSetup BadParameter,SI
	XOR AX,AX
	.Exit:
	POP SI
Return



;====================================================================
; Genera il nome di default della partizione specificata.
; --------------------------------------------------------------
; PARAMS
;	Partition	Puntatore alla struttura partizione inizializzata
;				per la quale generare il nome.
; RETURN
;	Puntatore al nome di default per la partaione oppure zero se è
;	avvenuto un errore.
;====================================================================
Function Bootes.Devices.Disk.GetPartitionDefaultName, Partition
Begin
	PUSH DI
	PUSH SI
	
	MOV SI,[Partition]
	OR SI,SI
	JZ .Error
	MOV DI,[CurrentPartition.Disk]
	OR DI,DI
	JZ .Error
	; --------------------------------------------------------------
	; Imposto il tipo del disco
	; --------------------------------------------------------------
	MOV AX,[CurrentDisk.DiskID]
	TEST AX,0x80
	JZ .eHardDisk
	.iHardDisk:
		MOV BYTE [Bootes.Devices.Disk.Data.DefaultName],'h'
		JMP .xHardDisk
	.eHardDisk:
		MOV BYTE [Bootes.Devices.Disk.Data.DefaultName],'f'
	.xHardDisk:
	; --------------------------------------------------------------
	; Imposto il numero del disco
	; --------------------------------------------------------------
	AND AX,0x7F
	CALL Bootes.String.FromNumber, Bootes.Devices.Disk.Data.DefaultName+2,0,AX,1,10
	MOV DI,AX
	; Partizione fittizia?
	CMP WORD [CurrentPartition.Index],-1
	JNE @f
		MOV BYTE [DI],0
		MOV AX,Bootes.Devices.Disk.Data.DefaultName
		JMP .Exit
	@@:
	MOV BYTE [DI],','
	; --------------------------------------------------------------
	; Imposto il numero della partizione
	; --------------------------------------------------------------
	MOV AX,[CurrentPartition.Index]
	INC DI
	AND AX,0x7FFF
	CALL Bootes.String.FromNumber, DI,0,AX,1,10
	MOV DI,AX
	MOV BYTE [DI],0
	; --------------------------------------------------------------
	; Ritorno
	; --------------------------------------------------------------
	MOV AX,Bootes.Devices.Disk.Data.DefaultName
	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	POP SI
	POP DI
Return



;====================================================================
; Scrive a video informazioni sui dischi.
;====================================================================
align 4
Bootes.Devices.Disk.InfoDisk:
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI

	; Info header
	CALL Bootes.String.FromNumber, Bootes.Devices.Disk.Data.Info+21,0,[Bootes.Devices.Disk.Data.PartitionArray.Count],1,10
	CALL Bootes.IO.Standard.Output, Bootes.Devices.Disk.Data.Info
	; 
	MOV CX,[Bootes.Devices.Disk.Data.DiskCount]
	OR CX,CX
	JZ .Error
	MOV DI,[Bootes.Devices.Disk.Data.DiskArray]
	.lDisk:
		;
		MOV AX,[CurrentDisk.DiskID]
		TEST AX,0x80
		JZ .eHardDisk
		.iHardDisk:
			AND AX,0x7F
			MOV WORD [Bootes.Devices.Disk.Data.Info.Disk],'hd'
			CALL Bootes.String.FromNumber, Bootes.Devices.Disk.Data.Info.Disk+2,0,AX,1,10
			MOV SI,AX
			MOV BYTE [SI],0
			CALL Bootes.IO.Standard.Output, Bootes.Devices.Disk.Data.Info.Disk
			CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
			;
			CMP WORD [CurrentDisk.PartitionCount],0
			JE .xHardDisk
			MOV SI,[CurrentDisk.FirstPartition]
			CALL Bootes.IO.Standard.Output, Bootes.Devices.Disk.Data.Info.PartHeader
			.lPartitions:
				OR SI,SI
				JZ .xHardDisk
				; Copio il nome
				LEA AX,[CurrentPartition.Name]
				CALL Bootes.String.Move, AX,Bootes.Devices.Disk.Data.Info.Part+5
				MOV BX,AX
				PUSH CX
				MOV CX,Bootes.Devices.Disk.Data.Info.Part+16
				SUB CX,AX
				@@:
				MOV BYTE [BX],' '
				INC BX
				LOOP @b
				POP CX
				; Type
				MOVZX AX,BYTE [CurrentPartition.Type]
				CALL Bootes.String.FromNumber, Bootes.Devices.Disk.Data.Info.Part+16,0,AX,8,16
				MOV BX,AX
				MOV BYTE [BX],'-'
				; Type (FileSystem)
				CALL Bootes.IO.FileSystems.GetFSHanlerName, [CurrentPartition.FSHandler]
				CALL Bootes.String.Move, AX,Bootes.Devices.Disk.Data.Info.Part+19
				MOV BX,AX
				PUSH CX
				MOV CX,Bootes.Devices.Disk.Data.Info.Part+27
				SUB CX,AX
				@@:
				MOV BYTE [BX],' '
				INC BX
				LOOP @b
				POP CX
				; Size
				MOV EAX,DWORD [CurrentPartition.Length]
				SHR EAX,11
				MOV BX,AX
				SHR EAX,16
				CALL Bootes.String.FromNumber, Bootes.Devices.Disk.Data.Info.Part+27,AX,BX,1,10
				MOV BX,AX
				MOV WORD [BX],'MB'
				MOV BYTE [BX+2],0
				; Scrivo le informazioni sulla partizione
				CALL Bootes.IO.Standard.Output, Bootes.Devices.Disk.Data.Info.Part
				CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
				; Next partition
				MOV SI,[CurrentPartition.Next]
				JMP .lPartitions
			.xPartitions:
			JMP .xHardDisk
		.eHardDisk:
			MOV SI,[CurrentDisk.FirstPartition]
			OR SI,SI
			JZ .xHardDisk
			LEA AX,[CurrentPartition.Name]
			CALL Bootes.IO.Standard.Output, AX
			CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
		.xHardDisk:
		ADD DI,CurrentDisk.Size
		DEC CX
		JNZ .lDisk
	.xDisk:
	MOV AX,1
	JMP .Exit
	.Error:
	ErrorSetup NoDiskMounted
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP CX
	POP BX
RET



;====================================================================
; Data
;====================================================================
align 2
Bootes.Devices.Disk.Data:
	.DiskArray					DW 0	; Array di strutture Disk
	.DiskArray.Size				DW 0	; Dimensione del DiskArray in byte
	.DiskArray.FreeBlocks		DW 0	; Numero di spazi liberi nell'array
	;
	.PartitionArray				DW 0	; Puntatore alla lista di strutture partizione.
	.PartitionArray.Size		DW 0	; Dimensione della lista in byte.
	.PartitionArray.FreeBlocks	DW 0	; Numero di spazi liberi nella lista.
	.PartitionArray.Count		DW 0
	;
	.DiskCount				DW ?
	.FloppyCount			DB ?,0
	.HDCount				DB ?,0
	;
	.DiskAddressPacket:
		.DiskAddressPacket.Size		DB 0x10, 0
		.DiskAddressPacket.Blocks	DW 1
		.DiskAddressPacket.Buffer	DW Bootes.Stage1.Info.Origin,0
		.DiskAddressPacket.Start1	DW 0
		.DiskAddressPacket.Start2	DW 0
		.DiskAddressPacket.Start3	DW 0
		.DiskAddressPacket.Start4	DW 0
	;
	.FloppyProbeValues		DB 36, 18, 15, 9, 0
	.DefaultName			DB "?d00,00    ",0
	;
	.Info					DB "Mounted disks. Total:    ",13,10,0
	.Info.Disk				DB "?d00 000.000MB",13,10,0
	.Info.PartHeader		DB "     Partition  Type       Size",13,10,0
	.Info.Part				DB "     NAME______ 00-Unknow  0000000000MB",0