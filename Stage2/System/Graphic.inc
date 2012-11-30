;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Graphic
;
; Disponibile solo con GraphicSupport abilitato.
; Contiene funzioni grafiche che sfruttano le funzioni definite in SVGA.inc.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if Bootes.Stage2.Info.GraphicSupport = 1


;====================================================================
; Imposta il colore a 32 bit.
; --------------------------------------------------------------
; PARAMS
;	Color	Colore a 32 bit da impostare nel formato XX RR GG BB.
;====================================================================
Function Bootes.Graphic.SetColor, ColorHi,ColorLo
Begin
	PUSH AX
	MOV AX,[ColorHi]
	MOV [Bootes.Devices.Video.Data.Color+2],AX
	MOV AX,[ColorLo]
	MOV [Bootes.Devices.Video.Data.Color],AX
	POP AX
Return


;====================================================================
; Imposta il colore a 32 bit che verrà utilizzato come sfondo dove
; richiesto.
; --------------------------------------------------------------
; PARAMS
;	Color	Colore a 32 bit da impostare nel formato XX RR GG BB.
;====================================================================
Function Bootes.Graphic.SetClearColor, ColorHi,ColorLo
Begin
	PUSH AX
	MOV AX,[ColorHi]
	MOV [Bootes.Devices.Video.Data.ClearColor+2],AX
	MOV AX,[ColorLo]
	MOV [Bootes.Devices.Video.Data.ClearColor],AX
	POP AX
Return



;====================================================================
; Restituisce la larghezza della risoluzione in pixel.
; --------------------------------------------------------------
; RETURN
;	Larghezza in pixel della risoluzione.
;====================================================================
Function Bootes.Graphic.GetHeight
Begin
	PUSH BX
	MOV BX,[Bootes.Devices.Video.Data.VesaInfoAddr]
	MOV AX,[VesaModeInfo.Height]
	POP BX
Return



;====================================================================
; Restituisce la larghezza della risoluzione in pixel.
; --------------------------------------------------------------
; RETURN
;	Larghezza in pixel della risoluzione.
;====================================================================
Function Bootes.Graphic.GetWidth
Begin
	PUSH BX
	MOV BX,[Bootes.Devices.Video.Data.VesaInfoAddr]
	MOV AX,[VesaModeInfo.Width]
	POP BX
Return



;====================================================================
; Pulisce lo schermo.
;====================================================================
Function Bootes.Graphic.Clear
Begin
	; TODO: pulire con il colore di clear
	CALL Bootes.Devices.Video.SVGA.Clear
Return



;====================================================================
; Disegna una stringa partendo dalla posizione indicata e cancellando
; lo sfondo se richiesto
; --------------------------------------------------------------
; PARAMS
;	X		Coordinata X della stringa da presentare.
;	Y		Coordinata Y della stringa da presentare.
;	String	Puntatore ad una stringa terminata da 0.
;	ClearBG	Valore booleano che indica se pulire o meno lo sfondo
;			della stringa con il colore ClearColor.
;====================================================================
Function Bootes.Graphic.DrawString, X,Y,String,ClearBG
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI

	MOVZX EAX,WORD [X]
	MOVZX EDI,WORD [Y]
	MOVZX ESI,WORD [String]
	MOVZX EBX,WORD [ClearBG]

	ProtectMode

	; Controlla i parametri
	CMP	[VesaModeInfo32.Width],AX
	JB	.Exit
	CMP	[VesaModeInfo32.Height],DX
	JB	.Exit

	; Salvo le coordinate
	PUSH EAX	; X
	PUSH EDI	; Y
	XOR EDX,EDX

	; Calcolo la posizione in memoria
	IMUL EDI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
	LEA	EDI,[EDI+EAX*4]
	ADD	EDI,[Bootes.Devices.Video.Data.BackBuffer]

	.lString:
		; Recupero il carattere
		PUSH ESI
		MOV	ESI,[ESI]
		AND ESI,0xFF
		CMP	ESI,0
		JE	.xString
		SHL	ESI,4
		ADD	ESI,Bootes.Devices.Video.SVGA.Data.RawFont
		; Loop di disegno, ho caratteri alti 16 pixel e larghi 8.
		PUSH EDI
		MOV	ECX,Bootes.Devices.Video.SVGA.Data.CharHeight
		.lYLoop:
			MOV AL,BYTE [ESI]
			PUSH EDI
			PUSH ECX
			MOV ECX,Bootes.Devices.Video.SVGA.Data.CharWidth
			.lXLoop:
				RCL AL,1
				JC .eClear
				.iClear:
					OR EBX,EBX
					JZ .xClear
					PUSH EAX
					MOV EAX,DWORD [Bootes.Devices.Video.Data.ClearColor]
					MOV DWORD [EDI],EAX
					POP EAX
					JMP .xClear
				.eClear:
					PUSH EAX
					MOV EAX,DWORD [Bootes.Devices.Video.Data.Color]
					MOV DWORD [EDI],EAX
					POP EAX
				.xClear:
				; Prossimo pixel
				ADD EDI,4
				LOOP .lXLoop
			POP ECX
			POP EDI
			ADD EDI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
			INC ESI
			LOOP .lYLoop
		; Prossima posizione
		POP EDI
		ADD EDI,Bootes.Devices.Video.SVGA.Data.CharWidthSpace*4
		; Prossimo carattere
		POP ESI
		INC EDX
		INC ESI
		JMP .lString
	.xString:
	POP ESI
	
	; Flip
	MOVZX EBX,WORD [Bootes.Devices.Video.Data.VesaInfoAddr]
	POP ESI	; Y
	POP EAX	; X
	MOV ECX,Bootes.Devices.Video.SVGA.Data.CharHeight
	IMUL EDX,Bootes.Devices.Video.SVGA.Data.CharWidthSpace
	CALL DWORD [Bootes.Devices.Video.Data.FlipRectDelegate]

	.Exit:
	RealMode
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
	POP AX
