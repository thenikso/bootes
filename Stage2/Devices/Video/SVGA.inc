;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Devices.Video.SVGA
;
; Funzioni grafiche VGA a 24 e 32 bit.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
if Bootes.Stage2.Info.GraphicSupport = 1


VIRTUAL AT EBX	; Comincia a [Bootes.Devices.Video.Data.VesaInfoAddr]
	VesaInfo32		Vesa.Info
	VesaModeInfo32	Vesa.Mode.Info
END VIRTUAL


;====================================================================
; Esegue la presentazione del back buffer sul front buffer.
;====================================================================
Function Bootes.Devices.Video.SVGA.Flip
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI
	MOVZX EBX,WORD [Bootes.Devices.Video.Data.VesaInfoAddr]
	ProtectMode
	
	CALL DWORD [Bootes.Devices.Video.Data.FlipDelegate]
	
	RealMode
	POP DI
	POP SI
	POP CX
	POP BX
	POP AX
Return

use32
align 4
Bootes.Devices.Video.SVGA.Flip32:
	MOVZX EAX,[VesaModeInfo32.Height]
	MOV ESI,[Bootes.Devices.Video.Data.BackBuffer]
	MOV EDI,[VesaModeInfo32.LFB]
	MOVZX ECX,[VesaModeInfo32.Width]
	IMUL ECX,EAX
	REP MOVSD
RET

align 4
Bootes.Devices.Video.SVGA.Flip24:
	MOV ESI,[Bootes.Devices.Video.Data.BackBuffer]
	MOV EDI,[VesaModeInfo32.LFB]
	MOVZX ECX,[VesaModeInfo32.Height]
	.lYLoop:
		PUSH ECX
		MOVZX ECX,WORD [VesaModeInfo32.Width]
		.lXLoop:
			; Ricavo il colore a 24 bit (tolgo il primo byte)
			MOV EAX,[ESI]
			; Scrivo il colore sul front buffer e mi sposto di 3 byte invece che di 4
			MOV [EDI],AX
			SHR EAX,16
			MOV [EDI+2],AL
			ADD EDI,3
			; Mi sposto anche nel back buffer
			ADD ESI,4
			LOOP .lXLoop
		POP ECX
		; Prossima riga
		LOOP .lYLoop
RET
use16


;====================================================================
; Esegue la presentazione di una parte specifica del back buffer sul 
; front buffer.
; --------------------------------------------------------------
; PARAMS
;	X		Coordinata X del rettangolo da presentare.
;	Y		Coordinata Y del rettangolo da presentare.
;	Width	Larghezza del rettangolo da presentare.
;	Height	Altezza del rettangolo da presentare.
;====================================================================
Function Bootes.Devices.Video.SVGA.FlipRect, X,Y,Width,Height
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	MOVZX EBX,WORD [Bootes.Devices.Video.Data.VesaInfoAddr]
	MOVZX EAX,WORD [X]
	MOVZX ESI,WORD [Y]
	MOVZX ECX,WORD [Height]
	MOVZX EDX,WORD [Width]
	ProtectMode
	
	CALL DWORD [Bootes.Devices.Video.Data.FlipRectDelegate]
	
	RealMode
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
	POP AX
Return

use32
align 4
Bootes.Devices.Video.SVGA.FlipRect32:
	IMUL ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
	LEA ESI,[ESI+EAX*4]
	MOV EDI,ESI
	ADD ESI,[Bootes.Devices.Video.Data.BackBuffer]
	ADD EDI,[VesaModeInfo32.LFB]
	.lY:
		PUSH ECX
		MOV ECX,EDX
		PUSH ESI
		PUSH EDI
		REP MOVSD
		POP EDI
		POP ESI
		POP ECX
		ADD ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
		ADD EDI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
		LOOP .lY
RET

