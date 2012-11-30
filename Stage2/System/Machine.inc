;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Machine
;
; Machine management.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2


; --------------------------------------------------------------
; Exported functions
; --------------------------------------------------------------
SysNamespace Bootes.Machine_Namespace, "Machine"
	Export Bootes.Machine.Shutdown,0, "Shutdown", Type.Null
	Export Bootes.Machine.Reboot,0, "Reboot", Type.Null
	;Export Bootes.Machine.GetMemorySize,0, "GetMemorySize"
	
; --------------------------------------------------------------
; Macros
; --------------------------------------------------------------
MACRO ProtectMode
{
	PUSH BP
	CALL Bootes.Machine.ToProtectMode
	use32
}

MACRO RealMode
{
	CALL Bootes.Machine.ToRealMode
	use16
	POP BP
}


;====================================================================
; Inizializza la macchina
;====================================================================
Bootes.Machine.Init:
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Machine_Namespace
	CALL Bootes.Machine.EnableA20, 1
	RET



;====================================================================
; Riavvia la macchina
;====================================================================
Bootes.Machine.Reboot:
	MOV DI,0x0472
	MOV AX,[DI]
	JMP FWORD 0xFFFF:0x0000



;====================================================================
; Spegnie la macchina
;====================================================================
Bootes.Machine.Shutdown:
	PUSH AX
	PUSH BX
	PUSH CX
	
	; Rileva APM
	MOV AX,0x5300
	XOR BX,BX
	INT 0x15
	JC .NoAPM
	
	; Disconnetto APM
	MOV AX,0x5304
	XOR BX,BX
	INT 0x15
	
	; Connetto APM
	MOV AX,0x5301
	XOR BX,BX
	INT 0x15
	JC .NoAPM
	
	; Imopsta protocollo APM
	MOV AX,0x530E
	XOR BX,BX
	MOV CX,0x0101
	INT 0x15
	JC .NoAPM

	; Spengo
	MOV AX,0x5307
	MOV BX,1
	MOV CX,3
	INT 0x15
	
	.NoAPM:
	CALL Bootes.IO.Standard.Output, .StopReady
	HLT
	POP CX
	POP BX
	POP AX
	RET
	
	.StopReady		DB "E' ora possibile spegnere la macchina",13,10,0

;====================================================================
; Enable or disable A20 line.
; --------------------------------------------------------------
; PARAMS
;	State	0 - Disable, 1 - Enable
;====================================================================
Function Bootes.Machine.EnableA20, State
Begin
	PUSH AX
	
	; Wait keyboard ready
	.lA20_1:
		IN AL,0x64
		TEST AL,2
		JNZ .lA20_1
	;
	MOV	AL,0xD1
	OUT	0x64,AL
	; Wait keyboard ready
	.lA20_2:
		IN AL,0x64
		TEST AL,2
		JNZ .lA20_2
	; Setup A20
	MOV	AL,0xDD
	CMP WORD [State],0
	JZ @f
		; Enable
		OR AL,0x02
	@@:
	OUT	0x60,AL
	
	POP AX
Return

;====================================================================
; Passa alla modalità 64 bit dalla modalità 32 bit
;====================================================================
Bootes.Machine.ToLongMode:
	use32
	; --------------------------------------------------------------
	; Controlla se la modalità 64 bit è supportata
	; --------------------------------------------------------------
