;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.IO.Console
;
; Console di interfacciamento.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2


; --------------------------------------------------------------
; Funzioni esportate
; --------------------------------------------------------------
SysNamespace Bootes.IO.Console_Namespace, "IO.Console"
	Export Bootes.IO.Console.Write,1, "Write", Type.Null, Type.String
	Export Bootes.IO.Console.WriteError,1, "WriteError", Type.Null, Type.String
	Export Bootes.IO.Console.Read,0, "Read", Type.String


;====================================================================
; Console di sistema. Per avviare una nuova istanza della console
; chiamare 
;====================================================================
align 4
Bootes.IO.Console.Init:
	CALL Bootes.Script.Comander.AddNamespace, Bootes.IO.Console_Namespace
RET


;====================================================================
; Console di sistema. Per avviare una nuova istanza della console
; chiamare 
;====================================================================
align 4
Bootes.IO.Console:
	; --------------------------------------------------------------
	; Inizializzazione nuova console
	; --------------------------------------------------------------
	; Inizializzo la memoria dove salvare il comando corrente
	CALL Bootes.Memory.Alloc, Bootes.Stage2.Info.DefaultStringLen
	OR AX,AX
	JZ .Error
	MOV [Bootes.IO.Console.Data.ReadBuffer],AX
	; TODO: pulire schermo (vedi int 0x10 AH=6)
	; Imposto il cursore
	CALL Bootes.Devices.Video.SetTextCursor, 0
	; Imposto le funzioni di IO standard
	CALL Bootes.IO.Standard.SetOutput, Bootes.IO.Console.Write
	CALL Bootes.IO.Standard.SetInput, Bootes.IO.Console.Read
	CALL Bootes.IO.Standard.SetError, Bootes.IO.Console.WriteError
	
	; --------------------------------------------------------------
	; Main console loop
	; --------------------------------------------------------------
	; Prompt
	CALL Bootes.Devices.Video.DrawChar, WORD 222,[Bootes.IO.Console.Data.DefaultPrefixColor]
	CALL Bootes.Devices.Video.DrawChar, WORD ' ',[Bootes.IO.Console.Data.DefaultReadColor]
	.lMain:
		; Aspetto un input
		CALL Bootes.IO.Console.Read
		; Controllo se ho un input vuoto
		MOV BX,AX
		CMP BYTE [BX],0
		JE .lMain
		; Avvio il comando
		CALL Bootes.Script.Scripter.Execute, AX
		OR AX,AX
		JZ .iError
		; Controllo risultato
		TEST WORD [Bootes.Script.Scripter.Data.LastResultType],Bootes.System.Types.Flags.NoError
		JNZ .xError
		CMP WORD [Bootes.Script.Scripter.Data.LastResult],0
		JNE .xError
		.iError:
			; Messaggio di errore
			ErrorStringPointer AX
			CALL Bootes.IO.Console.WriteError, AX
			MOV AX,[Bootes.Error.Data.LastErrorExtension]
			OR AX,AX
			JZ @f
				CALL Bootes.IO.Console.WriteError, AX
				MOV WORD [Bootes.Error.Data.LastErrorExtension],0
			@@:
		.xError:
		JMP .lMain

	JMP .Exit
	.Error:
	; TODO: Inviare un messaggio di errore allo standard error (via comander script)
	.Exit:
	CALL Bootes.Task.Stop
	RET



;====================================================================
; Scrive una stringa nella console corrente.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da zero da scrivere.
;====================================================================
Function Bootes.IO.Console.Write, String
Begin
	PUSH AX
	PUSH BX
	PUSH SI
	
	; --------------------------------------------------------------
	; Message text
	; --------------------------------------------------------------
	MOV SI,[String]
	OR SI,SI
	JZ .Exit
	.lText:
		LODSB
		OR AL,AL
		JZ .Exit
		; --------------------------------------------------------------
		; Controllo posizione cursore
		; --------------------------------------------------------------
		; TODO
		; --------------------------------------------------------------
		; caratteri speciali
		; --------------------------------------------------------------
		; Escapes
		CMP AL,'\'
		JNE .xEscape
		.iEscape:
			LODSB
			
			; Return: \n
			CMP AL,'n'
			JNE .xEscapeReturn
			.iEscapeReturn:
				; Vado a capo