align 4
Bootes.Devices.Video.SVGA.FlipRect24:
	MOV EDI,ESI
	IMUL ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
	LEA ESI,[ESI+EAX*4]
	ADD ESI,[Bootes.Devices.Video.Data.BackBuffer]
	PUSH EAX
	MOVZX EAX,WORD [VesaModeInfo32.BPS]
	IMUL EDI,EAX
	POP EAX
	IMUL EAX,3
	ADD EDI,EAX	
	ADD EDI,[VesaModeInfo32.LFB]
	XOR EAX,EAX
	.lY:
		PUSH ECX
		PUSH ESI
		PUSH EDI
		MOV ECX,EDX
		.lX:
			MOV EAX,[ESI]
			MOV [EDI],AX
			SHR EAX,16
			MOV [EDI+2],AL
			ADD EDI,3
			ADD ESI,4
			LOOP .lX
		POP EDI
		POP ESI
		MOVZX EAX,WORD [VesaModeInfo32.BPS]
		ADD ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
		ADD EDI,EAX
		POP ECX
		; Prossima riga
		LOOP .lY
RET
use16



;====================================================================
; Imposta il pixel specificato.
; --------------------------------------------------------------
; PARAMS
;	X		Coordinata X del rettangolo da presentare.
;	Y		Coordinata Y del rettangolo da presentare.
;	Color	Colore da attribuire al pixel oppure -1 per utilzzare il
;			colore a 32 bit.
;====================================================================
Function Bootes.Devices.Video.SVGA.PutPixel, X,Y,Color
Begin
	PUSH AX
	PUSH SI
	PUSH DI
	
	MOVZX EAX,WORD [X]
	MOVZX ESI,WORD [Y]
	MOVZX EDI,WORD [Color]
	
	ProtectMode
	
	; Traduzione colore
	CMP DI,-1
	JE .eTranslateColor
		OR DI,DI
		JZ .xTranslateColor
		; Light grey
		CMP EDI,7
		JNE @f
			MOV EDI,0xFFBBBBBB
			JMP .xTranslateColor
		@@:
		; Dark grey
		CMP EDI,8
		JNE @f
			MOV EDI,0xFF555555
			JMP .xTranslateColor
		@@:
		; Red
		CMP EDI,4
		JNE @f
			MOV EDI,0xFFFF1111
			JMP .xTranslateColor
		@@:
		; TODO: Altri colori
		MOV EDI,0xFFFFFFFF
		JMP .xTranslateColor
	.eTranslateColor:
		MOV EDI,DWORD [Bootes.Devices.Video.Data.Color]
	.xTranslateColor:
	; Controlla i parametri
	CMP [VesaModeInfo32.Width],AX
	JB .Exit
	CMP [VesaModeInfo32.Height],SI
	JB .Exit
	; Calcolo la posizione in memoria
	IMUL ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
	LEA ESI,[ESI+EAX*4]
	ADD ESI,DWORD [Bootes.Devices.Video.Data.BackBuffer]
	; Scrivo il pixel in memoria
	MOV [ESI],EDI

	.Exit:
	RealMode
	POP DI
	POP SI
	POP AX
Return


use32
; EAX=X, ESI=Y, EDI=Color
Bootes.Devices.Video.SVGA.PutPixel32:
	; Controlla i parametri
	CMP [VesaModeInfo32.Width],AX
	JB .Exit
	CMP [VesaModeInfo32.Height],SI
	JB .Exit
	PUSH ESI
	; Calcolo la posizione in memoria
	IMUL ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
	LEA ESI,[ESI+EAX*4]
	ADD ESI,DWORD [Bootes.Devices.Video.Data.BackBuffer]
	; Scrivo il pixel in memoria
	MOV [ESI],EDI
	POP ESI
	.Exit:
RET
use16



