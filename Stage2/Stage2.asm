;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes Stage2
;
; Lo Stage2 carica il kernel da una partizione specificata.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
use16
org Bootes.Stage2.Info.Origin

include "..\Shared\DebugMacro.inc"
include "..\Shared\Shared.inc"
include "Bootes.Stage2.Application.inc"


;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Start
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Bootes.Stage2.Start:
	; --------------------------------------------------------------
	; Inizializzo i segmenti
	; --------------------------------------------------------------
	JMP 0:@f
	@@:
	CLI
	XOR AX,AX
	MOV DS,AX
	MOV ES,AX
	MOV GS,AX
	MOV SS,AX
	MOV AX,0xFFFF
	MOV SP,AX
	
	; LockScheduler
	MOV BYTE [Bootes.Scheduler.Handler.LockOperation],1

	; --------------------------------------------------------------
	; Avvio sistema
	; --------------------------------------------------------------
	; System
	CALL Bootes.Memory.Init
	if Bootes.Stage2.Info.MultitaskSupport = 1
		CALL Bootes.Task.Start, .MainLoop,Bootes.Stage2.Info.TaskMaxStackSize
		ADD AX,18
		MOV SP,AX
	else
		CALL Bootes.Memory.Alloc,Bootes.Stage2.Info.TaskMaxStackSize+24
		ADD AX,Bootes.Stage2.Info.TaskMaxStackSize+24
		MOV SP,AX
	end if
	CALL Bootes.Scheduler.Init
	CALL Bootes.String.Init
	CALL Bootes.Time.Init
	CALL Bootes.Boot.Init
	CALL Bootes.Machine.Init
	; Script
	CALL Bootes.Script.Init
	; Input/output
	CALL Bootes.IO.FileSystems.Init
	CALL Bootes.IO.Console.Init
	; Devices
	CALL Bootes.Devices.Keyboard.Init
	CALL Bootes.Devices.Disk.Init
	CALL Bootes.Devices.Video.Init
	; Namespace principale
	CALL Bootes.Script.Comander.AddNamespace, Main_Namespace
	; Avvio il sistema
	if Bootes.Stage2.Info.MultitaskSupport = 1
		; Avvio Processi
		CALL Bootes.Task.Start, Bootes.Time.Timer, Bootes.Stage2.Info.TaskMaxStackSize
		CALL Bootes.Task.Start, Bootes.IO.Console, Bootes.Stage2.Info.TaskMaxStackSize
		; TaskUnlcok
		MOV BYTE [Bootes.Scheduler.Handler.LockOperation],0
		; Avvio
		IRET
	else
		CALL Bootes.IO.Console
		JMP $
	end if
	
	; --------------------------------------------------------------
	; Ciclo principale dello Stage2
	; --------------------------------------------------------------
	.MainLoop:
		@@:
			; TODO: Garbage collect nei momenti di inattivita (?)


		JMP @b
		
		
	
Namespace Main_Namespace, Bootes.SystemName

HelpRemarks	"Funzione di prova.\n\tQuesta funzione hasha una stringa e restituisce il risultato."
HelpParams	1,"str1","Prima stringa"
;HelpReturn	0,"Descrizione return value."
HelpSeeAlso	Bootes.Memory.InfoMemory.FunctionLink
HelpReturn 2,"Hash della stringa"
Public
Function CalcolaHash, str1
Begin
;	CALL Bootes.IO.Console.Write, Domanda
;	CALL Bootes.IO.Console.Read, 20
	CALL Bootes.String.Hash, [str1]
	PUSH AX
	CALL Bootes.String.FromNumber, Risposta+12,0,AX,1,10
	CALL Bootes.IO.Console.WriteLine, Risposta
	POP AX
Return

	Domanda DB "Stringa di cui fare l'hash: ",0
	Risposta DB "Hash value:           ",0
;	Accapo DB "\n",0


Public
Function WaitMe, t
Begin
	CALL Bootes.Time.Wait, [t]
	CALL Bootes.IO.Standard.Output, OSName
Return

Public
Function TestFileRead, FName
Begin
	CALL Bootes.IO.File.Read, [FName]
	OR AX,AX
	JZ @f
	CALL Bootes.IO.Standard.Output, AX
	CALL Bootes.Memory.Free, AX
	@@:
Return

Public
Function TestLongMode
Begin
	PUSH BP
	CALL Bootes.Machine.ToProtectMode
	use32

	CALL Bootes.Machine.ToLongMode
	use64
	
	MOV RDI,0xB8000
	MOV RAX,'O K ! !'
	MOV [RDI],RAX
	
	JMP $
	
	CALL Bootes.Machine.ToRealMode
	use16
	POP BP
Return

Public
Function GoDraw!
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	
	CALL Bootes.Devices.Video.SetMode, 0x100
	
	MOV AX,0x0C33
	MOV BH,0
	MOV CX,100
	MOV DX,CX
	@@:
	INT 0x10
	LOOP @b
	
	POP DX
	POP CX
	POP BX
Return

OSName			DB "multiboot",0
FileToBeReaded	DB "(fd1)\bootes\mboot.bin",0



;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Include
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 4
; System
include "System\Types.inc"
include "System\Scheduler.inc"
include "System\Task.inc"
include "System\Error.inc"
include "System\Memory.inc"
include "System\Machine.inc"
include "System\String.inc"
include "System\Time.inc"
include "System\Interrupt.inc"
include "System\Boot\Boot.inc"
include "System\Graphic.inc"
; Script
include "Script\Script.inc"
; Devices
include "Devices\Disk.inc"
include "Devices\Keyboard.inc"
include "Devices\Video\Video.inc"
; IO
include "IO\FileSystems\FileSystems.inc"
include "IO\Standard.inc"
include "IO\Console.inc"
include "IO\File.inc"

align 2
Bootes.Stage2.End:

; Padding del file a 64kB
if Bootes.Stage2.Info.64kPadding = 1
	times 0x10000-(Bootes.Stage2.End-Bootes.Stage2.Start) DB 0
end if

; Output informativo: Numero di settori dello Stage2
ReadSec = ((Bootes.Stage2.End-Bootes.Stage2.Start)/512+1)
Debug.Print.String "Settori da 512byte da leggere: ",ReadSec
if Bootes.Stage2.Info.SectorsCount <> ReadSec
	Debug.Print.String "AGGIORNARE Bootes.Stage2.Info.SectorsCount in Shared.inc"
end if