;				MOV AH,0x0E
;				MOV AL,13
;				INT 0x10
;				MOV AL,10
;				INT 0x10
				CALL Bootes.Devices.Video.DrawChar, WORD 13,[Bootes.IO.Console.Data.DefaultWriteColor]
				CALL Bootes.Devices.Video.DrawChar, WORD 10,[Bootes.IO.Console.Data.DefaultWriteColor]
				; Scrivo il prefisso standard
				CALL Bootes.Devices.Video.DrawChar, WORD 222,[Bootes.IO.Console.Data.DefaultPrefixColor]
				CALL Bootes.Devices.Video.DrawChar, WORD ' ',[Bootes.IO.Console.Data.DefaultReadColor]
				JMP .lText
			.xEscapeReturn:
			
			; Tab: \t
			CMP AL,'t'
			JNE .xEscapeTab
			.iEscapeTab:
				; TODO: Migliorare rendendo tutti i tab uniformi (con GetX...)
				PUSH CX
				MOV CX,[Bootes.IO.Console.Data.TabValue]
				@@:
					MOV AH,0x0E
					MOV AL,' '
					INT 0x10
				LOOP @b
				POP CX
				JMP .lText
			.xEscapeTab:
			
			; Slash: \\
			CMP AL,'\'
			JE .Write
			
			; Fine stringa
			OR AL,AL
			JZ .Exit
			
			; Altro, non stampo
			JMP .lText
		.xEscape:
		; Return
		CMP AL,13
		JNE .xReturn
		.iReturn:
			MOV AL,[SI]
			CMP AL,10
			JNE @f
				INC SI
			@@:
			JMP .iEscapeReturn
		.xReturn:
		; --------------------------------------------------------------
		; Scrivo
		; --------------------------------------------------------------
		.Write:
		CALL Bootes.Devices.Video.DrawChar, AX,[Bootes.IO.Console.Data.DefaultWriteColor]
		JMP .lText
	
	.Exit:
	POP SI
	POP BX
	POP AX
Return



;====================================================================
; Scrive una stringa nella console corrente e va a capo.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da zero da scrivere.
;====================================================================
Function Bootes.IO.Console.WriteLine, String
Begin
	CALL Bootes.IO.Console.Write, [String]
	CALL Bootes.IO.Console.Write, Bootes.String.CrLf
Return



;====================================================================
; Scrive una stringa di errore nella console corrente e va a capo.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da zero da scrivere.
;====================================================================
Function Bootes.IO.Console.WriteError, String
Begin
	CALL Bootes.Devices.Video.GetY
	CALL Bootes.Devices.Video.GotoXY,0,AX
	MOV WORD [Bootes.IO.Console.Data.DefaultPrefixColor],Bootes.Stage2.Info.ErrorColor
	CALL Bootes.Devices.Video.DrawChar, WORD 222,[Bootes.IO.Console.Data.DefaultPrefixColor]
	CALL Bootes.Devices.Video.DrawChar, WORD ' ',[Bootes.IO.Console.Data.DefaultReadColor]
	CALL Bootes.IO.Console.Write, [String]
	MOV WORD [Bootes.IO.Console.Data.DefaultPrefixColor],Bootes.Stage2.Info.LineColor
	CALL Bootes.IO.Console.Write, Bootes.String.CrLf
Return



