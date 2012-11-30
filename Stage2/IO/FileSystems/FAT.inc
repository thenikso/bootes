;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.IO.FileSystems.FAT
;
; FAT file system IO driver.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2


; --------------------------------------------------------------
; FSEntries
; --------------------------------------------------------------
Bootes.IO.FileSystems.FAT12		Bootes.IO.FileSystems.FSHandler 0x01,"FAT12", \
																0, \
																Bootes.IO.FileSystems.FAT.Mount, \
																Bootes.IO.FileSystems.FAT.Dir, \
																Bootes.IO.FileSystems.FAT.Read, \
																0
;Bootes.IO.FileSystems.FAT16		Bootes.IO.FileSystems.FSHandler 0x,"FAT16",0,0,0
Bootes.IO.FileSystems.FAT32		Bootes.IO.FileSystems.FSHandler 0x0C,"FAT32", \
																Bootes.IO.FileSystems.FAT32.Check, \
																Bootes.IO.FileSystems.FAT.Mount, \
																Bootes.IO.FileSystems.FAT.Dir, \
																Bootes.IO.FileSystems.FAT.Read, \
																0


;====================================================================
; Inizializza la gestione dei file systems FAT.
;====================================================================
Bootes.IO.FileSystems.FAT.Init:
	CALL Bootes.IO.FileSystems.AddFS, Bootes.IO.FileSystems.FAT32
	CALL Bootes.IO.FileSystems.AddFS, Bootes.IO.FileSystems.FAT12
	RET


; --------------------------------------------------------------
; FAT Constants
; --------------------------------------------------------------
; FAT attribute for FAT.FileEntry.Attribute field
FAT_ATTR_READ_ONLY	= 0x01
FAT_ATTR_HIDDEN		= 0x02
FAT_ATTR_SYSTEM		= 0x04
FAT_ATTR_VOLUME_ID	= 0x08
FAT_ATTR_DIRECTORY	= 0x10
FAT_ATTR_ARCHIVE	= 0x20
FAT_ATTR_LONG_NAME	= (FAT_ATTR_READ_ONLY or FAT_ATTR_HIDDEN or FAT_ATTR_SYSTEM or FAT_ATTR_VOLUME_ID)


; --------------------------------------------------------------
; Strutture FAT
; --------------------------------------------------------------
STRUC Bootes.IO.FileSystem.FAT.BPB	; BIOS Parameter Block
{
	.Jump				RB 3
	.OEMName			DB "ARESPROJ"
	.BytesPerSector		DW 512
	.SectorsPerCluster	DB 1
	.ReservedSectors	DW 1
	.NumFATs			DB 2
	.RootEntryCount		DW 224
	.TotSec16			DW 2880
	.Media				DB 0xF0
	.FATsz16			DW 9
	.SectorsPerTrack	DW 18
	.NumHeads			DW 2
	.HiddSec			DD 0
	.TotSec32			DD 0
	; Solo Per FAT12-16
;	.DrivNum			DB 0
;						DB 0
;						DB 0x29
;	.VolID				DD 1
;	.VolLabel			DB "NO NAME    "
;	.FileSysType		DB "FAT12   "
	; Solo per FAT32
	.FATsz32			DD ?
	.ExtFlags			DW ?
	.Version			DW ?
	.RootCluster		DD ?
	.FSInfo				DW 1 ; Settore dove risiede la FAT32.Info. Solitamente 1.
	.BkBootSector		DW 6 ; Da questo settore e per i 2 seguenti vi è una copia del boot
						RB 12
	.DriveNumber		DB ?,0
	.BootSignature		DB 0x29
	.VolumeID			DD ?
	.VolumeLabel		DB "NO NAME    "
	.FileSystemType		DB "FAT32   "
}

; FAT32 FSInfo structure
; Le informazioni che contiene vanno aggiornate. Anche la sua copia presente nel settore
; BPB.BkBootSector+1 va aggiornata.
STRUC Bootes.IO.FileSystems.FAT32.FSInfo 
{
	.Start:
	.LeadSign			DD 0x41615252
	.Reserved1			RB 480
	.StrucSign			DD 0x61417272
	.FreeCount			DD ?	; Conteggio del numero di cluster liberi
	.NextFree			DD ?	; Ultimo cluster libero conosciuto
	.Reserved2			RB 12
	.TailSign			DD 0xAA550000
	.Size				= $-.Start
}

