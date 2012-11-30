;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Devices.Video
;
; Gestisce il video.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2

; --------------------------------------------------------------
; Funzioni esportate
; --------------------------------------------------------------
SysNamespace Bootes.Devices.Video_Namespace, "Devices.Video"
	Export Bootes.Devices.Video.DrawString,2, "DrawString"
	Export Bootes.Devices.Video.SetMode,1,"SetMode", Type.Number, Type.Number
	Export Bootes.Devices.Video.SetTextCursor,1,"SetTextCursor", Type.Null, Type.Number


; --------------------------------------------------------------
; Strutture di informazioni
; --------------------------------------------------------------
STRUC Vesa.Info
{
	.Signature			DB 'VBE2'
	.Version:
	  .Version.Major	DB ?
	  .Version.Minor	DB ?
	.OEMName			DD ?		; Puntatore al nome OEM 
	.Capabilities		DD ?		; Flag delle capabilità
	.Modes				DD ?		; Puntatore alla lista di modi 
									; terminata da 0xFFFF
	.Memory 			DW ?		; Numero di blocchi da 64K
	.Revision:
	  .Revision.Major	DB ?
	  .Revision.Minor	DB ?
	.VendorName			DD ?		; Puntatore alla stringa del nome 
									; del produttore
	.ProductName		DD ?		; Puntatore alla stringa del nome 
									; del prodotto
	.RevisionName		DD ?		; Puntatore alla stringa di revisione
	.Reserved			RB 222
	.Data				RB 256
	
	.Size				= $-.Signature
}

STRUC Vesa.Mode.Info
{
	.ModeAttr	DW ?	;    mode attributes
	.AWinAttr	DB ?	;    window attributes, window A
	.BWinAttr	DB ?	;    window attributes, window B
	.WinGran	DW ?	;    window granularity in KB
	.WinSize	DW ?	;    window size in KB
	.AWinSeg	DW ?	;    start segment of window A 
	.BWinSeg	DW ?	;    start segment of window B 
	.PostFunc	DD ?	;    -> FAR window positioning function
	.BPS		DW ?	;    bytes per scan line

	.Width		DW ?	;    width in pixels (graphics) or characters
	.Height		DW ?	;    height in pixels (graphics) or characters
	.CellWidth	DB ?	;    width of character cell in pixels
	.CellHeight	DB ?	;    height of character cell in pixels
	.Planes		DB ?	;    number of memory planes
	.BPP		DB ?	;    number of bits per pixel
	.Banks		DB ?	;    number of banks
	.MemModel	DB ?	;    memory model type
	.BankSize	DB ?	;    size of bank in KB
	.ImgPages	DB ?	;    number of image pages (less one) that will 
						;    fit in video RAM
				DB ?	;    reserved
	;---VBE v1.2+ ---
	.RedMask	DB ?	;    red mask size
	.RedField	DB ?	;    red field position
	.GreenMask	DB ?	;    green mask size
	.GreenField	DB ?	;    green field size
	.BlueMask	DB ?	;    blue mask size
	.BlueField	DB ?	;    blue field size
	.ResMask	DB ?	;    reserved mask size
	.ResField	DB ?	;    reserved mask position
	.ColorMode	DB ?	;    direct color mode info
	;---VBE v2.0+ ---
	.LFB		DD ?	;    physical address of linear video buffer
	.OffScreen	DD ?	;    pointer to start of offscreen memory
	.OffScrSize	DW ?	;    KB of offscreen memory
	;---VBE v3.0 ---
	.BPSLinear	DW ?	;    bytes per scan line in linear modes
	.BankImages	DB ?	;    number of images (less one) for banked 
						;    video modes
	.LinImages	DB ?	;    number of images (less one) for linear 
						;    video modes
	
	.Size		= $-.ModeAttr
}

; --------------------------------------------------------------
; Virtuals
; --------------------------------------------------------------
VIRTUAL AT BX	; Comincia a [Bootes.Devices.Video.Data.VesaInfoAddr]
	VesaInfo		Vesa.Info
	VesaModeInfo	Vesa.Mode.Info