;====================================================================
; Scrive una carattere a video nella console corrente e sposta il 
; cursore.
; --------------------------------------------------------------
; PARAMS
;	Char		Carattere da scrivere.
;	Color		Colore del carattere.
;====================================================================
Function Bootes.Devices.Video.SVGA.DrawChar, Char,Color
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	MOVZX EBX,WORD [Bootes.Devices.Video.Data.VesaInfoAddr]
	MOVZX EAX,WORD [Bootes.Devices.Video.Data.X]
	MOVZX EDX,WORD [Bootes.Devices.Video.Data.Y]
	MOVZX ESI,WORD [Char]
	MOVZX EDI,WORD [Color]
	
	ProtectMode
	
   ; Controlla i parametri
   CMP	[VesaModeInfo32.Width],AX
   JB	.Exit
   CMP	[VesaModeInfo32.Height],DX
   JB	.Exit
	
	; Caratteri speciali
	CMP ESI,13
	JNE @f
		; Return
		.Return:
		MOVZX ECX,WORD [VesaModeInfo.Height]
		ADD EDX,Bootes.Devices.Video.SVGA.Data.CharHeight
		CMP EDX,ECX
		JB .xMoveBB
			; TODO: Allugare il buffer (?)
			MOV EDI,[Bootes.Devices.Video.Data.BackBuffer]
			MOV ESI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
			MOVZX ECX,WORD [VesaModeInfo32.Height]
			IMUL ECX,ESI
			SHL ESI,4
			ADD ESI,EDI
			SHR ECX,2
			REP MOVSD
			MOV DX,WORD [VesaModeInfo.Height]
			SUB DX,Bootes.Devices.Video.SVGA.Data.CharHeight
			CALL DWORD [Bootes.Devices.Video.Data.FlipDelegate]
		.xMoveBB:
		MOV [Bootes.Devices.Video.Data.Y],DX
		JMP .Exit
	@@:
	CMP ESI,10
	JNE @f
		; Line feed
		MOV WORD [Bootes.Devices.Video.Data.X],0
		JMP .Exit
	@@:
	
	; Traduzione colore
	CMP DI,-1
	JE .eTranslateColor
		OR DI,DI
		JZ .xTranslateColor
		; Light grey
		CMP EDI,7
		JNE @f
			MOV EDI,0xFFBBBBBB
			JMP .xTranslateColor
		@@:
		; Dark grey
		CMP EDI,8
		JNE @f
			MOV EDI,0xFF555555
			JMP .xTranslateColor
		@@:
		; Red
		CMP EDI,4
		JNE @f
			MOV EDI,0xFFFF1111
			JMP .xTranslateColor
		@@:
		; TODO: Altri colori
		MOV EDI,0xFFFFFFFF
		JMP .xTranslateColor
	.eTranslateColor:
		MOV EDI,DWORD [Bootes.Devices.Video.Data.Color]
	.xTranslateColor:

   ; Calcolo la posizione in memoria
   IMUL	EDX,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
   XCHG	EDX,EDI
   LEA	EDI,[EDI+EAX*4]
   ADD	EDI,[Bootes.Devices.Video.Data.BackBuffer]
   
   ; Recupero il carattere
   AND	ESI,0xFF
   SHL	ESI,4
   ADD	ESI,Bootes.Devices.Video.SVGA.Data.RawFont
   
   ; Loop di disegno, ho caratteri alti 16 pixel e larghi 8.
   MOV	ECX,Bootes.Devices.Video.SVGA.Data.CharHeight
   .lYLoop:
   		MOV AL,BYTE [ESI]
   		PUSH EDI
   		PUSH ECX
   		MOV ECX,Bootes.Devices.Video.SVGA.Data.CharWidth
   		.lXLoop:
   			RCL AL,1
   			JNC @f
   			MOV DWORD [EDI],EDX	; Coloro il pixel
   			@@:	; Prossimo pixel
   			ADD EDI,4
   			LOOP .lXLoop
   		POP ECX
   		POP EDI
   		ADD EDI,DWORD [Bootes.Devices.Video.Data.BackBufferBPS]
   		INC ESI
   		LOOP .lYLoop

	; Flip
	MOVZX ESI,WORD [Bootes.Devices.Video.Data.Y]
	MOVZX EAX,WORD [Bootes.Devices.Video.Data.X]
	MOV ECX,Bootes.Devices.Video.SVGA.Data.CharHeight
	MOV EDX,Bootes.Devices.Video.SVGA.Data.CharWidthSpace
	CALL [Bootes.Devices.Video.Data.FlipRectDelegate]
	
	; Aggiorno la posizione
	MOV AX,[Bootes.Devices.Video.Data.X]
	ADD AX,Bootes.Devices.Video.SVGA.Data.CharWidthSpace
	CMP AX,WORD [VesaModeInfo.Width]
	JB @f
		MOV WORD [Bootes.Devices.Video.Data.X],0
		MOVZX EDX,WORD [Bootes.Devices.Video.Data.Y]
		JMP .Return
	@@:
	MOV [Bootes.Devices.Video.Data.X],AX
	
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
; Cancella il carattere nella posizione corrente del cursore.
;====================================================================
align 4
Bootes.Devices.Video.SVGA.DeleteChar:
	PUSH AX
	PUSH BX

	MOV BX,[Bootes.Devices.Video.Data.Y]
	MOV AX,[Bootes.Devices.Video.Data.X]
	CMP AX,Bootes.Devices.Video.SVGA.Data.CharWidthSpace
	JAE @f
		MOV BX,[Bootes.Devices.Video.Data.Y]
		CMP BX,0
		JE .Exit
		MOV AX,[VesaModeInfo.Width]
		MOV WORD [Bootes.Devices.Video.Data.X],AX
		SUB BX,16
		MOV [Bootes.Devices.Video.Data.Y],BX
	@@:
	SUB AX,Bootes.Devices.Video.SVGA.Data.CharWidthSpace
	MOV [Bootes.Devices.Video.Data.X],AX
	CALL Bootes.Devices.Video.SVGA.DrawChar, WORD 219,0
	MOV [Bootes.Devices.Video.Data.X],AX
	MOV [Bootes.Devices.Video.Data.Y],BX
	
	.Exit:
	POP BX
	POP AX