;	PUSHFD				; Push EFLAGS
;	POP EAX
;	MOV EBX,EAX
;	XOR EAX,0x00200000	; Togle bit 21
;	PUSH EAX
;	POPFD				; Pop EFLAGS
;	; Check for changes (no change = no CPUID)
;	PUSHFD
;	POP EAX
;	CMP EAX,EBX
;;	JZ .Error
;	; Check for Long Mode support
;	MOV EAX,0x80000000
;	CPUID
;	CMP EAX,0x80000000	; Look for function > 0x80000000
;;	JBE .Error
;	MOV EAX,0x80000001
;	CPUID				; EDX = Extended features flags
;	BT EDX,29			; Bit Test long mode bit
;;	JNC .Error
	PM4 = Bootes.Stage2.Info.PageMapLevel4
	PDPoffset = 0x1000
	PD0offset = 0x2000
	PD1offset = 0x3000
	PD2offset = 0x4000
	PD3offset = 0x5000
	TABLE_BITS = 0x7	; user, r/w, present
	PAGE_BITS = 0x87	; not pat, not global, not dirty, not accessed, user, r/w, present
	NOT_PRESENT_TABLE = 0x6
	NOT_PRESENT_PAGE = 0x86
	
	; --------------------------------------------------------------
	; Creo la page table
	; --------------------------------------------------------------
	MOV EDI,PM4
	MOV ECX,512
	MOV EDX,0
	@@:
		; PML4E (Page-Map Level4 Entry)
		MOV DWORD [EDI + EDX*8 + 0],NOT_PRESENT_TABLE
		MOV DWORD [EDI + EDX*8 + 4],0
		; PDPE (Page-Directory Pointer Entry)
		MOV DWORD [EDI + EDX*8 + PDPoffset + 0],NOT_PRESENT_TABLE
		MOV DWORD [EDI + EDX*8 + PDPoffset + 4],0
		; PDE (Page-Directory Entry)
		MOV DWORD [EDI + EDX*8 + PD0offset + 0],NOT_PRESENT_PAGE
		MOV DWORD [EDI + EDX*8 + PD0offset + 4],0
		MOV DWORD [EDI + EDX*8 + PD1offset + 0],NOT_PRESENT_PAGE
		MOV DWORD [EDI + EDX*8 + PD1offset + 4],0
		MOV DWORD [EDI + EDX*8 + PD2offset + 0],NOT_PRESENT_PAGE
		MOV DWORD [EDI + EDX*8 + PD2offset + 4],0
		MOV DWORD [EDI + EDX*8 + PD3offset + 0],NOT_PRESENT_PAGE
		MOV DWORD [EDI + EDX*8 + PD3offset + 4],0
		INC EDX
	LOOP @b

	MOV DWORD [EDI + 0x000 * 8 + 0],PM4 + PDPoffset + TABLE_BITS
	MOV DWORD [EDI + 0x000 * 8 + 4],0

	MOV DWORD [EDI + 0x000 * 8 + PDPoffset + 0],PM4 + PD0offset + TABLE_BITS
	MOV DWORD [EDI + 0x000 * 8 + PDPoffset + 4],0
	MOV DWORD [EDI + 0x001 * 8 + PDPoffset + 0],PM4 + PD1offset + TABLE_BITS
	MOV DWORD [EDI + 0x001 * 8 + PDPoffset + 4],0
	MOV DWORD [EDI + 0x002 * 8 + PDPoffset + 0],PM4 + PD2offset + TABLE_BITS
	MOV DWORD [EDI + 0x002 * 8 + PDPoffset + 4],0
	MOV DWORD [EDI + 0x003 * 8 + PDPoffset + 0],PM4 + PD3offset + TABLE_BITS
	MOV DWORD [EDI + 0x003 * 8 + PDPoffset + 4],0
	
	MOV ECX,1005
	MOV EDX,0
	MOV EBX,0
	@@:
		MOV EAX,EBX
		ADD EAX,PAGE_BITS
		MOV DWORD [EDI + EDX*8 + PD0offset + 0],EAX
		MOV DWORD [EDI + EDX*8 + PD0offset + 4],0
		ADD EAX,(1 * 0x200 * 0x20000)
		MOV DWORD [EDI + EDX*8 + PD1offset + 0],EAX
		MOV DWORD [EDI + EDX*8 + PD1offset + 4],0
		ADD EAX,(1 * 0x200 * 0x20000)
		MOV DWORD [EDI + EDX*8 + PD2offset + 0],EAX
		MOV DWORD [EDI + EDX*8 + PD2offset + 4],0
		ADD EAX,(1 * 0x200 * 0x20000)
		MOV DWORD [EDI + EDX*8 + PD3offset + 0],EAX
		MOV DWORD [EDI + EDX*8 + PD3offset + 4],0
		INC EDX
		ADD EBX,0x200000
	LOOP @b
	
	
;	; PML4E (Page-Map Level4 Entry)
;	ADD EAX,(512*8) + 00000001111b	; Una PML4 = 
;	MOV DWORD [EDI],EAX
;	MOV DWORD [EDI+4],0
;	; PDPE (Page-Directory Pointer Entry)
;	POP EAX
;	PUSH EAX
;	ADD EAX,(512*8)*2 + 00000001111b	; Una PDPE = 
;	MOV DWORD [EDI+4096],EAX
;	MOV DWORD [EDI+4096+4],0
;	; PDE (Page-Directory Entry) - 512 * 2MB = 1GB
;	MOV EAX,00010001111b	; PDE.PS = 1 (Page Size)
;	MOV ECX,512
;	@@:
;		MOV [EDI+8192],EAX
;		MOV [EDI+8192+4],DWORD 0
;		ADD EAX,1 SHL 21 ; 2MB
;		ADD EDI,8
;	LOOP @b
;	POP EDI

	; --------------------------------------------------------------
	; Enable 64 bit
	; --------------------------------------------------------------