END VIRTUAL


;====================================================================
; Inizilizza la gestione della grafica
;====================================================================
align 4
Bootes.Devices.Video.Init:
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Devices.Video_Namespace
	CALL Bootes.Devices.Video.SetMode, Bootes.Stage2.Info.DefaultVideoMode
	RET



;====================================================================
; Seleziona la modalità SVGA specificata.
; --------------------------------------------------------------
; PARAMS
;	Mode	Modalità desiderata, 0 per selezionare la modalità 
;			massima.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Devices.Video.SetMode, Mode
Begin
	LockScheduler
	PUSH BX
	PUSH CX
	PUSH DI
	
	; --------------------------------------------------------------
	; Controllo il tipo di modalita (VGA/SVGA)
	; --------------------------------------------------------------
	MOV BX,[Mode]
	OR BX,BX
	JZ .iSetSVGA
	CMP BX,0xD0
	JBE .eSetSVGA
	.iSetSVGA:
		; --------------------------------------------------------------
		; Imposto SVGA
		; --------------------------------------------------------------
		; Alloco memoria se necessario
		CMP WORD [Bootes.Devices.Video.Data.VesaInfoAddr],0
		JNZ @f
			CALL Bootes.Memory.Alloc, (VesaInfo.Size+VesaModeInfo.Size)
			OR AX,AX
			JZ .Error
			MOV [Bootes.Devices.Video.Data.VesaInfoAddr],AX
			; Con l'INT 0x10 AH=0x4F recupero le informazioni sulla scheda video.
			MOV	DI,AX
			MOV	AX,0x4F00
			INT	0x10
		@@:
		MOV DI,[Bootes.Devices.Video.Data.VesaInfoAddr]
		; Imposto la modalità
		; TODO: Recuperare modalità da parametri di risoluzione
		OR BX,BX
		JZ .eModeSpecified
		.iModeSpecified:
			MOV AX,0x4F02	    ; Funzione per impostare la modalità
			INT 0x10
			OR AH,AH
			JNZ .Error
			JMP .xModeSpecified
		.eModeSpecified:
			; Cerca e imposta modalità
			MOV BX,[Bootes.Devices.Video.Data.VesaInfoAddr]
			MOV	AX,[VesaInfo.Version]	; Ricavo versione VESA
			MOV	BX,0x411B		      	; Massima modalità grafica (con linear framebuffer abilitato)
			CMP	AH,2
			JGE	@f 						; Versione della vesa > 2.0
				AND	 BX,0xFFF		     	; Disabilita linear framebuffer
			@@:

			; Provo le modalità
			.lTestMode:
				MOV	 CX,BX		    ; Modalità in CX
				AND	 CX,0xFFF		; Elimino dalla modalità i flag non richiesti

				CMP	 CX,0x10F		; Ultima modalità consentita
				JA   @f
				CMP  CX,0x10E
				JBE	 .Error
				MOV  BX,0x411A
				JMP  .lTestMode
				@@:

				MOV	 AX,0x4F02	    ; Funzione per impostare la modalità
				INT	 0x10

				OR	 AH,AH	     	; Controllo che la funzione abbia avuto successo
				JZ	 .xModeSpecified

				SUB	 BX,3		    ; Provo con la modalità precedente
				JMP	 .lTestMode
		.xModeSpecified:
		; --------------------------------------------------------------
		; Ricavo informazioni sulla modalità impostata
		; --------------------------------------------------------------
		MOV	DI,[Bootes.Devices.Video.Data.VesaInfoAddr]
		ADD DI,VesaInfo.Size
		MOV CX,BX
		MOV	AX,0x4F01
		INT	0x10
		OR	AH,AH
		JNE	.Error
		JMP .xSetSVGA
	.eSetSVGA:
		; --------------------------------------------------------------
		; Imposto EGA/VGA
		; --------------------------------------------------------------
		XOR AX,AX
		OR AL,BL
		INT 0x10
	.xSetSVGA:
	
	; --------------------------------------------------------------
	; Salvo informazioni modalità
	; --------------------------------------------------------------
	MOV AX,BX
	MOV	[Bootes.Devices.Video.Data.CurrentVideoMode],BX
	if Bootes.Stage2.Info.GraphicSupport = 1
	TEST BX,0x4000
	JNZ .iFBVideo
	CMP BX,0x13
	JNE .eFBVideo
	.iFBVideo:
		; Back buffer
		MOV BX,[Bootes.Devices.Video.Data.VesaInfoAddr]
		MOV CX,WORD [VesaModeInfo32.Width]
   		SHL CX,2
   		MOV [Bootes.Devices.Video.Data.BackBufferBPS],CX
		; Delegate specifico
		CMP BYTE [VesaModeInfo.BPP],24
		JA .iVideo32
		JE .iVideo24
		.iVideo16:
			; TODO: scrivere funzioni anche per modalità 0x13 (?)
		.iVideo24:
			MOV WORD [Bootes.Devices.Video.Data.FlipDelegate],Bootes.Devices.Video.SVGA.Flip24
			MOV WORD [Bootes.Devices.Video.Data.FlipRectDelegate],Bootes.Devices.Video.SVGA.FlipRect24
			JMP .xVideoxx
		.iVideo32:
			MOV WORD [Bootes.Devices.Video.Data.FlipDelegate],Bootes.Devices.Video.SVGA.Flip32
			MOV WORD [Bootes.Devices.Video.Data.FlipRectDelegate],Bootes.Devices.Video.SVGA.FlipRect32
		.xVideoxx:
		; Delegate comuni
		MOV WORD [Bootes.Devices.Video.Data.DrawCharDelegate],Bootes.Devices.Video.SVGA.DrawChar
		MOV WORD [Bootes.Devices.Video.Data.DeleteCharDelegate],Bootes.Devices.Video.SVGA.DeleteChar
		MOV WORD [Bootes.Devices.Video.Data.GotoXYDelegate],Bootes.Devices.Video.SVGA.GotoXY
		MOV WORD [Bootes.Devices.Video.Data.GetXDelegate],Bootes.Devices.Video.SVGA.GetX
		MOV WORD [Bootes.Devices.Video.Data.GetYDelegate],Bootes.Devices.Video.SVGA.GetY
		MOV WORD [Bootes.Devices.Video.Data.ClearDelegate],Bootes.Devices.Video.SVGA.Clear
		; Azzero posizione
		MOV WORD [Bootes.Devices.Video.Data.X],0
		MOV WORD [Bootes.Devices.Video.Data.Y],0
		; Pulisco il back buffer
	;	CALL Bootes.Devices.Video.SVGA.Clear		TODO:BUG!!??
		JMP .Exit
	.eFBVideo:
		MOV WORD [Bootes.Devices.Video.Data.DrawCharDelegate],Bootes.Devices.Video.VGA.DrawChar
		MOV WORD [Bootes.Devices.Video.Data.DeleteCharDelegate],Bootes.Devices.Video.VGA.DeleteChar
		MOV WORD [Bootes.Devices.Video.Data.GotoXYDelegate],Bootes.Devices.Video.VGA.GotoXY
		MOV WORD [Bootes.Devices.Video.Data.GetXDelegate],Bootes.Devices.Video.VGA.GetX
		MOV WORD [Bootes.Devices.Video.Data.GetYDelegate],Bootes.Devices.Video.VGA.GetY
		MOV WORD [Bootes.Devices.Video.Data.ClearDelegate],Bootes.Devices.Video.VGA.Clear
	.xFBVideo:
	end if
	
	JMP .Exit
	.Error:
	; TODO: Impostare modalità di default
	XOR AX,AX
	.Exit:
	POP DI
	POP CX
	POP BX
	UnlockScheduler