STRUC Bootes.IO.FileSystems.FAT.FileEntry
{
	.Name				RB 11
	; Note speciali sul nome:
	; Name[0]=0xE5	File entry vuota.
	; Name[0]=0x00	File entry vuota, non ci sono altre entrate dopo di questa.
	; Name[0]=0x05	Sostituisce il kanji che usa il codice 0xE5.
	; Il nome è diviso in 2 parti, 8 byte per il nome e 3 per l'estensione dell'entrata.
	; Le due parti sono divise, se necessario, da spazi.
	; Caratteri non consentiti: 0x22, 0x2A, 0x2B, 0x2C, 0x2E, 0x2F, 0x3A, 0x3B, 0x3C, 
	; 0x3D, 0x3E, 0x3F, 0x5B, 0x5C, 0x5D, 0x7C. Più i valori <0x20 in Name[0].
	.Attribute			DB ?
	.NTReserved			DB ?
	.CreationTime.Tenth	DB ?
	.CreationTime.Time	DW ?
	; Formato tempo:
	; bit 0-4	Secondi a passo 2
	; bit 5-10	Minuti
	; bit 11-15	Ora
	.CreationTime.Date	DW ?
	; Formato data:
	; bit 0-4	Giorno dell'anno
	; bit 5-10	Mese
	; bit 11-15	Numero anni dal 1980
	.AccessTime.Date	DW ?
	.FirstClusterHi		DW ?
	.WriteTime.Time		DW ?
	.WriteTime.Date		DW ?
	.FirstClusterLo		DW ?
	.FileSize			DW ?,?
}

; Nel caso una FileEntry avesse come attributo FAT_ATTR_LONG_NAME allora va interpretata
; nella maniera seguente.
STRUC Bootes.Devices.FileSystem.FAT.LongFileEntry
{
	.Order				DB ?
	.Name1				RB 10
	.Attribute			DB FAT_ATTR_LONG_NAME
	.Type				DB ?
	.Checksum			DB ?
	.Name2				RB 12
	.Arifact			DW 0
	.Name3				RB 4
}


; --------------------------------------------------------------
; Strutture interne
; --------------------------------------------------------------
STRUC Bootes.IO.FileSystems.FAT.State
{
	.Start:
	; File Buffer
	.FATBuffer				DW ?,?
	.FileBuffer				DW ?,?
	.FileBufferSize			= 512*4 ;0x2000-512
	; General
	.FATType				DB ?
	.SectorsPerCluster		DB ?
	.SectorsCount			DD ?
	; FAT Sector
	.FATSector				DD ?
	.FATSectorsCount		DD ?
	; Root Directory
	.RootCluster:
	.RootDirSector			DW ?
	.RootDirSectorsCount	DW ?
	; Data Sector
	.DataSector				DD ?
	.DataSectorsCount		DD ?
	; Current directory
	;.Name					DW ?		; TODO: Come fare? (?)
	.CDFirstCluster			DD ?		; TODO: Implementare utilizzo di questo campo
	
	.Size					= $-.Start
}

VIRTUAL AT BX
	CurrentFATBPB	Bootes.IO.FileSystem.FAT.BPB
END VIRTUAL

VIRTUAL AT DI
	CurrentFATState	Bootes.IO.FileSystems.FAT.State
END VIRTUAL	