;====================================================================
; Legge una stinga nella console corrente.
; --------------------------------------------------------------
; RETURN
;	Puntatore alla stringa letta.
;====================================================================
Function Bootes.IO.Console.Read
Begin
	; CX: Contiene il numero massimo di caratteri da leggere.
	; DX: Punta all'inizio del buffer di scrittura.
	; DI: Punta alla posizione corrente del buffer nel buffer.
	; BX: Contiene il numero di ripetizioni di TAB consecutive.
	; SI: Contiene la posizione nel buffer dell'ultimo carattere prima
	;	  di un TAB.
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI

	; --------------------------------------------------------------
	; Imposto il buffer di lettura
	; --------------------------------------------------------------
	MOV DX,[Bootes.IO.Console.Data.ReadBuffer]
	MOV DI,DX
	
	; --------------------------------------------------------------
	; Imposto lunghezza massima
	; --------------------------------------------------------------
	MOV CX,Bootes.Stage2.Info.DefaultStringLen
	DEC CX

	; --------------------------------------------------------------
	; Azzero ripetizioni TAB
	; --------------------------------------------------------------	
	XOR BX,BX
	
	; --------------------------------------------------------------
	; Ciclo di attesa carattere
	; --------------------------------------------------------------
	.lKey:
	CALL Bootes.Devices.Keyboard.WaitKey
	
	; --------------------------------------------------------------
	; Controllo caratteri particolari
	; --------------------------------------------------------------
	; Return -------------------------------------------------------
	CMP AL,13
	JNE .xReturn
	.iReturn:
		; Risistemo i colori
		MOV WORD [Bootes.IO.Console.Data.DefaultReadColor],Bootes.Stage2.Info.InputColor
		; Scrivo un line return e un line feed
		CALL Bootes.IO.Console.Write, Bootes.String.CrLf
		; Scrivo il carattere di fine stringa
		MOV BYTE [DI],0
		; Ritorno il valore letto
		MOV AX,DX
		JMP .Exit
	.xReturn:
	; Backspace ----------------------------------------------------
	CMP AL,8
	JNE .xBackspace
	.iBackspace:
		; Azzero ripetizioni TAB
		XOR BX,BX
		; Decremento il puntatore nel buffer
		DEC DI
		; Controllo se è consentito usare il backspace
		CMP DI,DX
		JAE .eNoBackspace
		.iNoBackSpace:
			MOV DI,DX
			JMP .lKey
		.eNoBackspace:
			; Cancello il carattere dallo schermo
			CALL Bootes.Devices.Video.DeleteChar
			INC CX
			; Sistemo i colori delle stringhe
			CMP BYTE [DI],'"'
			JNE .lKey
				CMP BYTE [Bootes.IO.Console.Data.DefaultReadColor],Bootes.Stage2.Info.InputColor
				JNE @f
					MOV WORD [Bootes.IO.Console.Data.DefaultReadColor],Bootes.Stage2.Info.StringColor
					JMP .lKey
				@@:
				MOV WORD [Bootes.IO.Console.Data.DefaultReadColor],Bootes.Stage2.Info.InputColor
				JMP .lKey
	.xBackspace:
	; Tab ----------------------------------------------------------
	CMP AL,9
	JNE .xTab
	.iTab:
		; Gestione ripetizione TAB
		OR BX,BX
		JZ .eTabRepetition
		.iTabRepetition:
			; Cancello il testo precedente
			CALL Bootes.String.Length, SI
			ADD CX,AX
			; TODO: Rivedere cancellazione da schermo
			PUSH CX
			MOV CX,AX
			@@:
				CALL Bootes.Devices.Video.DeleteChar
			LOOP @b
			POP CX
			; Reimposto la vecchia stringa
			MOV BYTE [SI],0
			MOV DI,SI
			JMP .xTabRepetition
		.eTabRepetition:
			MOV BYTE [DI],0
			MOV SI,DI
		.xTabRepetition:
		; Prendo la stringa da completare
		CALL Bootes.String.LastIndexOf, DX,Bootes.String.Space
		CMP AX,-1
		JE .eHaveSpace
		.iHaveSpace:
			INC AX
			ADD AX,DX
			JMP .xHaveSpace
		.eHaveSpace:
			MOV AX,DX
		.xHaveSpace:
		;CALL Bootes.String.TrimLeft, DX,0
		CALL Bootes.Script.Helper.Complete, AX,BX
		OR AX,AX
		JNZ @f
			XOR BX,BX
			JMP .lKey
		@@:
		; Incremento le ripetizioni
		INC BX
		; Completo
		PUSH SI
		MOV SI,AX
		.lComplete:
			; Carico il prossimo carattere
			LODSB
			OR AL,AL
			JZ .xlComplete
			; Controllo overflow
			CMP CX,0
			JBE .xlComplete
			DEC CX
			; Scrivo in memoria
			STOSB
			; Scrivo a video
			CALL Bootes.Devices.Video.DrawChar, AX,[Bootes.IO.Console.Data.DefaultReadColor]
			JMP .lComplete
		.xlComplete:
		POP SI
		MOV BYTE [DI],0
		JMP .lKey
	.xTab:
	; String -------------------------------------------------------
	CMP AL,'"'
	JNE .xString
		CMP BYTE [Bootes.IO.Console.Data.DefaultReadColor],Bootes.Stage2.Info.InputColor
		JNE @f
			MOV WORD [Bootes.IO.Console.Data.DefaultReadColor],Bootes.Stage2.Info.StringColor
			JMP .Echo
		@@:
		; Controllo overflow
		CMP CX,0
		JBE .lKey
		DEC CX
		; Azzero ripetizioni TAB
		XOR BX,BX
		; Scrivo in memoria
		STOSB
		; Scrivo a video
		CALL Bootes.Devices.Video.DrawChar, AX,[Bootes.IO.Console.Data.DefaultReadColor]
		MOV WORD [Bootes.IO.Console.Data.DefaultReadColor],Bootes.Stage2.Info.InputColor
		JMP .lKey
	.xString:
	; Altri --------------------------------------------------------
	CMP AL,0
	JNE .xScanCode
	.iScanCode:
		; Prendo lo scancode
		; Left Arrow
		CMP AH,75
		JNE .xSCLeftArrow
		.iSCLeftArrow:
;			; Controllo se è possibile spostarsi a sinistra
;			MOV AX,DI
;			SUB AX,DX
;			OR AX,AX
;			JZ .lKey
;			; Sposto
;			PUSH CX
;			PUSH DX
;			; Get cursor position
;			MOV AH,0x03
;			INT 0x10
;			; Cambio la posizione del cursore
;			MOV AH,0x02
;			DEC DL
;			INT 0x10
;			POP DX
;			POP CX
;			; Sposto il puntatore di scrittura
;			DEC DI
;			JMP .lKey
		.xSCLeftArrow:
		; Right Arrow
		CMP AH,77
		JNE .xSCRightArrow
		.iSCRightArrow:	
;			; Controllo se è possibile spostarsi a destra
;			CMP BYTE [DI],0
;			JE .lKey
;			
;			PUSH CX
;			PUSH DX
;			; Get cursor position
;			MOV AH,0x03
;			INT 0x10
;			; Cambio la posizione del cursore
;			INC DL
;			; Sposto
;			MOV AH,0x02
;			INT 0x10
;			POP DX
;			POP CX
;			; Sposto il puntatore di scrittura
;			INC DI
;			JMP .lKey
		.xSCRightArrow:
		; Home
		CMP AH,71
		JNE .xSCHome
		.iSHome:
			; TODO
		.xSCHome:
		; End
		CMP AH,79
		JNE .xSCEnd
		.iSEnd:
			; TODO
		.xSCEnd:
		; Ignoro
		JMP .lKey
	.xScanCode:
	
	; --------------------------------------------------------------
	; Salvo nella memoria
	; --------------------------------------------------------------
	.Echo:
	; Controllo overflow
	CMP CX,0
	JBE .lKey
	DEC CX
	; Azzero ripetizioni TAB
	XOR BX,BX
	; Scrivo in memoria
	STOSB
	; Scrivo a video
	CALL Bootes.Devices.Video.DrawChar, AX,[Bootes.IO.Console.Data.DefaultReadColor]
	JMP .lKey


	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return

;====================================================================
; Data
;====================================================================
Bootes.IO.Console.Data:
	.ReadBuffer						DW 0
	;
	.DefaultWriteColor				DW Bootes.Stage2.Info.OutputColor
	.DefaultReadColor				DW Bootes.Stage2.Info.InputColor
	.DefaultPrefixColor				DW Bootes.Stage2.Info.LineColor
	;
	.ActiveConsole					DB 0
	.InsStatus						DB 0
	;
	.TabValue						DW 5