Return



;====================================================================
; Scrive una carattere a video nella console corrente e sposta il 
; cursore.
; --------------------------------------------------------------
; PARAMS
;	Char		Carattere da scrivere.
;	Color		Colore del carattere.
;====================================================================
Function Bootes.Devices.Video.DrawChar, Char,Color
Begin
	CALL WORD [Bootes.Devices.Video.Data.DrawCharDelegate], [Char],[Color]
Return



;====================================================================
; Cancella il carattere nella posizione corrente del cursore.
;====================================================================
Bootes.Devices.Video.DeleteChar:
	CALL WORD [Bootes.Devices.Video.Data.DeleteCharDelegate]
RET



;====================================================================
; Sposta il cursore di scrittura nella posizione indicata.
; --------------------------------------------------------------
; PARAMS
;	X		Posizione X nella quale spostare il cursore.
;	Y		Posizione Y nella quale spostare il cursore.
; RETURN
;	Posizione del cursore, AH riga e AL colonna.
;====================================================================
Function Bootes.Devices.Video.GotoXY, X,Y
Begin
	CALL WORD [Bootes.Devices.Video.Data.GotoXYDelegate], [X],[Y]
Return



;====================================================================
; Ricava la posizione X del cursore.
; --------------------------------------------------------------
; RETURN
;	Posizione X del cursore.
;====================================================================
Function Bootes.Devices.Video.GetX
Begin
	CALL WORD [Bootes.Devices.Video.Data.GetXDelegate]