;	CLI
;	MOV AL,0x70
;	MOV DX,0x80
;	OUT DX,AL 
	
	MOV EAX,CR0
	AND EAX,0x7FFFFFFF
	MOV CR0,EAX
	; Enable PAE
	MOV EAX,CR4
	OR EAX,0x20
	MOV CR4,EAX
	; Setup page table base address
	MOV EAX,PM4
	MOV CR3,EAX
	; Enable long mode
	MOV ECX,0x0C0000080
	RDMSR					; Read EFER
	OR EAX,0x100
	WRMSR
	; Enable paging to enable long mode
	MOV EAX,CR0
	OR EAX,0x80000000
	MOV CR0,EAX
	
	; Salto al codice a 64 bit
	DB 0x66
	DB 0xEA
	DD @f
	DW 0x28
	
	use64
	align 8
	@@:
	; Prepare return address
	MOV RAX,0
	MOV EAX,DWORD [ESP]
	; Stack initialization
	MOV RSP,[.StackAddr]
	MOV [RSP],RAX
	; GDT
;	LGDT [Bootes.Machine.ToProtectMode.GDT]
;	LIDT [.IDT]
	;
;	STI
	RET

	use32
	.Error:
	; TODO: migliorare
	MOV EDI,0xB8000
	MOV [EDI],DWORD 'E R '
	MOV [EDI+4],DWORD 'R 1 '
	JMP $
	
	; --------------------------------------------------------------
	; ToLongMode Data
	; --------------------------------------------------------------
	; Protect mode stack pointer
	.StackAddr	DQ Bootes.Stage2.Info.LModeStack - 8
	; Function return address 
	.ReturnAddr	DQ ?
	; For restore EAX value
	.EAXValue	DQ ?
	
	; --------------------------------------------------------------
	; Interrupt handlers
	; --------------------------------------------------------------
;	use64
;	.Interrupt_Dummy:
;		MOV AL,0x20
;		OUT 0x20,AL
;		IRET

;	.Interrupt_Keyboard:
;		PUSH RAX
;		IN AL,0x60
;		MOV [0xB8000],AL	; Echo a schermo
		; Return
;		MOV AL,0x20
;		OUT 0x20,AL
;		POP RAX
;		IRET

	; --------------------------------------------------------------
	; Interrupt Descriptor Table
	; --------------------------------------------------------------
;	.IDT:
;	.IDT.Size	DW .IDT.End-.IDT.Start-1	; GDT length - 1
;	.IDT.Addr	DQ .IDT.Start 				; GDT addr
;				DW 0						; Align 4
		
;	.IDT.Start:
		; Target offset(low), Target Code Selector, Flags, Target offset(mid), Target offset(hi1), Target offset(hi2), 0(Reserved), 0(Reserved)
;		times 64 DW .Interrupt_Dummy AND 0xFFFF, 0x08, 1000111000000000b, (.Interrupt_Dummy SHL 16) AND 0xFFFF, 0, 0, 0, 0
;	.IDT.End:
		
	