;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Implementazione
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Function Bootes.IO.FileSystems.FAT.Mount, Partition
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	; [Disk.inc]CurrentPartition
	MOV SI,[Partition]
	OR SI,SI
	JZ .Error
	
	; --------------------------------------------------------------
	; Controllo disco e lettura BPB
	; --------------------------------------------------------------
	CMP [CurrentPartition.Disk],0
	JE .Error
	MOV AL,BYTE [CurrentPartition.Mounted]
	CMP AL,1
	JE .Exit
	; Leggo
	MOV BX,Bootes.Stage1.Info.Origin
	CALL Bootes.Devices.Disk.Read, [CurrentPartition.Disk],0,BX,[CurrentPartition.Start+2],[CurrentPartition.Start],1
	OR AX,AX
	JZ .Error
	; Controllo magic
	CMP WORD [BX+510],Bootes.Stage1.Info.BootSignature
	JNE .Error
	
	; --------------------------------------------------------------
	; Recupero dati
	; --------------------------------------------------------------
	; Creazione blocco di informazioni
	CALL Bootes.Memory.Alloc, CurrentFATState.Size
	OR AX,AX
	JZ .Exit
	MOV DI,AX
	MOV [CurrentPartition.FSInfo],AX
	; Popolazione struttura di stato
	; Sectors per cluster shift value
	MOV AL,BYTE [CurrentFATBPB.SectorsPerCluster]
	MOV [CurrentFATState.SectorsPerCluster],AL
	
	; FAT Sector
	MOVZX EAX,WORD [CurrentFATBPB.ReservedSectors]
	MOV [CurrentFATState.FATSector],EAX
	
	MOV AX,[CurrentFATBPB.FATsz16]
	OR AX,AX
	JZ .InitFAT32
	.InitFAT16:
		; Sectors Count
		MOVZX EAX,[CurrentFATBPB.TotSec16]
		MOV [CurrentFATState.SectorsCount],EAX
		
		; FAT Sectors Count
		MOVZX EAX,[CurrentFATBPB.FATsz16]
		MOV [CurrentFATState.FATSectorsCount],EAX
		
		; Root Sector
		MOV CL,[CurrentFATBPB.NumFATs]
		MUL CL
		ADD AX,[CurrentFATBPB.ReservedSectors]
		MOV [CurrentFATState.RootDirSector],AX
		
		; Root Sectors Count
		MOVZX EAX,[CurrentFATBPB.RootEntryCount]
		MOV CL,32
		MUL CL
		ADD AX,[CurrentFATBPB.BytesPerSector]
		DEC AX		; Per approssimare per eccesso aggiungo un settore meno 1
		SHR AX,9	; TODO: Rendere dinamico: 9bit per settori da 512...
		MOV [CurrentFATState.RootDirSectorsCount],AX
		
		; Data Sector
		ADD AX,[CurrentFATState.RootDirSector]
		MOV [CurrentFATState.DataSector],EAX
		
		; Data Sectors Count
		MOV ECX,[CurrentFATState.SectorsCount]
		SUB CX,AX
		MOV [CurrentFATState.DataSectorsCount],ECX
		
		; FAT type
		CMP CX,(4085 + 2)
		JA .eInitFAT12
		.iInitFAT12:
			MOV [CurrentFATState.FATType],12
			JMP .xInitFAT12
		.eInitFAT12:
			MOV [CurrentFATState.FATType],16
		.xInitFAT12:
		
		JMP .xInitFAT
	.InitFAT32:	
	;	ProtectMode
	;	AND EDI,0xFFFF
		; Sectors Count
		MOV EAX,[CurrentFATBPB.TotSec32]
		MOV [CurrentFATState.SectorsCount],EAX
		
		; FAT Sectors Count
		MOV EAX,[CurrentFATBPB.FATsz32]
		MOV [CurrentFATState.FATSectorsCount],EAX
		
		; Data Sector
		MOVZX CX,BYTE [CurrentFATBPB.NumFATs]
		MUL CX
		ADD EAX,[CurrentFATState.FATSector]
		MOV [CurrentFATState.DataSector],EAX
		
		; Data Sectors Count
		MOV ECX,[CurrentFATState.SectorsCount]
		SUB ECX,EAX
		MOV [CurrentFATState.DataSectorsCount],ECX
		
		; Root Cluster
		MOV EAX,[CurrentFATBPB.RootCluster]
		MOV [CurrentFATState.RootCluster],EAX
		
		; FAT Type
		MOV [CurrentFATState.FATType],32
		
	;	RealMode
	.xInitFAT:
	
	; Alloco 8KB di spazio utile
	CALL Bootes.Memory.DataAlloc, 0x2000
	CMP AX,-1
	JE .Cancel
	MOV CX,AX
	MOV BX,DS
	XOR AX,AX
	SHR BX,12
	MOV DS,AX
	MOV [CurrentFATState.FATBuffer+2],BX
	MOV [CurrentFATState.FATBuffer],CX
	MOV [CurrentFATState.FileBuffer+2],BX
	ADD CX,0x200
	MOV [CurrentFATState.FileBuffer],CX
	
	MOV AX,1
	MOV [CurrentPartition.Mounted],AL
	JMP .Exit
	.Cancel:
	CALL Bootes.Memory.Free, [CurrentPartition.FSInfo]
	.Error:
	LEA AX,[CurrentPartition.Name]
	ErrorSetup UnableToMount,AX
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



