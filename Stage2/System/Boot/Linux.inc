;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Boot.Linux
;
; Gestisce le funzioni di boot per kernel linux.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


; --------------------------------------------------------------
; Strutture
; --------------------------------------------------------------
STRUC Bootes.Boot.Linux.Header
{
	.Code1					RB 0x20
	.Magic1					DW 0xA33F
	.CommandLineOffset		DW ?
	.Code2					RB (0x01F1-0x20-2-2)
	.SetupSectorsCount		DB ?
	.RootFlags				DW ?
	.SysSize				DW ?			; Obsoleto
	.SwapDev				DW ?			; Obsoleto
	.RamSize				DW ?			; Obsoleto
	.VideoMode				DW ?
	.RootDevice				DW ?
	.BootSignature			DW Bootes.Stage1.Info.BootSignature
	;
	.Jump					DW ?
	.Magic2					DD 0x53726448	; "HdrS"
	.Version				DW ?
	.RealModeHook			DD ?
	.StartSys				DW ?			; Obsoleto
	.KernelVersion			DW ?
	.BootLoaderType			DB ?
	.LoadFlags				DB ?
	.SetupMoveSize			DW ?
	.Entrypoint				DD ?
	.RamdiskAddr			DD ?
	.RamdiskSize			DD ?
	.BootsectorKludge		DD ?			; Obsoleto
	.HeapEndAddr			DW ?
	.Pad1					DW ?
	.CommandLine			DD ?
	.InitrdAddr				DD ?
}


Function Bootes.Boot.Linux.CheckKernel, Entry,BufferHi,BufferLo
Begin
	MOV AX,[BufferHi]
	
Return