;====================================================================
; Passa alla modalità 32 bit dalla modalità 16 bit
;====================================================================
Bootes.Machine.ToProtectMode:
	use16
	CLI
	MOV [.EAXValue],EAX
	; --------------------------------------------------------------
	; Switch to protect mode
	; --------------------------------------------------------------
	; Load Global Descriptor Table
	LGDT [CS:.GDT]
	; Change CR0 Protect Mode bit
	MOV EAX,CR0
	OR AX,1
	MOV CR0,EAX
	; Far jump for change code segment
	JMP FWORD 0x08:@f
	
	use32
	; --------------------------------------------------------------
	; Protect mode operations
	; --------------------------------------------------------------
	@@:
	; Setup data segments
	MOV AX,0x10
	MOV DS,AX
	MOV ES,AX
	MOV SS,AX
	MOV FS,AX
	MOV GS,AX
	; Save RMode stack pointer
	AND ESP,0xFFFF
	MOV DWORD [Bootes.Machine.ToRealMode.StackAddr], ESP
	; Save return address
	MOVZX EAX,WORD [ESP]
	MOV [.ReturnAddr],EAX
	; Change stack pointer
	MOV EAX,[.StackAddr]
	MOV ESP,EAX
	MOV EBP,EAX
	; Resume return address
	MOV EAX,[.ReturnAddr]
	MOV [ESP],EAX
	; Return
	MOV EAX,[.EAXValue]
	RET
		
	use16
	align 8
	
	; --------------------------------------------------------------
	; ToProtectMode Data
	; --------------------------------------------------------------
	; Protect mode stack pointer
	.StackAddr	DD Bootes.Stage2.Info.PModeStack - 4
	; Function return address 
	.ReturnAddr	DD ?
	; For restore EAX value
	.EAXValue	DD ?
	
	; --------------------------------------------------------------
	; Global Descriptor Table
	; --------------------------------------------------------------
	align 8
	.GDT:
	.GDT.Size	DW .GDT.End-.GDT.Start-1	; GDT length - 1
	.GDT.Addr	DD .GDT.Start 				; GDT addr
				DD 0
	
	align 8
	.GDT.Start:
	DD 0,0
	
	; Stage2 Protect Mode Code segment
	; Index         0x08
	; Type          Code - read/execute
	; Ring          0
	; Base/Len      0/4GB
	.GDT.PMCode:
	DW 0xFFFF
	DW 0x0000
	DB 0x00
	DW 11001111b *256 +10011010b
	DB 0x0

	; Stage2 Protect Mode Data segment
	; Index         0x10
	; Type          Data - read/write
	; Ring          0
	; Base/Len      0/4GB
	.GDT.PMData:
	DW 0xFFFF
	DW 0x0000
	DB 0x0
	DW 11001111b *256 +10010010b
	DB 0x0
	
	; Stage2 Real Mode Code segment
	; Index         0x18
	; Type          Code - read/execute
	; Ring          0
	; Base/Len      0/4GB
	.GDT.RMCode:
	DW 0xFFFF
	DW 0x0000
	DB 0x00
	DW 10011110b
	DB 0x0

	; Stage2 Real Mode Data segment
	; Index         0x20
	; Type          Data - read/write
	; Ring          0
	; Base/Len      0/4GB
	.GDT.RMData:
	DW 0xFFFF
	DW 0x0000
	DB 0x0
	DW 10010010b
	DB 0x0

	; Stage2 Long Mode Code segment
	; Index         0x28
	; Type          Code - read/execute
	; Ring          0
	; Base/Len      0/4GB
	.GDT.LMCode:
	DW 0xFFFF
	DW 0x0000
	DB 0x00
	DB 0x9A, 0xAF
	DB 0x0

	; Stage2 Long Mode Data segment
	; Index         0x30
	; Type          Data - read/write
	; Ring          0
	; Base/Len      0/4GB
	.GDT.LMData:
	DW 0xFFFF
	DW 0x0000
	DB 0x0
	DB 0x92, 0xAF
	DB 0x0
	.GDT.End:


;====================================================================
; Passa alla modalità 16 bit dalla modalità 32 bit
;====================================================================
Bootes.Machine.ToRealMode:
	use32
	MOV [.EAXValue],EAX
	; --------------------------------------------------------------
	; Save protect mode stack pointer
	; --------------------------------------------------------------
	MOV EAX,ESP
	ADD EAX,4
	MOV [Bootes.Machine.ToProtectMode.StackAddr],EAX
	
	; --------------------------------------------------------------
	; Move return address to real mode stack
	; --------------------------------------------------------------
	MOV EAX,[ESP]
	MOV ESP,[.StackAddr]
	MOV WORD [ESP],AX

	; --------------------------------------------------------------
	; Set up segment limit
	; --------------------------------------------------------------
	MOV AX,0x20
	MOV DS,AX
	MOV ES,AX
	MOV SS,AX
	MOV FS,AX
	MOV GS,AX
	JMP FWORD 0x18:@f
	@@:
	use16
		
	; --------------------------------------------------------------
	; Switch to real mode
	; --------------------------------------------------------------
	MOV EAX,CR0
	AND EAX,0xFFFFFFFE
	MOV CR0,EAX
	; Change CS segment
	DB 0x66
	JMP FWORD 0:@f
	@@:

	
	; --------------------------------------------------------------
	; Real mode operations
	; --------------------------------------------------------------
	XOR AX,AX
	; Setup segments
	MOV DS,AX
	MOV ES,AX
	MOV SS,AX
	MOV FS,AX
	MOV GS,AX
	; Return
	MOV EAX,[.EAXValue]
	STI
	RET

	; For restore EAX value
	.EAXValue	DD ?
	; For restore real mode stack pointer
	.StackAddr	DD ?