Function Bootes.IO.FileSystems.FAT32.Check, Header
Begin
	PUSH BX

	; Eseguo controlli
	MOV BX,[Header]
	MOV AX,1
	CMP WORD [BX+510],Bootes.Stage1.Info.BootSignature
	JNE .Error
	CMP DWORD [CurrentFATBPB.FATsz32],0
	JE .Error
	CMP BYTE [CurrentFATBPB.BootSignature],0x29
	JNE .Error
	CMP DWORD [CurrentFATBPB.FileSystemType],'FAT3'
	JE .Exit
	
	.Error:
	XOR AX,AX
	.Exit:
	POP BX
Return



Function Bootes.IO.FileSystems.FAT.Dir, Partition, DirName
Begin
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI

	; [Disk.inc]CurrentPartition
	MOV SI,[Partition]
	OR SI,SI
	JZ .Error
	; Controllo che sia montata
	CMP BYTE [CurrentPartition.Mounted],0
	JE .Error
	; Recupero le informazioni della partizione
	MOV DI,[CurrentPartition.FSInfo]
	OR DI,DI
	JZ .Error
	; Controllo che la partizione sia collegata a un disco valido
	CMP [CurrentPartition.Disk],0
	JE .Error
	; Trovo il primo cluster per la directory selezionata
	MOV BX,[DirName]
	OR BX,BX
	JZ .eLoadNamedDir
	CALL Bootes.String.Length, BX
	OR AX,AX
	JZ .eLoadNamedDir
	.iLoadNamedDir:
		CALL Bootes.IO.FileSystems.FAT.GetEntryFirstCluster, SI,BX
		OR EAX,EAX
		JNZ @f
			ErrorSetup DirNotFound,BX
			JMP .Exit
		@@:
		MOV DWORD [CurrentFATState.CDFirstCluster],EAX
		JMP .xLoadNamedDir
	.eLoadNamedDir:
		MOV DWORD [CurrentFATState.CDFirstCluster],-1
	.xLoadNamedDir:
	; TODO: Generare nomi (?)
	JMP .Exit
		
	.Error:
	ErrorSetup FileReadError
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP CX
	POP BX
Return