Return



;====================================================================
; Disegna il bordo di un rettangolo delle dimensioni specificate.
; --------------------------------------------------------------
; PARAMS
;	X		Coordinata X del rettangolo da presentare.
;	Y		Coordinata Y del rettangolo da presentare.
;	Width	Larghezza del rettangolo da presentare.
;	Height	Altezza del rettangolo da presentare.
;====================================================================
Function Bootes.Graphic.DrawRect, X,Y,Width,Height
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI

	; Prepara registri
	MOVZX EBX,WORD [Bootes.Devices.Video.Data.VesaInfoAddr]
	MOVZX EAX,WORD [X]
	MOVZX ESI,WORD [Y]
	MOVZX ECX,WORD [Width]
	MOVZX EDX,WORD [Height]

	ProtectMode
	; Prepara il colore
	MOV EDI,DWORD [Bootes.Devices.Video.Data.Color]

	; Lato superiore
	PUSH EAX
	PUSH ECX
	@@:
		CALL Bootes.Devices.Video.SVGA.PutPixel32
		INC EAX
		LOOP @b
	POP ECX
	POP EAX
	
	;Lato inferiore
	PUSH ECX
	PUSH EAX
	PUSH ESI
	ADD ESI,EDX
	DEC ESI
	@@:
		CALL Bootes.Devices.Video.SVGA.PutPixel32
		INC	EAX
		LOOP @b
	POP ESI
	POP EAX
	POP ECX

	; Lato destro
	XCHG ECX,EDX
	PUSH ESI
	PUSH ECX
	@@:
		CALL Bootes.Devices.Video.SVGA.PutPixel32
		INC ESI
		LOOP @b
	POP ECX
	POP ESI

	;Lato sinistro
	PUSH EAX
	PUSH ECX
	PUSH ESI
	ADD EAX,EDX
	DEC EAX
	@@:
		CALL Bootes.Devices.Video.SVGA.PutPixel32
		INC	ESI
		LOOP @b
	POP ESI
	POP ECX
	POP EAX

	; Flip
	CALL DWORD [Bootes.Devices.Video.Data.FlipRectDelegate]
	
	.Exit:
	RealMode
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
	POP AX
Return



;====================================================================
; Disegna un rettangolo delle dimensioni specificate.
; --------------------------------------------------------------
; PARAMS
;	X		Coordinata X del rettangolo da presentare.
;	Y		Coordinata Y del rettangolo da presentare.
;	Width	Larghezza del rettangolo da presentare.
;	Height	Altezza del rettangolo da presentare.
;====================================================================
Function Bootes.Graphic.FillRect, X,Y,Width,Height
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI

	; Prepara registri
	MOVZX EBX,WORD [Bootes.Devices.Video.Data.VesaInfoAddr]
	MOVZX EAX,WORD [X]
	MOVZX ESI,WORD [Y]
	MOVZX ECX,WORD [Height]
	MOVZX EDX,WORD [Width]

	ProtectMode
	; Prepara il colore
	MOV EDI,DWORD [Bootes.Devices.Video.Data.Color]

	; Controlla i parametri
	CMP [VesaModeInfo32.Width],AX
	JB .Exit
	CMP [VesaModeInfo32.Height],SI
	JB .Exit
	
	; Calcolo la posizione in memoria
	PUSH ESI
	IMUL ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
	LEA ESI,[ESI+EAX*4]
	ADD ESI,DWORD [Bootes.Devices.Video.Data.BackBuffer]
	; Scrivo i pixel in memoria
	PUSH ECX
	DEC ECX
	.lY:
		PUSH ESI
		PUSH ECX
		MOV ECX,EDX
		DEC ECX
		.lX:
			MOV [ESI],EDI
			ADD ESI,4
			LOOP .lX
		POP ECX
		POP ESI
		ADD ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
		LOOP .lY
	POP ECX
	POP ESI
	
	;Flip
	CALL DWORD [Bootes.Devices.Video.Data.FlipRectDelegate]

	.Exit:
	RealMode
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
	POP AX
Return

end if