Return



;====================================================================
; Ricava la posizione Y del cursore.
; --------------------------------------------------------------
; RETURN
;	Posizione Y del cursore.
;====================================================================
Function Bootes.Devices.Video.GetY
Begin
	CALL WORD [Bootes.Devices.Video.Data.GetYDelegate]
Return



;====================================================================
; Pulisce lo schermo.
;====================================================================
Function Bootes.Devices.Video.Clear
Begin
	CALL WORD [Bootes.Devices.Video.Data.ClearDelegate]
Return



;====================================================================
; Scrive una stinga a video nella console corrente.
; --------------------------------------------------------------
; PARAMS
;	String		Puntatore alla stringa terminata da 0 da scrivere.
;	Text		Colore del testo.
;====================================================================
Function Bootes.Devices.Video.DrawString, String,Color
Begin
	PUSH AX
	PUSH SI
	
	MOV SI,[String]
	OR SI,SI
	JZ .Exit
	.lText:
		LODSB
		OR AL,AL
		JZ .Exit
		; Scrivo
		.Write:
		CALL Bootes.Devices.Video.DrawChar, AX,[Color]
		JMP .lText
	
	.Exit:
	POP SI
	POP AX
Return



;====================================================================
; Imposta il cursore in motalità testuale.
; --------------------------------------------------------------
; PARAMS
;	Height	Altezza del cursore.
;====================================================================
Function Bootes.Devices.Video.SetTextCursor, Height
Begin
	PUSH AX
	PUSH CX

	MOV AX,[Height]
	NOT AL
	AND AL,0x0F
	MOV CH,AL
	MOV CL,0x0F
	MOV AX,0x0100
	INT 0x10
	
	POP CX
	POP AX
Return	



;====================================================================
; Data
;====================================================================
Bootes.Devices.Video.Data:
	.VesaInfoAddr				DW 0
	.CurrentVideoMode			DW ?
	.X							DW 0
	.Y							DW 0
	.Color						DW 0,0
	.ClearColor					DW 0,0
	.BackBuffer					DD Bootes.Stage2.Info.BackFrameBuffer
	.BackBufferBPS				DW ?,0
	;
	.FlipDelegate				DW ?,0
	.FlipRectDelegate			DW ?,0
	;
	.DrawCharDelegate			DW Bootes.Devices.Video.VGA.DrawChar
	.DeleteCharDelegate			DW Bootes.Devices.Video.VGA.DeleteChar
	.GotoXYDelegate				DW Bootes.Devices.Video.VGA.GotoXY
	.GetXDelegate				DW Bootes.Devices.Video.VGA.GetX
	.GetYDelegate				DW Bootes.Devices.Video.VGA.GetY
	.ClearDelegate				DW Bootes.Devices.Video.VGA.Clear


;====================================================================
; Include
;====================================================================
include "VGA.inc"
include "SVGA.inc"