Function Bootes.IO.FileSystems.FAT.Read, Partition, FileName, BufferHi, BufferLo, Size
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	; [Disk.inc]CurrentPartition
	MOV SI,[Partition]
	OR SI,SI
	JZ .Error
	; Controllo che sia montata
	CMP BYTE [CurrentPartition.Mounted],0
	JE .Error
	; Recupero le informazioni della partizione
	MOV DI,[CurrentPartition.FSInfo]
	OR DI,DI
	JZ .Error
	; Controllo che la partizione sia collegata a un disco valido
	CMP [CurrentPartition.Disk],0
	JE .Error
	; Cerco il file
	MOV BX,[FileName]
	CALL Bootes.IO.FileSystems.FAT.GetEntryFirstCluster, SI,BX
	OR EAX,EAX
	JNZ @f
		ErrorSetup FileNotFound,BX
		JMP .Exit
	@@:
	; Ciclo di lettura ed eventuale spostamento in memoria
	MOV BX,[BufferHi]
	MOV DX,[Size]
	SHL EBX,16
	MOV BX,[BufferLo]
	OR EBX,EBX
	JZ .eMoveBuffer
	.iMoveBuffer:
		; Controllo byte da leggere
		OR DX,DX
		JNZ @f
			CMP EAX,-1
			JNE @f
				MOV AX,[BufferHi]
				SHL EAX,16
				MOV AX,[BufferLo]
				JMP .Exit
		@@:
		; Controllo dimensioni buffer
		CMP DX,CurrentFATState.FileBufferSize
		JBE @f
			MOV DX,CurrentFATState.FileBufferSize
			SUB WORD [Size],DX
		@@:
		; Preparo il numero del primo cluster
		MOV CX,AX
		SHR EAX,16
		; Leggo il file nel buffer standard
		PUSH EBX
		MOVZX BX,BYTE [CurrentFATState.FATType]
		CALL Bootes.IO.FileSystems.FAT.ReadClusters, SI,BX,AX,CX,DX
		POP EBX
		; Sposto il file
		MOV ESI,DWORD [CurrentFATState.FileBuffer]
		ProtectMode
		MOV EDI,EBX
		MOVZX ECX,DX
		SHR ECX,2
		OR ECX,ECX
		JNZ @f
			MOV ECX,(CurrentFATState.FileBufferSize/4)
		@@:
		REP MOVSD
		RealMode
		; Ciclo
		MOV DX,[Size]
		CMP DX,CurrentFATState.FileBufferSize
		JA @f
			MOV AX,[BufferHi]
			SHL EAX,16
			MOV AX,[BufferLo]
			JMP .Exit
		@@:
		JMP .iMoveBuffer
	.eMoveBuffer:
		MOVZX BX,BYTE [CurrentFATState.FATType]
		; Preparo il numero del primo cluster
		MOV CX,AX
		SHR EAX,16
		; Verifico che il buffer standard sia abbastanza ampio per la dimensione desiderata
		CMP DX,CurrentFATState.FileBufferSize
		JA .Error
		; Leggo il file
		CALL Bootes.IO.FileSystems.FAT.ReadClusters, SI,BX,AX,CX,DX
		OR EAX,EAX
		JZ .Error
		; Risultato
		MOV EAX,DWORD [CurrentFATState.FileBuffer]
	.xMoveBuffer:
	
	JMP .Exit
	.Error:
	ErrorSetup FileReadError
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Supporto
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Restituisce il numero del primo cluster del file specificato all'interno della cartella corrente
Function Bootes.IO.FileSystems.FAT.GetEntryFirstCluster, Partition, Name
.LastDirCluster	DW ?,?
Begin
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI
	
	MOV BX,[Name]
	OR BX,BX
	JZ .Error
	CALL Bootes.String.Length, BX
	OR AX,AX
	JZ .Error
	CMP AX,12
	JA .eShortName
	.iShortName:
		; --------------------------------------------------------------
		; Preparo il nome da cercare
		; --------------------------------------------------------------
		MOV SI,BX
		MOV DI,Bootes.IO.FileSystems.FAT.Data.ShortName
		MOV CX,8
		.lCopyName:
			LODSB
			CMP AL,'.'
			JE .xCopyName
			CMP AL,0
			JNE @f
				ADD CX,3
				JMP .xCopyExt
			@@:
			STOSB
			LOOP .lCopyName
		.xCopyName:
		; TODO: Inserire controllo per verificare che non sia un long name
		MOV AL,' '
		REP STOSB
		MOV CX,3
		.lCopyExt:
			LODSB
			CMP AL,0
			JE .xCopyExt
			STOSB
			LOOP .lCopyExt
		.xCopyExt:
		MOV AL,' '
		REP STOSB
		MOV BX,Bootes.IO.FileSystems.FAT.Data.ShortName
		CALL Bootes.String.ToUpperCase, BX
		; --------------------------------------------------------------
		; Carico il contenuto della cartella corrente e cerco
		; --------------------------------------------------------------
		MOV SI,[Partition]
		CALL Bootes.IO.FileSystems.FAT.LoadCurrentDirectory, SI
		OR EAX,EAX
		JZ .Exit
		MOV DWORD [.LastDirCluster],EAX
		; Inizio ricerca sul contenuto della cartella caricato
		.ShortNameSearchStart:
		MOV CX,11
		MOV DI,[CurrentPartition.FSInfo]
		MOV SI,WORD [CurrentFATState.FileBuffer]
		MOV AX,[CurrentFATState.FileBuffer+2]
		MOV DI,Bootes.IO.FileSystems.FAT.Data.ShortName
		; Imposto il DS
		SHL AX,12
		MOV DS,AX
		; Cerco tra i nomi brevi
		.lShortNameSearch:
			MOV AL,[DS:SI]
			; Entry non trovata
			CMP AL,0
			JE .ShortNameDirFetchMore
			; Entry non allocata
			CMP AL,0xE5
			JE .nShortNameSearch
			; Ripristino kanji
			CMP AL,0x05
			JNE @f
				MOV BYTE [DS:SI],0xE5
			@@:
			; Confronto entry
			MOV AX,SI
			PUSH CX
			REPE CMPSB
			POP CX
			JE .xShortNameSearch
			MOV SI,AX
			MOV DI,BX
			; Prossima entry
			.nShortNameSearch:
			ADD SI,32
			CMP SI,CurrentFATState.FileBufferSize
			JAE .ShortNameDirFetchMore
			JMP .lShortNameSearch
			.xShortNameSearch:
		; Ricavo cluster iniziale
		MOV BX,AX
		MOV AX,[DS:BX+20]
		SHL EAX,16
		MOV AX,[DS:BX+26]
		; Ripristino il DS
		PUSH WORD 0
		POP DS
		; Ritorno
		JMP .Exit
		; --------------------------------------------------------------
		; Se disponibile, carico ulteriore contenuto della cartella
		; --------------------------------------------------------------
		.ShortNameDirFetchMore:
		; Ripristino il DS
		PUSH WORD 0
		POP DS
		; Controllo se ci sono ancora cluster da caricare per la cartella
		MOV EAX,DWORD [.LastDirCluster]
		CMP EAX,-1
		JE .Error
		; Carico contenuto cartella
		MOV CX,AX
		SHR EAX,16
		MOV SI,[Partition]
		MOV DI,[CurrentPartition.FSInfo]
		MOVZX DI,BYTE [CurrentFATState.FATType]
		CALL Bootes.IO.FileSystems.FAT.ReadClusters, SI,DI,AX,CX,CurrentFATState.FileBufferSize
		OR EAX,EAX
		JZ .Exit
		MOV DWORD [.LastDirCluster],EAX
		JMP .ShortNameSearchStart
	.eShortName:
		JMP .Error
	.xSnortName:
	
	.Error:
	XOR EAX,EAX
	.Exit:
	POP DI
	POP SI
	POP CX
	POP BX