RET



;====================================================================
; Sposta il cursore di scrittura nella posizione indicata.
; --------------------------------------------------------------
; PARAMS
;	X		Posizione X nella quale spostare il cursore.
;	Y		Posizione Y nella quale spostare il cursore.
;====================================================================
Function Bootes.Devices.Video.SVGA.GotoXY, X,Y
Begin
	PUSH AX
	PUSH BX
	PUSH DX
	; X
	MOV AX,[X]
	MOV BX,Bootes.Devices.Video.SVGA.Data.CharWidthSpace
	MUL BX
	MOV [Bootes.Devices.Video.Data.X],AX
	; Y
	MOV AX,[Y]
	SHL AX,4
	MOV [Bootes.Devices.Video.Data.Y],AX
	POP DX
	POP BX
	POP AX
Return



;====================================================================
; Ricava la posizione X del cursore.
; --------------------------------------------------------------
; RETURN
;	Posizione X del cursore.
;====================================================================
align 4
Bootes.Devices.Video.SVGA.GetX:
	PUSH BX
	PUSH DX
	MOV AX,[Bootes.Devices.Video.Data.X]
	XOR DX,DX
	MOV BX,Bootes.Devices.Video.SVGA.Data.CharWidthSpace
	DIV BX
	POP DX
	POP BX
RET



;====================================================================
; Ricava la posizione Y del cursore.
; --------------------------------------------------------------
; RETURN
;	Posizione Y del cursore.
;====================================================================
align 4
Bootes.Devices.Video.SVGA.GetY:
	MOV AX,[Bootes.Devices.Video.Data.Y]
	SHR AX,4
RET



;====================================================================
; Pulisce lo schermo.
;====================================================================
align 4
Bootes.Devices.Video.SVGA.Clear:
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI
	ProtectMode
	MOVZX EBX,WORD [Bootes.Devices.Video.Data.VesaInfoAddr]
	MOVZX EAX,WORD [VesaModeInfo32.Width]
	MOVZX ECX,WORD [VesaModeInfo32.Height]
	IMUL ECX,EAX
	MOV EDI,[Bootes.Devices.Video.Data.BackBuffer]
	XOR EAX,EAX
	REP MOVSD
	CALL DWORD [Bootes.Devices.Video.Data.FlipDelegate]
	RealMode
	MOV WORD [Bootes.Devices.Video.Data.X],0
	MOV WORD [Bootes.Devices.Video.Data.Y],0
	POP DI
	POP SI
	POP CX
	POP BX
	POP AX
RET


; --------------------------------------------------------------
; Data
; --------------------------------------------------------------
Bootes.Devices.Video.SVGA.Data:
	.CharWidth					= 8
	.CharWidthSpace				= .CharWidth ;+1
	.CharHeight					= 16
	.RawFont					file "RawFont.bin"


end if