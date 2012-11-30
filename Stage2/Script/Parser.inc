;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Script.Parser
;
; Permette l'elaborazione di una raccolta di istruzioni script.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::



;====================================================================
; Esegue una successione di comandi letta da un file.
; --------------------------------------------------------------
; PARAMS
;	FileName	Puntatore a una stringa terminata da 0 contenente
;				il percorso del file da leggere.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Script.Parser.ParseFile, FileName
Begin
	PUSH BX
	CALL Bootes.IO.File.Read, [FileName]
	OR AX,AX
	JZ @f
		MOV BX,AX
		CALL Bootes.Script.Parser.Parse, AX
		CALL Bootes.Memory.Free, BX
	@@:
	POP BX
Return



;====================================================================
; Esegue una successione di comandi.
; --------------------------------------------------------------
; PARAMS
;	Text	Puntatore a una stringa terminata da 0.
; RETURN
;	Zero se è avvenuto un errore altrimenti non-zero.
;====================================================================
Function Bootes.Script.Parser.Parse, Text
Begin
	PUSH BX
	PUSH DX
	PUSH SI
	PUSH DI

	; TODO: Cambiare colore linea (?)
	MOV AX,[Text]
	XOR DX,DX
	.lParse:
		CALL Bootes.String.TrimLeft, AX,Bootes.Scritp.Parser.Data.Spaces
		MOV SI,AX
		MOV AX,[SI]
		; Fine testo
		CMP AL,0
		JNE @f
			MOV AX,1
			JMP .xParse
		@@:
		; Salto commenti
		CMP AX,'//'
		JNE .xComment
			CALL Bootes.String.IndexOfAny, SI,Bootes.String.CrLf
			CMP AX,-1
			JE .xParse
			ADD AX,SI
			JMP .lParse
		.xComment:
		; Direttive
		CMP AL,'#'
		JNE .xDirective
			INC SI
			CALL Bootes.String.IndexOfAny, SI,Bootes.Scritp.Parser.Data.SpacesComment
			CMP AX,-1
			JNE @f
				CALL Bootes.String.GetEndPoint,SI
			@@:
			MOV DI,AX
			ADD DI,SI
			MOV BL,[DI]
			MOV BYTE [DI],0
			; Switch direttive
			CALL Bootes.String.Compare, SI,Bootes.Scritp.Parser.Data.DirectiveIngoreError
			OR AX,AX
			JNZ @f
				OR DL,Bootes.Scritp.Parser.Data.IgnoreErrorFlag
			@@:
			CALL Bootes.String.Compare, SI,Bootes.Scritp.Parser.Data.DirectiveNoEcho
			OR AX,AX
			JNZ @f
				OR DL,Bootes.Scritp.Parser.Data.NoEchoFlag
			@@:
			.nDirective:
			; Prossima linea
			MOV [DI],BL
			MOV AX,DI
			JMP .lParse
		.xDirective:
		; Comando
		; TODO: Saltare stringhe (!)
		CALL Bootes.String.IndexOf, SI,Bootes.Scritp.Parser.Data.Semicolon
		CMP AX,-1
		JNE @f
			ErrorSetup ParseMissingSemicolon
			XOR AX,AX
			JMP .Exit
		@@:
		MOV DI,AX
		ADD DI,SI
		MOV BYTE [DI],0
		; Echo
		TEST DL,Bootes.Scritp.Parser.Data.NoEchoFlag
		JNZ @f
			CALL Bootes.IO.Standard.Output, SI
			CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
		@@:
		; Esecuzione
		CALL Bootes.Script.Scripter.Execute, SI
		OR AX,AX
		JZ .iCommandError
		TEST WORD [Bootes.Script.Scripter.Data.LastResultType],Bootes.System.Types.Flags.NoError
		JNZ .xCommandError
		CMP WORD [Bootes.Script.Scripter.Data.LastResult],0
		JNE .xCommandError
		.iCommandError:
			; Errore esecuzione
			INC DH
			TEST DL,Bootes.Scritp.Parser.Data.IgnoreErrorFlag
			JNZ .xCommandError
			ErrorStringPointer AX
			CALL Bootes.IO.Standard.Error, AX
			MOV AX,[Bootes.Error.Data.LastErrorExtension]
			OR AX,AX
			JZ .Error
				CALL Bootes.IO.Standard.Error, AX
				MOV WORD [Bootes.Error.Data.LastErrorExtension],0
			JMP .Error
		.xCommandError:
		MOV AX,DI
		INC AX
		JMP .lParse
	.xParse:
	CMP DH,0
	JE .Exit
	.Error:
	ErrorSetup ParseError
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP BX
Return

; --------------------------------------------------------------
; Data
; --------------------------------------------------------------
align 2
Bootes.Scritp.Parser.Data:
	.SpacesComment			DB '/'
	.Spaces					DB ' ',9,13,10,0
	.Semicolon				DB ';'
	;
	.DirectiveIngoreError	DB "ignore_error",0
	.IgnoreErrorFlag		= 0x0001
	.DirectiveNoEcho		DB "no_echo",0
	.NoEchoFlag				= 0x0002