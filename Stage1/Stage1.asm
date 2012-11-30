;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Boot
;
;REMARKS-------------------------------------------------------------------------------------------
;  Il sistema inizia da qui!
;  Lo Stage1 carica lo Stage2 che a sua volta caricherà il kernel.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
org Bootes.Stage1.Info.Origin
use16
include "..\Shared\DebugMacro.inc"
include "..\Shared\Shared.inc"

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

VIRTUAL AT BX
	PEntry		Bootes.Devices.Disk.PartitionEntry
END VIRTUAL



;============================================================================================= CODE
Bootes.Stage1.Start:
	; --------------------------------------------------------------
	; BIOS Parameters Block
	; --------------------------------------------------------------
	JMP	Bootes.Stage1.Init
	NOP
	
	if Bootes.Stage1.Info.FileSystem eq FAT12
	; FAT12 Header
		OEMName				DB "MSDOS5.0"
		BytesPerSector		DW 512
		SectorPerCluster	DB 1
		ReservedSectors		DW 1
		NumFATs				DB 2
		RootEntryCount		DW 224
		TotSec16			DW 2880
		Media				DB 0xF0
		FATsz16				DW 9
		SectorsPerTrack		DW 18
		NumHeads			DW 2
		HiddSec				DD 0
		TotSec32			DD 0
		DrivNum				DB 0
							DB 0
							DB 0x29
		VolID				DD 0x502EE8A8
		VolLabel			DB "NO NAME    "
		FileSysType			DB "FAT12   "
	else if Bootes.Stage1.Info.FileSystem eq FAT32
	; FAT32 Header
		file "Binary\FAT32.bin":3,90-3
	end if
	
;	times (Bootes.Stage1.Info.BPB.End-($-Bootes.Stage1.Start)) DB 0


Bootes.Stage1.Init:
;====================================================================
; (1) Inizilizzazione sistema
;====================================================================
	JMP 0:@f
	@@:
	; --------------------------------------------------------------
	; Inizializzo stack e data segment
	; --------------------------------------------------------------
	CLI
	XOR	AX,AX
	MOV	SS,AX
	MOV	SP,0x7C00
	MOV	DS,AX
	MOV ES,AX
	STI

	; --------------------------------------------------------------
	; Mostro il messaggio di attesa
	; --------------------------------------------------------------
	MOV	SI,Bootes.Stage1.Data.Loading
	CALL Bootes.Stage1.Print

;====================================================================
; (3) Carico lo Stage2 o il Loader
;====================================================================
	; Seleziono il drive
	MOV AL,[Bootes.Stage1.Data.Stage2.Drive]
	CMP AL,0xFF
	JE @f
		MOV DL,AL
	@@:
	; Seleziono la motalità di lettura
	CALL Bootes.Stage1.SetReadMode
	OR AX,AX
	JZ .iLoadError
	; Leggo lo Stage2
	if ~ Bootes.Stage1.Info.Loader.Use = 1
		MOV BX,Bootes.Stage2.Info.Origin
		MOV EAX,[Bootes.Stage1.Data.Stage2.StartSector]
		MOV CX,[Bootes.Stage1.Data.Stage2.Sectors]
		@@:
			; Comando di lettura
			CALL Bootes.Stage1.ReadSector
			OR AX,AX
			JZ .iLoadError
			; Prossimo settore da leggere
			ADD BX,512
			LOOP @b
	
	; Leggo il Loader
	else
		; Seleziono il settore da leggere
		if Bootes.Stage1.Info.Loader.Sector = 0
			MOV BX,Bootes.Stage1.PartitionTable
			MOV CX,4
			.lTestActive:
				CMP BYTE [PEntry.Flag],0x80
				JE .xTestActive
				ADD BX,PEntry.Size
				LOOP .lTestActive
			JMP .iLoadError
			.xTestActive:
			MOV EAX,DWORD [PEntry.Start]
		else
			MOV EAX,Bootes.Stage1.Info.Loader.Sector
		end if
		; Lettura
		MOV BX,Bootes.Stage1.End
		CALL Bootes.Stage1.ReadSector
		OR AX,AX
		JZ .iLoadError
		; Imposto la funzione di lettura per il Loader
		MOV WORD [Bootes.Stage1.End+512-5],Bootes.Stage1.ReadSector
		; Imposto il descrittore del filesystem
		if ~ Bootes.Stage1.Info.FileSystem eq NONE
			MOV BX,Bootes.Stage1.Start
		else
			MOV BX,0
		end if
	end if
	
	JMP .Jump
	.iLoadError:
		MOV SI,Bootes.Stage1.Data.LoadError
		CALL Bootes.Stage1.Print
		JMP $