Return


; Carica il contenuto della cartella corrente nel buffer
Function Bootes.IO.FileSystems.FAT.LoadCurrentDirectory, Partition
Begin
	PUSH BX
	PUSH SI
	PUSH DI
	
	MOV SI,[Partition]
	MOV DI,[CurrentPartition.FSInfo]
	
	MOV EAX,[CurrentFATState.CDFirstCluster]
	CMP EAX,-1
	JNE .eRootDir
	.iRootDir:
		MOV AL,BYTE [CurrentFATState.FATType]
		CMP AL,16
		JA .Root32
			MOV BX,[CurrentFATState.RootDirSectorsCount]
			CALL Bootes.Devices.Disk.PartitionRead, SI,[CurrentFATState.FileBuffer+2],[CurrentFATState.FileBuffer], \
													0,[CurrentFATState.RootDirSector],BX
			OR AX,AX
			JNZ @f
				XOR EAX,EAX
				JMP .Exit
			@@:
			; Termino con 0
			SHL BX,9
			ADD BX,[CurrentFATState.FileBuffer]
			PUSH [CurrentFATState.FileBuffer+2]
			POP DS
			MOV WORD [DS:BX],0
			PUSH WORD 0
			POP DS
			; Esco indcando che ho caricato tutto
			MOV EAX,-1
			JMP .Exit
		.Root32:
			MOV EAX,[CurrentFATState.RootCluster]
			MOV BX,AX
			SHR EAX,16
			CALL Bootes.IO.FileSystems.FAT.ReadClusters, SI,32,AX,BX,CurrentFATState.FileBufferSize
			JMP .Exit
	.eRootDir:
		MOV BX,AX
		SHR EAX,16
		MOVZX DI,BYTE [CurrentFATState.FATType]
		CALL Bootes.IO.FileSystems.FAT.ReadClusters, SI,DI,AX,BX,CurrentFATState.FileBufferSize

	.Exit:
	POP DI
	POP SI
	POP BX
Return


