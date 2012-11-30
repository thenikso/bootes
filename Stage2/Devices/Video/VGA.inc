;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Devices.Video.VGA
;
; Funzioni grafiche testuali.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



;====================================================================
; Scrive una carattere a video nella console corrente e sposta il 
; cursore.
; --------------------------------------------------------------
; PARAMS
;	Char		Carattere da scrivere.
;	Color		Colore del carattere.
;====================================================================
Function Bootes.Devices.Video.VGA.DrawChar, Char,Color
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	
	; Caratteri speciali
	MOV AX,[Char]
	CMP AL,13
	JE .Special
	CMP AL,10
	JE .Special
	CMP AL,8
	JE .Special
	CMP AL,7
	JE .Special
	
	; Prendo la posizione del cursore
	MOV AH,0x03
	MOV BH,[Bootes.IO.Console.Data.ActiveConsole]
	INT 0x10
	; Scrivo nella posizione corrente
	MOV AX,[Char]
	MOV AH,0x09
	MOV BX,[Color]
	MOV BH,[Bootes.IO.Console.Data.ActiveConsole]
	MOV CX,1
	INT 0x10
	; Incremento la posizione del cursore
	INC DL
	MOV AH,0x02
	INT 0x10
	; Risultato
	MOV AX,DX
	JMP .Exit
	
	.Special:
	MOV AH,0x0E
	INT 0x10
	.Exit:
	POP DX
	POP CX
	POP BX
	POP AX
Return



;====================================================================
; Cancella il carattere nella posizione corrente del cursore.
;====================================================================
align 4
Bootes.Devices.Video.VGA.DeleteChar:
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	
	; Prendo la posizione del cursore
	MOV AH,0x03
	MOV BH,[Bootes.IO.Console.Data.ActiveConsole]
	INT 0x10
	; Decremento la posizione del cursore
	DEC DL
	MOV AH,0x02
	INT 0x10
	; Cancello il carattere
	MOV AX,0x0900
	MOV BX,[Bootes.IO.Console.Data.DefaultWriteColor]
	MOV BH,[Bootes.IO.Console.Data.ActiveConsole]
	MOV CX,1
	INT 0x10
	
	POP DX
	POP CX
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
Function Bootes.Devices.Video.VGA.GotoXY, X,Y
Begin
	PUSH AX
	PUSH BX
	PUSH DX
	
	MOV BH,[Bootes.IO.Console.Data.ActiveConsole]
	MOV AX,[Y]
	MOV DX,[X]
	SHL AX,8
	AND DX,0xFF
	OR DX,AX
	MOV AH,0x02
	INT 0x10
	
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
Function Bootes.Devices.Video.VGA.GetX
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	
	MOV BH,[Bootes.IO.Console.Data.ActiveConsole]
	MOV AH,0x03
	INT 0x10
	MOV AX,DX
	AND AX,0xFF
	
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Ricava la posizione Y del cursore.
; --------------------------------------------------------------
; RETURN
;	Posizione Y del cursore.
;====================================================================
Function Bootes.Devices.Video.VGA.GetY
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	
	MOV BH,[Bootes.IO.Console.Data.ActiveConsole]
	MOV AH,0x03
	INT 0x10
	MOV AX,DX
	SHR AX,8
	
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Pulisce lo schermo.
;====================================================================
Function Bootes.Devices.Video.VGA.Clear
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH DX
	
	; Mi posiziono all'inizio
	MOV AH,0x02
	MOV BH,[Bootes.IO.Console.Data.ActiveConsole]
	XOR DX,DX
	INT 0x10
	; Scrivo nella posizione corrente
	MOV AX,0x0920
	MOV BL,7
	MOV CX,(80*25)
	INT 0x10
	; Mi riporto posiziono all'inizio
	MOV AH,0x02
	INT 0x10
	
	POP DX
	POP CX
	POP BX
	POP AX
Return
