Bootes.SystemName		equ "iOS"

Bootes.Stage1.Info:
	.Origin				= 0x7C00
	.BPB.Start		= 0x03
	.BPB.End			= 0x5A
	.UsePartitionTable	= 0
	.BootSignature		= 0xAA55
	; Loaders
	.Loader.Use			= 0
	.Loader.Sector	= 1				; 0 Indica il primo settore della partizione attiva.
Bootes.Stage1.Info.FileSystem			equ FAT12
	
Bootes.Stage2.Info:
	; General
	.Origin				= 0x500
	.64kPadding			= 0
	.StartSector		= 1 ;33		; Utilizzato solo se viene specificato Loader.Use = 0
	.SectorsCount		= 0x2D
	.DefaultVideoMode	= 3			; 0 Per modalità predefinita, 3 per modalità testuale
	; Optional support
	.ErrorSupport		= 1
	.HelpSupport		= 1
	.MultitaskSupport	= 1
	.GraphicSupport		= 1
	; Memory
	.MaxMemoryBlocks	= .MaxTasks*3+100
	.PModeStack			= 0xFFFF
	.LModeStack			= 0xFFFF
	.PageMapLevel4		= 0x70000
	.DefaultStringLen	= 512
	.MaxFileSize		= 1024
	.BackFrameBuffer	= 1024*1024*4
	; Tasks
	.MaxTasks			= 10
	.TaskMaxStackSize	= 512-24
	; Script
	.MaxUsing			= 10	; Numero massimo di using
	.MaxVars			= 20	; Numero massimo di variabili
	.MaxStringVarLen	= 50	; Numero massimo di caratteri per una variabile stringa + 1
	.MaxFuncParameters	= 10	; Numero massimo di parametri per una funzione.
	; Console
	.InputColor			= 7
	.OutputColor		= 7
	.LineColor			= 8
	.ErrorColor			= 4
	.StringColor		= 8
	; Code security
	.MaxCicles			= 2000	; Numero massimo di cicli permessi dalla macro InfiniteLoopCheck
Bootes.Stage2.Info.FileName	equ "STAGE2"


;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Code macros
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

; InfiniteLoopCheck
; CX NON deve essere usato nel ciclo e deve essere inizializzato a 0.
MACRO InfiniteLoopCheck ExitTo
{
	INC CX
	CMP CX,Bootes.Stage2.Info.MaxCicles
	JAE ExitTo
}

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Multiboot structs
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
STRUC Multiboot.Info
{
	.Flags			DD ?
	; Con bit 0 dei flag settato
	.LowerMemory	DD ?	; In kB, max: 640 kB
	.UpperMemory	DD ?	; Memoria sopra 1 megabytes
	; Con bit 1 dei flag settato
	.BootDevice:
		.BootDevice.Drive	DB ?	; BIOS drive number
		.BootDevice.Part1	DB ?	; Top level partition (0xFF not used)
		.BootDevice.Part2	DB ?	; Sub level partition (0xFF not used)
		.BootDevice.Part3	DB ?	;
	; Con bit 2 dei flag settato
	.CommandLine	DD ?	; Indirizzo di una stringa terminata da 0
	; Con bit 3 dei flag settato
	.ModsCount		DD ?
	.ModsAddr		DD ?
	; Con bit 4 o 5 dei flag settato
	.Syms			RD 4
	; Con bit 6 dei flag settato
	.MemMapLength	DD ?
	.MemMapAddr		DD ?
	; Con bit 7 dei flag settato
	.DrivesLength	DD ?
	.DrivesAddr		DD ?	; Puntatore a strutture Multiboot.Drives.Info
	; Con bit 8 dei flag settato
	.ConfigTable	DD ?	; ROM config table
	; Con bit 9 dei flag settato
	.BootLoaderName	DD ?
	; Con bit 10 dei flag settato
	.AMPTableAddr	DD ?	; Puntatore alla struttura Multiboot.AMP.Info
	; Con bit 11 dei flag settato
	.VBEInfo		DD ?	; Indirizzo alla struttura Vesa.Info
	.VBEModeInfo	DD ?	; Indirizzo alla struttura Vesa.Mode.Info
	.VBEMode		DD ?
	.VBEInterfaceSeg	DD ?
	.VBEInterfaceOffset	DD ?
	.VBEInterfaceLength	DD ?
}

STRUC Multiboot.Drives.Info
{
	.Size			DD ?	; Dimensioni di questa struttura
	.DriveNumber	DB ?	; BIOS Drive number
	.DriveMode		DB ?	; 0-CHS	1-LBA
	.DriveCylinders	DW ?
	.DriveHeads		DB ?
	.DriveSectors	DB ?
	.DrivePorts:			; Collezione di word terminata da 0 
}

STRUC Multiboot.AMP.Info	; Advanced Power Management
{
	.Version		DW ?
	.CSeg			DW ?
	.Offset			DD ?
	.CSeg16			DW ?
	.DSeg			DW ?
	.Flags			DW ?
	.CSegLength		DW ?
	.CSeg16Length	DW ?
	.DSegLength		DW ?
}