;====================================================================
; (4) Salto allo Stage2 o al Loader
;====================================================================
	.Jump:
	if ~ Bootes.Stage1.Info.Loader.Use = 1
	JMP Bootes.Stage2.Info.Origin
	else
	JMP WORD [Bootes.Stage1.End+512-3]
	end if


;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; private static void                                                                   Bootes.Stage1.Print
;----------------------------------------------------------------------------------------------------
; reg SI: Indirizzo della stringa terminata da 0 da scrivere.
;----------------------------------------------------------------------------------------------------
; Stampa a viseo una stringa puntata da DS:[E]SI utilizzando l'INT 0x10.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Bootes.Stage1.Print:
	 MOV AH,0x0E		 ; Per chiamare la funzione di sctrittura carattere tramite int 0x10
	 MOV BX,0x0007		 ; Imposta lo sfondo del carattere
	 .loop:
	 LODSB			 ; LOaD Single Byte da DS:[E]SI in AL
	 OR AL,AL
	 JZ .xloop
	 INT 0x10
	 JMP .loop
	 .xloop:
	 RET

;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; private static void                                                                    SetReadMode
;
;PARAMS----------------------------------------------------------------------------------------------
;	DL:	Drive dal quale leggere.
;
;RETURN----------------------------------------------------------------------------------------------
;	Zero se si è verificato un errore altrimenti 1.
;
;REMARKS---------------------------------------------------------------------------------------------
;	Seleziona la funzione di lettura corretta per il dirive indicato.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Bootes.Stage1.SetReadMode:
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DX
	PUSH ES
	
	; Se non è un HD uso CHS
	TEST DL,0x80
	JZ .CHSMode
	; Verifico che LBA sia supportato
	PUSH DX
	MOV AX,0x4100
	MOV BX,0x55AA
	INT 0x13
	POP DX
	JC .CHSMode
	CMP BX,0xAA55
	JNE .CHSMode
	; Verifico che INT 0x13 AH=0x42 sia supportato
	AND CX,1
	JZ .CHSMode
	.LBAMode:
		; Tento una lettura
		MOV SI,Bootes.Stage1.ReadSectorLBA.DiskAddressPacket
		MOV AX,0x4200
		INT 0x13
		JC .CHSMode
		; Metodo funzionante
		MOV WORD [Bootes.Stage1.ReadSector.Function],Bootes.Stage1.ReadSectorLBA
		JMP .Exit
	.CHSMode:
		; Provo a determinare con l'INT 0x13 ah=0x08
		MOV AH,0x08
		INT 0x13
		JNC .SaveProbe
		; Verifico nuovamente di non avere un HD
		TEST DL,0x80
		JNZ .iDriveError
		; Floppy
		.FloppyProbe:
			MOV SI,.ProbeValues
			MOV CL,0
			@@:
			; Resetta il floppy
			MOV AX,0
			INT 0x13
			; Prossima ProbeValue
			MOV CL,[SI]
			INC SI
			CMP CL,0
			JE .iDriveError
			; Tento una lettura
			MOV BX,Bootes.Stage2.Info.Origin
			MOV AX,0x0201
			MOV CH,0
			MOV DH,0
			INT 0x13
			JC @b
			MOV DH,1
		.SaveProbe:
		; Salvo il numero di settori per traccia
		AND CX,00111111b
		MOV [Bootes.Stage1.ReadSectorCHS.SecPerTrack],CX
		; Salvo il numero di testine
		MOVZX AX,DH
		INC AX
		MOV [Bootes.Stage1.ReadSectorCHS.Heads],AX
		.Done:
		MOV WORD [Bootes.Stage1.ReadSector.Function],Bootes.Stage1.ReadSectorCHS
	.Exit:
	MOV AX,1
	JMP .End
	
	.iDriveError:
	MOV AX,0
	
	.End:
	POP ES
	POP DX
	MOV [Bootes.Stage1.ReadSector.Drive],DL
	POP SI
	POP CX
	POP BX
	RET
	
	.ProbeValues	DB 36, 18, 15, 9, 0
	