; Legge i cluster collegati tra loro nella FAT e li scrive nel buffer senza eccedere 
; dalla dimensione massima specificata.
Function Bootes.IO.FileSystems.FAT.ReadClusters, Partition,FATType,NHi,NLo,MaxSize
.FbLo	DW ?
.FbHi	DW ?
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	MOV SI,[Partition]
	MOV DI,[CurrentPartition.FSInfo]
	MOV BX,[NHi]
	SHL EBX,16
	MOV BX,[NLo]
	XOR EDX,EDX
	MOV CX,[CurrentFATState.FileBuffer]
	MOV AX,[CurrentFATState.FileBuffer+2]
	MOV [.FbLo],CX
	MOV [.FbHi],AX
	.lReadCluster:
		; Controllo dimensione massima
		MOV CX,[MaxSize]
		CMP CX,0
		JE @f
			MOV AX,[.FbLo]
			SUB AX,[CurrentFATState.FileBuffer]
			CMP CX,AX
			JA @f
			MOV EAX,EBX
			JMP .Exit
		@@:
		; Ricavo numero di settore FAT
		MOV ECX,EBX
		CMP [FATType],16
		JA .GetFATSec32
		JE .GetFATSec16
		.GetFATSec12:
			SHR CX,1
			ADD CX,BX								; N * 1.5
			JMP .xGetFATSec
		.GetFATSec16:
			SHL CX,1								; N * 2
			JMP .xGetFATSec
		.GetFATSec32:
			SHL ECX,2								; N * 4
		.xGetFATSec:
		SHR ECX,9									; (.) / 512
		ADD ECX,[CurrentFATState.FATSector]
		; Leggo il settore di FAT
		CMP ECX,EDX
		JE @f
			MOV EAX,ECX
			MOV EDX,ECX
			SHR EAX,16
			CALL Bootes.Devices.Disk.PartitionRead, SI,[CurrentFATState.FATBuffer+2],[CurrentFATState.FATBuffer],AX,CX,1
			OR AX,AX
			JZ .Error
		@@:
		; Computo la posizione dei settori
		MOVZX CX,BYTE [CurrentFATState.SectorsPerCluster]
		MOV EAX,EBX
		PUSH BX
		MOVZX EBX,CX
		SHR CX,1
		SUB EAX,2
		SHL EAX,CL
		ADD EAX,[CurrentFATState.DataSector]
		MOV CX,AX
		SHR EAX,16
		; Leggo un cluster del file
		CALL Bootes.Devices.Disk.PartitionRead, SI,[.FbHi],[.FbLo],AX,CX,BX
		SHL BX,9
		ADD WORD [.FbLo],BX
		POP BX
		OR AX,AX
		JZ .Error
		; Imposto ES
		MOV AX,[.FbHi]
		SHL AX,12
		MOV ES,AX
		; Prossimo cluster
		CMP [FATType],16
		JA .NextClusterOff32
		JE .NextClusterOff16
		.NextClusterOff12:
			MOV CX,BX
			SHR BX,1
			ADD BX,CX
			AND BX,111111111b						; (N * 1.5) % 512
			ADD BX,WORD [CurrentFATState.FATBuffer]
			MOVZX EAX,WORD [ES:BX]
			TEST CX,1
			JZ .eOdd
			.iOdd:
				SHR AX,4
				JMP .xOdd
			.eOdd:
				AND AX,0x0FFF
			.xOdd:
			CMP AX,0x0FF8
			JAE .ExitEnd
			JMP .xNextClusterOff
		.NextClusterOff16:
			SHL BX,1
			AND BX,111111111b						; (N * 2) % 512
			ADD BX,WORD [CurrentFATState.FATBuffer]
			MOVZX EAX,WORD [ES:BX]
			CMP AX,0xFFF8
			JAE .ExitEnd
			JMP .xNextClusterOff
		.NextClusterOff32:
			SHL BX,2
			AND BX,111111111b						; (N * 4) % 512
			ADD BX,WORD [CurrentFATState.FATBuffer]
			MOV EAX,[ES:BX]
			CMP EAX,0x0FFFFFF8
			JAE .ExitEnd
		.xNextClusterOff:
		MOV BX,WORD [.FbLo]
		MOV [ES:BX], WORD 0
		; Riptistino ES
		PUSH WORD 0
		POP ES
		; Prossimo cluster
		MOV EBX,EAX
		JMP .lReadCluster
	.xReadCluster:
	JMP .Exit
	
	.Error:
	XOR EAX,EAX
	JMP .Exit
	.ExitEnd:
	; Riptistino ES
	PUSH WORD 0
	POP ES
	MOV EAX,-1
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Data
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Bootes.IO.FileSystems.FAT.Data:
	.ShortName				DB "           ",0