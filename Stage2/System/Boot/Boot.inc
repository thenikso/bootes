;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Boot
;
; Gestisce le funzioni di boot dei sistemi operativi.
; Ogni sistema è identificato da una stringa la quale viene usata come chiave da passare a ogni
; istruzione di boot.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

SysNamespace Bootes.Boot_Namespace, "Boot"
	Export Bootes.Boot.AddOSEntry,1, "AddOSEntry", Type.Null, Type.String
	Export Bootes.Boot.SetBootstrap,2, "SetBootstrap", Type.Null, Type.String, Type.String
	Export Bootes.Boot.SetKernel,2, "SetKernel", Type.Null, Type.String, Type.String
	Export Bootes.Boot.Boot,1, "Boot", Type.Null, Type.String
	Export Bootes.Boot.BootGraphicInterface,2, "BootGraphicInterface", Type.Null, Type.Number, Type.Number

; --------------------------------------------------------------
; Strutture
; --------------------------------------------------------------
STRUC Bootes.Boot.OSEntry
{
	.Start:
	; Chiave
	.KeyAddr		DW ?	; Puntatore alla stringa terminata da zero contenente la chiave
	; Bootstrap system
	.BootPartition	DW ?	; Partizione dalla quale caricare il boot sector (opzionale)
	; Kernel system
	.EntryAddr		DD ?	; Entrypoint del kernel
	.KernelLoader	DW ?	; Puntatore alla funzione che provvederà a caricare il kernel
	.KernelInfo		DW ?	; Puntatore ad una struttura di informazioni
	.KernelFileName	DW ?	; Puntatore alla stringa con il nome del file
	
	.Size		= $-.Start
}

VIRTUAL AT BX
	OSEntry		Bootes.Boot.OSEntry
END VIRTUAL


;====================================================================
; Inizializza il supporto alle funzioni di boot
;====================================================================
align 4
Bootes.Boot.Init:
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Boot_Namespace
RET



;====================================================================
; Aggiunge una entrata per descirivere un nuovo sistema.
; --------------------------------------------------------------
; PARAMS
;	OSKey	Puntatore alla stringa terminata da 0 contenente la 
;			chiave del sistema.
; RETURN
;	Zero se non è stato possibile inserire la chiave, altrimenti
;	non-zero.
;====================================================================
Function Bootes.Boot.AddOSEntry, OSKey
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	MOV SI,[OSKey]
	; --------------------------------------------------------------
	; Recupero posizione nell'array alla quale scrivere
	; --------------------------------------------------------------
	MOV BX,Bootes.Boot.Data.OSEntryArray
	MOV CX,Bootes.Boot.Data.MaxOSEntry
	CALL Bootes.String.Hash, SI
	XOR DX,DX
	DIV CX
	MOV AX,DX
	SUB CX,AX
	MOV DX,OSEntry.Size
	MUL DX
	ADD BX,AX
	; Gestione collisioni
	.lCollision:
		MOV DI,[OSEntry.KeyAddr]
		OR DI,DI
		JZ .xCollision
		CMP DI,1
		JE .xCollision
		; Confronto chiavi
		CALL Bootes.String.Compare, SI,DI
		OR AX,AX
		JZ .Error
		; Prossimo posto
		ADD BX,OSEntry.Size
		LOOP .lCollision
		JMP .Error
	.xCollision:
	
	; --------------------------------------------------------------
	; Inserisco la nuova entrata
	; --------------------------------------------------------------
	CALL Bootes.String.Copy, SI
	MOV [OSEntry.KeyAddr],AX
	; TODO: Azzerare il resto (?)
	
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
; Recupera l'entrata per il sistema specifiacto.
; --------------------------------------------------------------
; PARAMS
;	OSKey	Puntatore alla stringa terminata da 0 contenente la 
;			chiave del sistema.
; RETURN
;	L'indirizzo della struttura che rappresenta il sistema oppure
;	zero se non è stato possibile recuperarla.
;====================================================================
Function Bootes.Boot.GetOSEntry, OSKey
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	MOV SI,[OSKey]
	; --------------------------------------------------------------
	; Recupero posizione nell'array
	; --------------------------------------------------------------
	MOV BX,Bootes.Boot.Data.OSEntryArray
	MOV CX,Bootes.Boot.Data.MaxOSEntry
	CALL Bootes.String.Hash, SI
	XOR DX,DX
	DIV CX
	MOV AX,DX
	SUB CX,AX
	MOV DX,OSEntry.Size
	MUL DX
	ADD BX,AX
	; Gestione collisioni
	.lCollision:
		MOV DI,[OSEntry.KeyAddr]
		OR DI,DI
		JZ .Error
		; Confronto posto già usato
		CMP DI,1
		JE @f
			; Controllo chiave
			CALL Bootes.String.Compare, SI,DI
			OR AX,AX
			JZ .Exit
		@@:
		ADD BX,OSEntry.Size
		LOOP .lCollision
	
	.Error:
	ErrorSetup OSKeyNotFound
	XOR BX,BX
	.Exit:
	MOV AX,BX
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Specifica da quale partizione verrà caricato il settore di boot
; da utilizzare per caricare il sistema indicato.
; --------------------------------------------------------------
; PARAMS
;	OSKey	Puntatore alla stringa terminata da 0 contenente la 
;			chiave del sistema da lanciare.
;	PartitionName	Puntatore alla stringa terminata da 0 contenente
;					il nome della partizione dalla quale leggere il
;					boot sector.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Boot.SetBootstrap, OSKey,PartitionName
Begin
	PUSH BX
	PUSH SI
	
	; --------------------------------------------------------------
	; Recupero la partizione
	; --------------------------------------------------------------
	CALL Bootes.Devices.Disk.GetPartition, [PartitionName]
	OR AX,AX
	JZ .Exit
	MOV BX,AX
	MOV SI,AX
	
	; --------------------------------------------------------------
	; Leggo il primo settore della partizione
	; --------------------------------------------------------------
	CALL Bootes.Devices.Disk.Read, [CurrentPartition.Disk],0,0x7C00,[CurrentPartition.Start+2],[CurrentPartition.Start],1
	OR AX,AX
	JZ .Error
	
	; --------------------------------------------------------------
	; Controllo che il settore caricato sia di boot
	; --------------------------------------------------------------
	CMP WORD [0x7C00+510],Bootes.Stage1.Info.BootSignature
	JNE .Error
	
	; --------------------------------------------------------------
	; Recupero l'entrata del sistema
	; --------------------------------------------------------------
	CALL Bootes.Boot.GetOSEntry, [OSKey]
	OR AX,AX
	JZ .Exit
	XCHG AX,BX
	
	; --------------------------------------------------------------
	; Imposto i parametri necessari
	; --------------------------------------------------------------
	MOV [OSEntry.BootPartition],AX
	JMP .Exit
	
	.Error:
	MOV AX,[PartitionName]
	ErrorSetup InvalidBoot,AX
	MOV AX,0
	.Exit:
	POP SI
	POP BX
Return


;====================================================================
; Carica in memoria il kernel specificato. Il kernel deve attenersi
; alla specifica multiboot per essere caricato.
; --------------------------------------------------------------
; PARAMS
;	OSKey	Puntatore alla stringa terminata da 0 contenente la 
;			chiave del sistema da lanciare.
;	FileName	Puntatore alla stringa terminata da 0 contenente
;				il nome del file da caricare come kernel.
; RETURN
;	Uno se il caricamento è avvenuto con successo altrimenti 0 se
;	è avvenuto un errore.
;====================================================================
Function Bootes.Boot.SetKernel, OSKey,FileName
Begin
	PUSH BX
	
	; --------------------------------------------------------------
	; Recupero l'entrata del sistema
	; --------------------------------------------------------------
	CALL Bootes.Boot.GetOSEntry, [OSKey]
	MOV BX,AX
	OR BX,BX
	JZ .Exit
	
	; --------------------------------------------------------------
	; Copio la stringa in una posizione sicura
	; --------------------------------------------------------------
	CALL Bootes.String.Copy, [FileName]
	OR AX,AX
	JZ .Exit
	
	; --------------------------------------------------------------
	; Imposto i parametri necessari
	; --------------------------------------------------------------
	MOV [OSEntry.KernelFileName],AX
	
	.Exit:
	POP BX
Return



;====================================================================
; Esegue il boot del sistema individuato dalla chiave specificata.
; --------------------------------------------------------------
; PARAMS
;	OSKey	Puntatore alla stringa terminata da 0 contenente la 
;			chiave del sistema da lanciare.
; RETURN
;	Zero se avviene un errore nella fase di boot. Altrimenti la 
;	funzione non ritorna.
;====================================================================
Function Bootes.Boot.Boot, OSKey
Begin
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI

	; --------------------------------------------------------------
	; Recupero l'entrata del sistema
	; --------------------------------------------------------------
	CALL Bootes.Boot.GetOSEntry, [OSKey]
	OR AX,AX
	JZ .Error
	MOV BX,AX
	
	; --------------------------------------------------------------
	; Controllo il tipo di boot da effettuare
	; --------------------------------------------------------------
	CMP WORD [OSEntry.BootPartition],0
	JE .eBootstrap
	.iBootstrap:
		; --------------------------------------------------------------
		; Rieffettuo il boot con un nuovo bootsector
		; --------------------------------------------------------------
		MOV SI,[OSEntry.BootPartition]
		MOV DI,[CurrentPartition.Disk]
		; Leggo il primo settore della partizione
		CALL Bootes.Devices.Disk.Read, DI,0,Bootes.Stage1.Info.Origin,[CurrentPartition.Start+2],[CurrentPartition.Start],1
		OR AX,AX
		JZ .Error
		; Controllo che il settore caricato sia di boot
		CMP WORD [Bootes.Stage1.Info.Origin+510],Bootes.Stage1.Info.BootSignature
		JNE .Error
		; Puliso lo schermo
		CALL Bootes.Devices.Video.Clear
		; Leggo la tabella delle partizioni
		MOV BX,0x600
		CALL Bootes.Devices.Disk.Read, DI,0,BX,0,0,1
		OR AX,AX
		JNZ @f
			; TODO: Reboot (?)
		@@:
		ADD BX,446
		; Azzero l'active flag
		MOV BYTE [BX],0
		MOV BYTE [BX+16],0
		MOV BYTE [BX+16*2],0
		MOV BYTE [BX+16*3],0
		; Preparo i registri per il riavvio del sistema
		MOV SI,[CurrentPartition.Index]
		CMP SI,-1
		JE .eSetPartAddr
		.iSetPartAddr:
			SHL SI,4
			ADD SI,BX
			; Imposto l'active flag
			MOV BYTE [SI],0x80
			JMP .xSetPartAddr
		.eSetPartAddr:
			MOV SI,BX
		.xSetPartAddr:
		MOVZX EDX,WORD [CurrentDisk.DiskID]			; ID disco di avvio (per INT 0x13)
		; Ripristino l'handler origiale del timer
		CALL Bootes.Interrupt.RestoreOriginal
		CALL Bootes.Machine.EnableA20, 0
		; Salto al bootloader
		JMP 0:0x7C00
	.eBootstrap:
		; --------------------------------------------------------------
		; Carico un kernel e lo eseguo		
		; --------------------------------------------------------------
		MOV AX,[OSEntry.KernelFileName]
		OR AX,AX
		JZ .Error
		; Controllo se è un kernel multiboot
		CALL Bootes.String.Copy, AX
		MOV CX,AX
		CALL Bootes.IO.File.ExtendedRead, AX,0,0,(0x2000-512)
		CALL Bootes.Memory.Free, CX
		OR EAX,EAX
		JZ .Exit
		PUSH BX
		ProtectMode
		MOV ESI,EAX
		MOV ECX,(0x2000-512)/4
		MOV EAX,0x1BADB002	; Multiboot magic
		.lMultiMagic:
			CMP [ESI],EAX
			JNE @f
				; Controllo checksum
				XOR EBX,EBX
				SUB EBX,EAX
				SUB EBX,[ESI+4]
				CMP EBX,[ESI+8]
				JE .xMultiMagic
			@@:
			ADD ESI,4
			LOOP .lMultiMagic
			.xMultiMagic:
		; Controllo il riscontro
		CMP ECX,0
		JE @f
			; Se trovato lo copio in una zona di memoria più favorevole
			MOV EDI,Bootes.Stage1.Info.Origin
			MOV ECX,512/4
			REP MOVSD
			MOV ESI,Bootes.Stage1.Info.Origin
			MOV ECX,[ESI+4]
		@@:
		RealMode
		POP BX
		CMP CX,0
		JE .eMultiboot
		.iMultiboot:
			; Imposto tutti i parametri richiesti dalle flags
			TEST ECX,0x10000
			JZ @f
				; TODO: Controllare meglio
				CALL Bootes.IO.File.ExtendedRead, [OSEntry.KernelFileName],[SI+4*4+2],[SI+4*4],0
				MOV EAX,[SI+4*7]
				MOV [OSEntry.EntryAddr],EAX
			@@:
			; TODO: Altre flag
			; TODO: Sistemare i registri
			; Avvio il kernel
			CALL Bootes.Interrupt.RestoreOriginal
			CALL Bootes.Machine.EnableA20, 1
			MOV EAX,[OSEntry.EntryAddr]
			ProtectMode
			JMP EAX
			RealMode
		.eMultiboot:
			; TODO: Caricare altro tipo di kernel (linux?)
		.xMultiboot:
	.xBootstrap:
	
	.Error:
	.Exit:
	MOV AX,0
	POP DI
	POP SI
	POP CX
	POP BX
Return



;====================================================================
; Offre un interfaccia grafica per la selezione dell'OSEntry da 
; lanciare.
; --------------------------------------------------------------
; PARAMS
;	Default		Indice dell'OSEntry da considerare di default.
;	Cooldown	Numero di secondi da aspettare prima che venga
;				avviata l'OSEntry di default. Zero per non
;				porre limiti di tempo.
; RETURN
;	Zero se avviene un errore all'uscita dall'inderfaccia altrimenti
;	non-zero.
;====================================================================
Function Bootes.Boot.BootGraphicInterface, Default, Cooldown
.ListX			DW ?
.ListY			DW ?
.ListWidth		DW ?
.CooldownX		DW ?
.CooldownY		DW ?
Begin
	if Bootes.Stage2.Info.GraphicSupport = 1
	PUSH BX
	PUSH CX
	PUSH DX
	
	; --------------------------------------------------------------
	; Disegno interfaccia
	; --------------------------------------------------------------
	; Pulisco schermo
	CALL Bootes.Graphic.Clear
	; Disegno sfondo
	CALL Bootes.Graphic.GetHeight
	MOV CX,AX
	CALL Bootes.Graphic.GetWidth
	SHR AX,1
	MOV BX,AX
	SHR AX,1
	MOV DX,AX
	SHR DX,1
	SUB BX,DX
	SHR DX,1
	ADD AX,DX
	CALL Bootes.Graphic.SetColor, 0x0011,0x1111
	CALL Bootes.Graphic.FillRect, AX,0,BX,CX
	ADD AX,2
	MOV [.ListY],100
	MOV [.ListX],AX
	MOV [.ListWidth],BX
	; Disegno statusbar (rifare)
	CALL Bootes.Graphic.SetColor, 0x000A,0x0A0A
	MOV DX,CX
	SUB DX,Bootes.Devices.Video.SVGA.Data.CharHeight+4
	CALL Bootes.Graphic.FillRect, AX,DX,BX,Bootes.Devices.Video.SVGA.Data.CharHeight+4
	ADD AX,BX
	ADD DX,2
	SUB AX,Bootes.Devices.Video.SVGA.Data.CharWidth*2+2
	MOV [.CooldownY],DX
	MOV [.CooldownX],AX
	
	
	; --------------------------------------------------------------
	; Interazione
	; --------------------------------------------------------------
	; Scrivo le entry
	CALL Bootes.Graphic.SetColor, 0x002F,0x2F2F
	MOV CX,Bootes.Boot.Data.MaxOSEntry
	MOV BX,Bootes.Boot.Data.OSEntryArray
	MOV AX,[.ListY]
	XOR DI,DI
	.lDrawEntryes:
		CMP WORD [OSEntry.KeyAddr],0
		JE .cDrawEntryes
		CALL Bootes.Graphic.DrawString, [.ListX],AX,[OSEntry.KeyAddr],0
		ADD AX,Bootes.Devices.Video.SVGA.Data.CharHeight
		INC DI
		.cDrawEntryes:
		ADD BX,OSEntry.Size
		LOOP .lDrawEntryes
	.xDrawEntryes:
	; Seleziono quella di default
	OR DI,DI
	JZ .eSelectDefault
		CALL Bootes.Graphic.SetColor, 0x00FF,0xFFFF
		MOV CX,[Default]
		XOR AX,AX
		MOV SI,CX
		OR CX,CX
		JZ @f
			.lMul:
			ADD AX,Bootes.Devices.Video.SVGA.Data.CharHeight+2
			LOOP .lMul
		@@:
		MOV BX,[.ListX]
		ADD AX,[.ListY]
		SUB BX,2
		SUB AX,1
		CALL Bootes.Graphic.DrawRect, BX,AX,[.ListWidth],Bootes.Devices.Video.SVGA.Data.CharHeight+1
		JMP .xSelectDefault
	.eSelectDefault:
		CALL Bootes.Graphic.DrawString, [.ListX],[.ListY],Bootes.Boot.Data.BootMenuNoEntry,0
	.xSelectDefault:
	; Sistemo il cooldown
	MOV DX,[Cooldown]
	CMP DX,-1
	JE .eCooldown
	.iCooldown:
		; Imposto i colori
		CALL Bootes.Graphic.SetColor, 0x00FF,0xFFFF
		CALL Bootes.Graphic.SetClearColor, 0x000A,0x0A0A
		; Scrivo il cooldown
		GetTime
		CALL Bootes.String.FromNumber, Bootes.Boot.Data.CooldownString, 0,DX,2,10
		CALL Bootes.Graphic.DrawString, [.CooldownX],[.CooldownY],Bootes.Boot.Data.CooldownString,1
		MOV BX,[Bootes.Time.Data.Time.Ticks]
		.lInterface:
			; Zero check
			CMP DX,0
			JE .BootSelection
			; Handler tasti
			CALL Bootes.Devices.Keyboard.GetKey
			OR AX,AX
			JZ @f
				; ESC
				CMP AL,27
				JE .Exit
				; Freccie
				; TODO
			@@:
			; Conto alla rovescia
			GetTime
			MOV CX,[Bootes.Time.Data.Time.Ticks]
			SUB CX,BX
			CMP CX,50
			JB @f
				; Decremento contatore cooldown
				DEC DX
				CALL Bootes.String.FromNumber, Bootes.Boot.Data.CooldownString, 0,DX,2,10
				CALL Bootes.Graphic.DrawString, [.CooldownX],[.CooldownY],Bootes.Boot.Data.CooldownString,1
				MOV BX,[Bootes.Time.Data.Time.Ticks]
			@@:
			JMP .lInterface
	.eCooldown:
	.xCooldown:
	
	.BootSelection:
	
	.Exit:
	CALL Bootes.Graphic.Clear
	CALL Bootes.IO.Standard.Output, Bootes.Boot.Data.BootMenuExit
	POP DX
	POP CX
	POP BX
	end if
Return

; --------------------------------------------------------------
; Data
; --------------------------------------------------------------
align 4
Bootes.Boot.Data:
	.MaxOSEntry			= 11
	.CooldownString		DB "00",0,0
	.OSEntryArray		RB .MaxOSEntry*OSEntry.Size
	
	.BootMenuNoEntry	DB "No boot entry in the list",0
	.BootMenuESC		DB "Press ESC to exit menu",0
	.BootMenuExit		DB "Boot menu closed",13,10,0