;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Boot.Multiboot
;
; Gestisce le funzioni di boot per kernel multiboot.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


; --------------------------------------------------------------
; Strutture
; --------------------------------------------------------------
STRUC Bootes.Boot.Multiboot.Header
{
	.Start:
	.Magic					DD 0x1BADB002
	.Flags					DD ?
							; Flags
							; bit 0 - Allinea a 4Kb
							; bit 1 - Passare informazioni sulla memoria
							; bit 2 - Passare informazioni sulla modalità video
							
	.Checksum				DD ?			; 0-[.Flags]-[.Magic]
	; Con Flags[16] a 1
	.HeaderAddr				DD ?
	.LoadAddr				DD ?
	.LoadEndAddr			DD ?
	.BssEndAddr				DD ?
	.EntryAddr				DD ?
	; Con Flags[2] a 1
	.ModeType				DD ?
	.Width					DD ?
	.Height					DD ?
	.Depth					DD ?
	
	.Size					= $-.Start
}

STRUC Bootes.Boot.Multiboot.Info
{
	.Flags					DD ?
	; Con Flags[0] a 1
	.MemLower				DD ?
	.MemUpper				DD ?
	; Con Flags[1] a 1
	.BootDevice				DD ?	; 0 Drive | Part1 | Part2 | Part3 31
									; Si indica la partizione ed eventuale sub partizione oppure 0xFF
	; Con Flags[2] a 1
	.CommandLine			DD ?
	; Con Flags[3] a 1
	.ModulesCount			DD ?
	.ModulesAddr			DD ?	; Puntatore al primo Bootes.Boot.Multiboot.Modules
	; Con Flags[4] o Flags[5] a 1
	.TabSize:
	.Num:					DD ?
	.StrSize:
	.Size					DD ?
	.Addr					DD ?
	.Reserved:
	.Shndx					DD ?
	; Con Flags[6] a 1
	.MemoryMapLength		DD ?
	.MemoryMapAddr			DD ?
	; Con Flags[7] a 1
	.DrivesLength			DD ?
	.DrivesAddr				DD ?
	; Con Flags[8] a 1
	.ConfigTable			DD ?
	; Con Flags[9] a 1
	.BootLoaderName			DD ?
	; Con Flags[10] a 1
	.APMTable				DD ?
	; Con Flags[11] a 1
	.VBEControlInfo			DD ?
	.VBEModeInfo			DD ?
	.VBEMode				DW ?
	.VBEInterfaceSegment	DW ?
	.VBEInterfaceOffset		DW ?
	.VBEInterfaceLength		DW ?
}

STRUC Bootes.Boot.Multiboot.Modules
{
	.ModulesStart			DD ?
	.ModulesEnd				DD ?
	.String					DD ?
	.Reserved				DD ?
}

STRUC Bootes.Boot.Multiboot.ELF.Header
{
	.Ident					RB 16		; Ident[4]=1?
	.Type					DW ?
	.Machine				DW ?
	.Version				DD ?
	.Entry					DD ?
	.ProgramTableOffset		DD ?
	.SectionHeaderOffset	DD ?
	.Flags					DD ?
	.Size					DW ?
	.ProgramTableSize		DW ?
	.ProgramTableCount		DW ?
	.SectionHeaderSize		DW ?
	.SectionHeaderCount		DW ?
	.SectionHeaderString	DW ?
}


VIRTUAL AT SI
	MultibootHeader				Bootes.Boot.Multiboot.Header
END VIRTUAL


; --------------------------------------------------------------
; Descrizione stato al boot
; --------------------------------------------------------------
; EAX	- Magic number 0x2BADB002
; EBX	- Puntatore all'indirizzo fisico della struttura di info
; CS-SS	- Segmenti appropriati
; A20	- Abilitato
; CR0	- Bit 31 (pagging) disabilitato, bit 0 (Pmode) abilitato
; EFLAGS- Bit 17 (VM) e 9 (IF) disabilitati




;====================================================================
; Controlla se i dati contenuti nel buffer (di dimensione minima 8KB)
; descrivono un kernel multiboot.
; --------------------------------------------------------------
; PARAMS
;	Entry	Puntatore ad una struttura [Boot.inc]OSEntry descrivente
;			il sistema operativo per il quale caricare il kernel.
;	Buffer	Puntatore alla zona di memoria fisica alla quale sono
;			stati caricati i dati del kernel.
; RETURN
;	Zero se i dati nel buffer non appartengono a un kernel multiboot 
;	altrimenti non-zero.
;====================================================================
Function Bootes.Boot.Multiboot.CheckKernel, Entry,BufferHi,BufferLo
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	; Preparo il puntatore al buffer
	MOV SI,[BufferHi]
	SHL ESI,16
	MOV SI,[BufferLo]
	; Alloco memoria dove eventualmente salvare l'header multiboot
	CALL Bootes.Memory.Alloc, MultibootHeader.Size
	MOVZX EDX,AX
	ProtectMode
	MOV ECX,Bootes.Boot.Multiboot.Data.MaxSearchLength/4
	MOV EAX,Bootes.Boot.Multiboot.Data.HeaderMagic
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
		MOV EDI,EDX
		MOV ECX,MultibootHeader.Size/4
		REP MOVSD
		MOV ESI,EDX
		MOV EAX,[ESI+4]
		INC ECX
		; Imposto indirizzi di caricamento se non presenti
		; TODO: Verifcare header ELF se flag 16 a 0
	@@:
	RealMode
	CMP CX,0
	JE .Error
	; Salvo la posizione dell'header multiboot copiato
	MOV BX,[Entry]
	MOV WORD [OSEntry.KernelInfo],DX
	; Imposto la funzione di load
	MOV WORD [OSEntry.KernelLoader],Bootes.Boot.Multiboot.LoadKernel
	MOV AX,1
	JMP .Exit
	
	.Error:
	CALL Bootes.Memory.Free, DX
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Carica il kernel specificato dall'entry e inizializza tutte le 
; strutture necessarie preparandole per il boot del kernel stesso.
; --------------------------------------------------------------
; PARAMS
;	Entry	Puntatore ad una struttura [Boot.inc]OSEntry descrivente
;			il sistema operativo per il quale caricare il kernel.
; RETURN
;	Zero se è avvenuto qualche errore in fase di preparazione 
;	altrimenti non-zero.
;====================================================================
Function Bootes.Boot.Multiboot.LoadKernel, Entry
Begin
Return


; --------------------------------------------------------------
; Data
; --------------------------------------------------------------
Bootes.Boot.Multiboot.Data:
	.HeaderMagic					= 0x1BADB002
	.ReturnMagic					= 0x2BADB002
	.MaxSearchLength				= 1024*8		; 8KB