;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; private static void                                                                     ReadSector
;
;PARAMS----------------------------------------------------------------------------------------------
;	EAX: Settore da leggere (LBA).
;	ES:BX: Dove leggere il dato.
;
;RETURN----------------------------------------------------------------------------------------------
;	EAX vale 0 se è avvenuto un errore. Altrimenti contiene il successivo settore.
;
;REMARKS---------------------------------------------------------------------------------------------
;	Legge un settore dal dive impostato con SetReadMode.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Bootes.Stage1.ReadSector:
	PUSH EAX
	PUSH DX
	
	MOV DL,[.Drive]
	CALL [.Function]
	
	POP DX
	; Controllo se le funzioni hanno fatto un errore
	OR AX,AX
	JZ .Error
	POP EAX
	INC EAX
	RET
	
	.Error:
	POP EAX
	XOR AX,AX
	RET
	
	.Function	DW ?
	.Drive		DB ?
	
	

Bootes.Stage1.ReadSectorCHS:
	PUSH CX
	PUSH DX
	
	XOR DX,DX
	DIV WORD [.SecPerTrack]
	INC DL
	MOV CL,DL	; Settore ricavato!

	XOR DX,DX
	DIV	WORD [.Heads]
	MOV CH,AL	; Traccia
	MOV AL,DL	; Head
	POP DX
	MOV DH,AL

	MOV AX,0x0201	; Per preparare la funzione di lettura con l'INT 0x13
	INT 0x13
	JC .iLoadError
	JMP .End
	
	.iLoadError:
	XOR AX,AX
	
	.End:
	POP CX
	RET
	
	.Heads			DW ?
	.SecPerTrack	DW ?
	

Bootes.Stage1.ReadSectorLBA:
	PUSH SI
	MOV [.DiskAddressPacket.Start],EAX
	MOV [.DiskAddressPacket.Buffer],BX
	MOV SI,.DiskAddressPacket
	MOV AX,0x4200
	INT 0x13
	POP SI
	JC .iLoadError
	MOV AX,1
	RET
	.iLoadError:
	XOR AX,AX
	RET
	
	.DiskAddressPacket:
	.DiskAddressPacket.Size		DB 0x10, 0
	.DiskAddressPacket.Blocks	DW 1
	.DiskAddressPacket.Buffer	DW Bootes.Stage2.Info.Origin,0
	.DiskAddressPacket.Start	DD 0,0
	

;============================================================================================== DATA
Bootes.Stage1.Data:
    .Loading	 DB 13,10,'Loading...',0
    .LoadError	 DB 'Error',0

	; Stage2 sectors
	Debug.Print.Addr "Drive byte"
	.Stage2.Drive	DB 0xFF	; Specifica il drive dello Stage2.
					; 0xFF per specificare il drive di boot.

	if ~ Bootes.Stage1.Info.Loader.Use = 1
	Debug.Print.Addr "Stage2 LBA sectors"
	; Settori che lo Stage1 deve leggere
	.Stage2.StartSector DD Bootes.Stage2.Info.StartSector	; Inizio
	.Stage2.Sectors		DW Bootes.Stage2.Info.SectorsCount	; Numero settori
	end if

;===================================================================
; Padding
;===================================================================
BootSectFree = ((512-(Bootes.Stage1.End-Bootes.Stage1.PartitionTable))-($-Bootes.Stage1.Start))
times BootSectFree DB 0
Debug.Print.String "Spazio codice disponinile: ",BootSectFree," bytes"

;===================================================================
; Partitions
;===================================================================
Bootes.Stage1.PartitionTable:
if Bootes.Stage1.Info.UsePartitionTable = 1
	file "Binary\MBR.bin":446,64
end if
	
;===================================================================
; Signature
;===================================================================
DW Bootes.Stage1.Info.BootSignature

Bootes.Stage1.End: