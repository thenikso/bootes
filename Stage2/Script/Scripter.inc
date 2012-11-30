;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Script.Scripter
;
; Parser di script.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2

; --------------------------------------------------------------
; Strutture e virtuals
; --------------------------------------------------------------
STRUC Bootes.Script.Scripter.Variable
{
	.Start:
	.NamePointer			DW ?
	.Value					DW ?
	.ValueType				DW ?	; Tipo del valore: 2=numero, 1=stringa
	.Size					= $-.Start
}

VIRTUAL AT DI
	CurrentVar				Bootes.Script.Scripter.Variable
END VIRTUAL

VIRTUAL AT SI
	TempFuncLink			Bootes.Script.Comander.FunctionLink
END VIRTUAL



;====================================================================
; Inizilizza lo scripter
;====================================================================
Bootes.Script.Scripter.Init:
	; Alloco lo spazio per la struttura di parametri
	CALL Bootes.Memory.Alloc, Bootes.Stage2.Info.MaxFuncParameters*4+2
	MOV [Bootes.Script.Scripter.Data.FunctionParams],AX			
	; Alloco lo spazio necessario alle variabili
	CALL Bootes.Memory.Alloc, ((Bootes.Stage2.Info.MaxVars+1)*CurrentVar.Size)
	OR AX,AX
	JZ @f
		PUSH CX
		PUSH DI
		
		MOV [Bootes.Script.Scripter.Data.VarsList],AX
		MOV DI,AX
		; Imposto la variabile risposta
		MOV WORD [CurrentVar.NamePointer],Bootes.Script.Scripter.Data.AnswareName
		MOV WORD [CurrentVar.Value],0
		MOV WORD [CurrentVar.ValueType],Bootes.System.Types.Number
		ADD DI,CurrentVar.Size
		; Azzero la memoria
		MOV CX,(Bootes.Stage2.Info.MaxVars*CurrentVar.Size/2)
		XOR AX,AX
		REP STOSW
		
		POP DI
		POP CX
	@@:
RET



;====================================================================
; Esegue il comando specificato.
; --------------------------------------------------------------
; PARAMS
;	Command		Puntatore alla stringa terminata da 0 contenente il
;				comando da eseguire. La stringa puntata può venir
;				modificata dalla funzione.
; RETURN
;	Zero se l'esecuzione ha incontrato errori.
;====================================================================
Function Bootes.Script.Scripter.Execute, Command
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	MOV SI,[Command]
	; TODO: Verificare se questo va bene qui
	MOV WORD [Bootes.Script.Scripter.Data.LastResult], 1
	
	; --------------------------------------------------------------
	; Controllo la presenza di una direttiva
	; --------------------------------------------------------------
	CALL Bootes.String.SplitLeft, SI,Bootes.Script.Scripter.Data.DirectiveEndSign
	CMP AX,1
	; Nessuna direttiva
	JE DWORD .Directive_NONE
	; Possibile direttiva presente
	MOV DI,Bootes.Script.Scripter.Data.Directives
	XOR BX,BX
	.lCheckDirective:
		; Controllo fine ciclo
		CMP BYTE [DI],0
		JE .xCheckDirective
		; Controllo la direttiva
		CALL Bootes.String.Compare, SI,DI
		OR AX,AX
		JZ .xCheckDirective
		; Prossima direttiva
		CALL Bootes.String.GetEndPoint, DI
		INC AX
		MOV DI,AX
		ADD BX,.JumpSize
		JMP .lCheckDirective
	.xCheckDirective:
	; --------------------------------------------------------------
	; Salti alle drettive
	; --------------------------------------------------------------
	LEA AX,[.DirectiveJumps+BX]
	JMP AX
	.DirectiveJumps:
	JMP DWORD .Directive_help
	.JumpSize	= $-.DirectiveJumps
	JMP DWORD .Directive_using
	JMP DWORD .Directive_valueof
	JMP DWORD .Directive_delete
	JMP DWORD .Directive_NONE_Resume
	; --------------------------------------------------------------
	; Handler delle direttive
	; --------------------------------------------------------------
	.Directive_help:
		CALL Bootes.String.GetEndPoint, SI
		INC AX
		CALL Bootes.String.Trim, AX,Bootes.Script.Scripter.Data.FunctionNoiseChars
		; TODO: con stringa vuota fai partire il tutorial(?)
		CALL Bootes.Script.Comander.GetFunction, AX,-1
		OR AX,AX
		JNZ @f
			ErrorSetup FunctionNotFound
			JMP .Error
		@@:
		CALL Bootes.Script.Helper.Help, AX
		OR AX,AX
		JNZ .Exit
		; Scrivo che non c'è help
		ErrorSetup NoHelp
		JMP .Error
	.Directive_using:
		; Prendo il namespace da usare
		CALL Bootes.String.GetEndPoint, SI
		INC AX
		CALL Bootes.Script.Comander.Using, AX
		OR AX,AX
		JNZ .Exit
		JMP .Error
	.Directive_valueof:
		; restituisco il valore della variabile
		CALL Bootes.String.GetEndPoint, SI
		INC AX
		CALL Bootes.String.Trim, AX,0
		CALL Bootes.Script.Scripter.GetVarValue, AX,Bootes.Script.Scripter.Data.LastResult
		CMP AX,1
		JB .Error
		JNE @f
			; E' una stringa
			CALL Bootes.IO.Standard.Output, Bootes.Script.Scripter.Data.ValueOf.String
			CALL Bootes.IO.Standard.Output, [Bootes.Script.Scripter.Data.LastResult]
			CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
			JMP .Exit	
		@@:
			; E' un numero
			CALL Bootes.String.FromNumber, Bootes.Script.Scripter.Data.ValueOf.Number+10,0,[Bootes.Script.Scripter.Data.LastResult],16,16
			CALL Bootes.IO.Standard.Output, Bootes.Script.Scripter.Data.ValueOf.Number
			JMP .Exit

	.Directive_delete:
		; rimuove la variabile
		CALL Bootes.String.GetEndPoint, SI
		INC AX
		CALL Bootes.String.Trim, AX,0
		CALL Bootes.Script.Scripter.DeleteVar, AX
		JMP .Exit
		
	; --------------------------------------------------------------
	; Non ho una direttiva
	; --------------------------------------------------------------
	.Directive_NONE_Resume:
	; Ripristino il valore del comando
	CALL Bootes.String.GetEndPoint, SI
	MOV DI,AX
	MOV BYTE [DI],' '
	.Directive_NONE:
	; Ricerca assegnazione variabile '='
	CALL Bootes.String.SplitLeft, SI,Bootes.Script.Scripter.Data.VarSign
	CMP AX,2
	JNE .eVarAssign
	.iVarAssign:
		CALL Bootes.String.GetEndPoint, SI
		MOV DI,AX
		INC AX
		; Recupero il valore
		CALL Bootes.Script.Scripter.Evalue, AX,Bootes.Script.Scripter.Data.LastResult,Bootes.Script.Scripter.Data.LastResultType
		OR AX,AX
		JZ .ResumeCommand
		; Imposto la variabile
		CALL Bootes.String.Trim, SI,0
		CALL Bootes.Script.Scripter.SetVarValue, AX,[Bootes.Script.Scripter.Data.LastResult],[Bootes.Script.Scripter.Data.LastResultType]
		OR AX,AX
		JNZ .Exit
		; Ripristino il valore originale del comando
		.ResumeCommand:
		CALL Bootes.String.GetEndPoint, SI
		MOV BX,AX
		MOV BYTE [BX],' '
		MOV BYTE [DI],'='
	.eVarAssign:
		; Provo a valutare il comando
		CALL Bootes.Script.Scripter.Evalue, SI,Bootes.Script.Scripter.Data.LastResult,Bootes.Script.Scripter.Data.LastResultType
		OR AX,AX
		JNZ .Exit
	
	.Error:
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Risolve l'espressione specificata. Una funzione deve terminare con 
; il carattere FunctionEndSign ')'.
; --------------------------------------------------------------
; PARAMS
;	Expression	Puntatore alla stringa terminata da 0 contenente
;				l'espressione da risolvere. La stringa puntata può 
;				venir modificata dalla funzione.
;	RetValue	Puntatore a una zona di memoria dove verrà salvato
;				il valore di ritorno. Oppure 0 se non si desidera 
;				recuperare tale valore.
; RETURN
;	Zero se è avvenuto un errore nella risoluzione dell'espressione.
;====================================================================
Function Bootes.Script.Scripter.Evalue, Expression,RetValue,RetValueType
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	CALL Bootes.String.TrimLeft, [Expression],0
	MOV SI,AX
	; --------------------------------------------------------------
	; Vedo se è una funzione
	; --------------------------------------------------------------
	XOR CX,CX
	CALL Bootes.String.SplitLeft, SI,Bootes.Script.Scripter.Data.FunctionSign
	CMP AX,2
	JNE .xFunction
	; Trovo l'inizio dei parametri
	CALL Bootes.String.GetEndPoint, SI
	MOV CX,AX
	INC AX
	MOV DX,AX
	; Cerco la funzione
	CALL Bootes.String.TrimRight, SI,0
	CALL Bootes.Script.Comander.GetFunction, SI,-1
	OR AX,AX
	JZ .xFunction
	.iFunction:
		MOV SI,AX	; SI = Puntatore alla struttura della funzione
		; --------------------------------------------------------------
		; Recupero i parametri per la funzione
		; --------------------------------------------------------------
		MOV DI,DX
		CALL Bootes.String.Split, DI,Bootes.Script.Scripter.Data.ParamsSplitter
		OR AX,AX
		JNZ @f
			ErrorSetup MissingBracket
			JMP .Error
		@@:
		MOV CX,AX
		; --------------------------------------------------------------
		; Preparo la struttura per i parametri
		; --------------------------------------------------------------	
		MOV BX,[Bootes.Script.Scripter.Data.FunctionParams]
		OR BX,BX
		JNZ @f
			ErrorSetup AllocFail
			JMP .Error
		@@:
		ADD BX,2	; BX = Puntatore primo parametro della struttura parametri
		
		; --------------------------------------------------------------
		; Evaluto i CX parametri che cominciano da DI
		; --------------------------------------------------------------
		XOR DX,DX
		XOR AX,AX
		.lCheckParams:
			; --------------------------------------------------------------
			; Verifiche parametro
			; --------------------------------------------------------------
			; Se è l'ultimo parametro deve terminare con FunctionEndSign ')'
			CMP CX,1
			JNE @f
				PUSH DI
				CALL Bootes.String.GetEndPoint, DI
				DEC AX
				MOV DI,AX
				MOV AL,[DI]
				MOV BYTE [DI],0
				POP DI
				CMP AL,[Bootes.Script.Scripter.Data.FunctionEndSign]
				JE @f
					ErrorSetup MissingBracket
					JMP .Error
			@@:
			; Trim di spazi e stringhe vuote
			CALL Bootes.String.TrimLeft, DI,0
			MOV DI,AX
			; Verifica si stringa vuota
			MOV AL,[DI]
			CMP AL,0
			JE .Continue
			; Verifica completezza di una stringa
			CMP AL,'"'
			JNE .xCheckString
			.iCheckString:
				PUSH BX
				CALL Bootes.String.TrimRight, DI,0
				OR AX,AX
				JZ .eHaveEndSpace
				.iHaveEndSpace:
					MOV BX,AX
					MOV BYTE [BX],' '
					DEC BX
					CMP BX,DI
					JE .eHaveEndSpace
					CMP BYTE [BX],'"'
					JNE .eHaveEndSpace
					JMP .xhaveEndSpace
				.eHaveEndSpace:
					CALL Bootes.String.GetEndPoint, DI
				.xhaveEndSpace:
				; Controllo se ho delle "," nella stringa
				DEC AX
				MOV BX,AX
				CMP AX,DI
				JE .iAddStringComma
				CMP BYTE [BX],'"'
				JE .xAddStringComma
				.iAddStringComma:
					; Scrivo la virgola erroneamente tolta
					INC BX
					MOV BYTE [BX],','
					; Ricomputo il parametro
					POP BX
					JMP .Continue
				.xAddStringComma:
				POP BX
			.xCheckString:
			; TODO: Verifica completezza di una funzione annidata
			; --------------------------------------------------------------
			; Valutazione parametro
			; TODO: Aggiornare a nuova struttura parametri con specificazione tipo
			; --------------------------------------------------------------
			MOV AX,BX
			ADD AX,2
			CALL Bootes.Script.Scripter.Evalue, DI,BX,AX
			OR AX,AX
			JNZ @f
				ErrorSetup BadParameter,DI
				JMP .Error
			@@:
			ADD BX,4
			; --------------------------------------------------------------
			; Prossimo parametro
			; --------------------------------------------------------------
			; Punto DI al prossimo parametro
			CALL Bootes.String.GetEndPoint, DI
			CMP BYTE [DI],'"'
			JNE .eWasString
			.iWasString:
				INC AX
				MOV DI,AX
				@@:
					INC DI
					CMP BYTE [DI],' '
					JE @b
				CMP BYTE [DI],0
				JNE @f
					INC DI
				@@:
				JMP .xWasString
			.eWasString:
				INC AX
				MOV DI,AX
			.xWasString:	
			; Incremento il numero di parametri
			INC DX
			.Continue:
			DEC CX
			JNZ .lCheckParams
		.xCheckParams:
	
		; --------------------------------------------------------------
		; Controllo che la funzione abbia DX parametri (overload)
		; --------------------------------------------------------------
		CMP DX,[TempFuncLink.ParamsCount]
		JE .xResearch
			; Provo a cercare l'overload corretto
			LEA CX,[TempFuncLink.Name]
			.lOverload:
				MOV SI,[TempFuncLink.Next]
				OR SI,SI
				JZ .xOverload
				LEA AX,[TempFuncLink.Name]
				CALL Bootes.String.Compare, AX,CX
				OR AX,AX
				JNZ .lOverload
				CMP DX,[TempFuncLink.ParamsCount]
				JE .xResearch
				JMP .lOverload
			.xOverload:
			ErrorSetup InvalidParamNum
			JMP .Error
		.xResearch:
		
		; --------------------------------------------------------------
		; Sistemo struttura parametri
		; --------------------------------------------------------------
		MOV AX,DX
		SHL AX,2	; * (Param + Type)[4byte]
		ADD AX,2	; + (Size)[2byte]
		SUB BX,AX
		MOV [BX],DX
		
		; --------------------------------------------------------------
		; Chiamo la funzione
		; --------------------------------------------------------------
		MOV DI,[RetValueType]
		CALL Bootes.Script.Comander.Run, [RetValue],SI,BX,DI
		INC AX
		OR AX,AX
		JZ .Exit
		
		; --------------------------------------------------------------
		; Imposto answare
		; --------------------------------------------------------------
		MOV BX,[Bootes.Script.Scripter.Data.VarsList]
		OR BX,BX
		JZ .Exit
		XCHG BX,DI
		MOV BX,[BX]
		AND BX,0xFF
		OR BX,BX
		JZ .Exit
		MOV WORD [CurrentVar.ValueType],BX
		MOV BX,[RetValue]
		MOV BX,[BX]
		MOV WORD [CurrentVar.Value],BX
		JMP .Exit
	.xFunction:
	
	; --------------------------------------------------------------
	; Provo a risolvere una espressione
	; --------------------------------------------------------------
	; Ripristino la stringa originale se ve ne è bisogno
	CALL Bootes.String.GetEndPoint, SI
	CMP CX,AX
	JB @f
		MOV DI,AX
		MOV BYTE [DI],' '
		MOV DI,CX
		MOV BYTE [DI],'('
	@@:
	; TrimRight spazi
	CALL Bootes.String.TrimRight, SI,0
	MOV DX,AX
	; TODO: espressioni complesse
	MOV BX,[RetValue]
	CALL Bootes.Script.Scripter.GetValueOf, SI,BX
	OR AX,AX
	JZ .InternalError
	; Imposto answare
	MOV DI,[Bootes.Script.Scripter.Data.VarsList]
	OR DI,DI
	JZ @f
		MOV BX,[BX]
		MOV WORD [CurrentVar.Value],BX
		MOV WORD [CurrentVar.ValueType],AX
	@@:
	MOV BX,[RetValueType]
	MOV [BX],AX
	; Ripristino la stringa originale
	OR DX,DX
	JZ .Exit
		MOV SI,DX
		MOV BYTE [SI],' '
		
	JMP .Exit
	.InternalError:
	ErrorSetup InvalidExpression,SI
	.Error:
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Ritorna il valore del parametro associato alla stringa specificata.
; Questa funzione può alterare il valore della stringa passata.
; --------------------------------------------------------------
; PARAMS
;	ParamStr	Puntatore alla stringa terminata da 0 da convertire
;				in parametro.
;	RetValue	Puntatore alla zona di memoria dove salvare il valore
;				del parametro. Se il parametro viene riconosciuto 
;				come numero, viene ritornato il numero associato. Se 
;				viene riconosciuto come stringa è ritornato il 
;				puntatore alla stringa terminata da 0 e liberata dagli 
;				apici. Se viene riconsciuto come variabile, viene 
;				ritornato il valore della variabile.
; RETURN
;	Tipo del valore del parametro oppure zero se è avvenuto un 
;	errore. I tipi di parametro sono definiti in Types.inc
;====================================================================
Function Bootes.Script.Scripter.GetValueOf, ParamStr, RetValue
Begin
	PUSH BX
	PUSH SI
	PUSH DI
	
	MOV BX,[RetValue]
	MOV SI,[ParamStr]
	LODSB
	OR AL,AL
	JZ .Error
	; --------------------------------------------------------------
	; Controllo se è una stringa
	; --------------------------------------------------------------
	CMP AL,'"'
	JNE .xString
	.iString:
		CALL Bootes.String.GetEndPoint, SI
		DEC AX
		MOV DI,AX
		CMP BYTE [DI],'"'
		JNE .Error
		MOV BYTE [DI],0
		MOV AX,Bootes.System.Types.String
		MOV [BX],SI
		JMP .Exit
	.xString:
	; --------------------------------------------------------------
	; Controllo sia un numero
	; TODO: Controllo errori
	; --------------------------------------------------------------
	MOV AH,AL
	SUB AH,'0'
	CMP AH,9
	JA .xNumber
	.iNumber:
		; Verifico se è in esadecimale
		CMP BYTE [SI],'x'
		JNE .xHex
		.iHex:
			INC SI
			CALL Bootes.String.ToHex, SI
			MOV [BX],AX
			MOV AX,Bootes.System.Types.Number
			JMP .Exit
		.xHex:
		DEC SI
		; Verifico se è binario
		CALL Bootes.String.GetEndPoint, SI
		SUB AX,2
		MOV DI,AX
		CMP WORD [DI],".b"
		JNE .xBin
		.iBin:
			MOV BYTE [DI],0
			CALL Bootes.String.ToBin, SI
			MOV [BX],AX
			MOV AX,Bootes.System.Types.Number
			JMP .Exit
		.xBin:
		; Converto da decimale
		; TODO: Verificare se la confersione è stata possibile
		CALL Bootes.String.ToDec, SI
		MOV [BX],AX
		MOV AX,Bootes.System.Types.Number
		JMP .Exit
	.xNumber:
	; --------------------------------------------------------------
	; Controllo sia una variabile
	; --------------------------------------------------------------
	DEC SI
	CALL Bootes.Script.Scripter.GetVarValue, SI,[RetValue]
	OR AX,AX
	JNZ .Exit
	
	.Error:
	MOV AX,Bootes.System.Types.Void
	.Exit:
	POP DI
	POP SI
	POP BX
Return



;====================================================================
; Ritorna il valore della variabile specificata, se la variabile
; non viene trovata ritorna un errore.
; --------------------------------------------------------------
; PARAMS
;	VarName		Nome della variabile da recuperare.
;	RetValue	Puntatore alla zona di memoria dove salvare il valore
;				della variabile.
; RETURN
;	Zero in caso di errore altrimenti il tipo della variabile.
;	I tipi sono definiti in Types.inc
;====================================================================
Function Bootes.Script.Scripter.GetVarValue, VarName, RetValue
Begin
	PUSH CX
	PUSH SI
	PUSH DI
	
	; --------------------------------------------------------------
	; Controllo che la lista di variabili sia presente
	; --------------------------------------------------------------
	MOV AX,[Bootes.Script.Scripter.Data.VarsList]
	OR AX,AX
	JZ .Error
	
	; --------------------------------------------------------------
	; Ricerco la variabile nella lista di variabili
	; --------------------------------------------------------------
	MOV DI,AX
	MOV SI,[VarName]
	MOV CX,Bootes.Stage2.Info.MaxVars
	.lSearchVar:
		CMP WORD [CurrentVar.NamePointer],0
		JE .Error
		CALL Bootes.String.Compare, SI,[CurrentVar.NamePointer]
		OR AX,AX
		JZ .GetVarValue
		ADD DI,CurrentVar.Size
		LOOP .lSearchVar
	JMP .Error
	
	; --------------------------------------------------------------
	; Restutuisco il valore
	; --------------------------------------------------------------
	.GetVarValue:
	; Controllo di poter scrivere in RetValue
	MOV SI,[RetValue]
	OR SI,SI
	JZ .Error
	; Recupero il valore
	MOV AX,[CurrentVar.Value]
	MOV WORD [SI],AX
	; Risultato
	MOV AX,[CurrentVar.ValueType]
	
	JMP .Exit
	.Error:
	ErrorSetup VariableNotFound
	MOV AX,Bootes.System.Types.Void
	.Exit:
	POP DI
	POP SI
	POP CX
Return



;====================================================================
; Imposta il valore della variabile indicata. Se la variabile non 
; esiste viene creata.
; --------------------------------------------------------------
; PARAMS
;	VarName		Nome della variabile da impostare.
;	Value		Valore immediato da attribuire alla variabile.
;	ValueType	Tipo della variabile: 1 - Stringa, 2 - Numero. Se si
;				vuole mantenere intatto il tipo del valore corrente
;				immettere 0.
; RETURN
;	Zero in caso di errore altrimenti non zero.
;====================================================================
Function Bootes.Script.Scripter.SetVarValue, VarName, Value ,ValueType
Begin
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	; --------------------------------------------------------------
	; Controllo che la lista di variabili sia presente
	; --------------------------------------------------------------
	MOV AX,[Bootes.Script.Scripter.Data.VarsList]
	OR AX,AX
	JZ .Error
	
	; --------------------------------------------------------------
	; Ricerco la variabile nella lista di variabili
	; --------------------------------------------------------------
	MOV DI,AX
	MOV SI,[VarName]
	MOV CX,Bootes.Stage2.Info.MaxVars
	.lSearchVar:
		CMP WORD [CurrentVar.NamePointer],0
		JE .DeclareVar
		CALL Bootes.String.Compare, SI,[CurrentVar.NamePointer]
		OR AX,AX
		JZ .SetVarValue
		ADD DI,CurrentVar.Size
		LOOP .lSearchVar
	JMP .Error
	
	; --------------------------------------------------------------
	; Dichiaro la variabile
	; --------------------------------------------------------------
	.DeclareVar:
	; Controllo lunghezza nome
	CALL Bootes.String.Length, SI
	CMP AX,Bootes.Script.Scripter.Data.MaxVarNameChars
	JA .Error
	OR AX,AX
	JZ .Error
	; Controllo caratteri non validi
	CALL Bootes.String.IndexOfAny, SI,Bootes.Script.Scripter.Data.InvalidVarChars
	CMP AX,-1
	JNE .Error
	; Imposto il tipo della variabile
	CMP BYTE [ValueType],Bootes.System.Types.String
	JB .Error
	JA .eStringVar
	.iStringVar:
		; Alloco lo stazio per il nome e per il valore stringa
		CALL Bootes.String.Length, SI
		MOV CX,AX
		ADD AX,Bootes.Stage2.Info.MaxStringVarLen
		INC CX
		CALL Bootes.Memory.Alloc, AX
		MOV DX,CX
		OR AX,AX
		JZ .Exit
		; Copio il nome
		PUSH DI
		MOV DI,AX
		REP MOVSB
		POP DI
		; Imposto il riferimento al nome della variabile
		MOV [CurrentVar.NamePointer],AX
		; Imposto il puntatore al valore stringa
		ADD AX,DX
		MOV [CurrentVar.Value],AX
		; Imposto il tipo del valore della variabiles
		MOV WORD [CurrentVar.ValueType],Bootes.System.Types.String
		JMP .xStringVar
	.eStringVar:
		; Copio il nome
		CALL Bootes.String.Copy, SI
		OR AX,AX
		JZ .Error
		; Imposto il riferimento al nome della variabile
		MOV [CurrentVar.NamePointer],AX
		; Imposto il tipo del valore della variabiles
		MOV WORD [CurrentVar.ValueType],Bootes.System.Types.Number
	.xStringVar:
	; Incremento il numero di variabili
	INC WORD [Bootes.Script.Scripter.Data.VarCount]
	
	; --------------------------------------------------------------
	; Imposto il valore
	; --------------------------------------------------------------
	.SetVarValue:
	; Non si può impostare la variabile answare
	CMP DI,[Bootes.Script.Scripter.Data.VarsList]
	JE .Error
	; Controllo che il valore da impostare sia di tipo corretto
	MOV AX,[ValueType]
	AND AX,0xFF
	CMP WORD [CurrentVar.ValueType],Bootes.System.Types.String
	JA .eStringValue
	.iStringValue:
		CMP AX,Bootes.System.Types.String
		JBE .eConvertNum
		.iConvertNum:
			; Converto il numero passato in una stringa e lo salvo nella variabile
			CALL Bootes.String.FromNumber, [CurrentVar.Value],0,[Value],16,16
			MOV SI,AX
			MOV BYTE [SI],0
			MOV AX,[CurrentVar.Value]
			JMP .Exit
		.eConvertNum:
			; Controllo che la stirnga abbia una lunghezza valida
			MOV SI,[Value]
			OR SI,SI
			JZ .Error
			CALL Bootes.String.Length, SI
			MOV CX,AX
			CMP AX,Bootes.Stage2.Info.MaxStringVarLen
			JA .Error
			; Copio la stringa
			INC CX
			MOV AX,[CurrentVar.Value]
			MOV DI,AX
			REP MOVSB
			JMP .Exit
	.eStringValue:
		; Se il valore è una striga ho un errore
		CMP AX,Bootes.System.Types.String
		JE .Error
		; Imposto il valore
		MOV AX,[Value]
		MOV [CurrentVar.Value],AX
		OR AX,AX
		JNZ .Exit
		INC AX
	.xStringValue:
	
	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
Return



;====================================================================
; Elimina la variabile specificata.
; --------------------------------------------------------------
; PARAMS
;	VarName		Nome della variabile da impostare.
;====================================================================
Function Bootes.Script.Scripter.DeleteVar, VarName
Begin
	PUSH AX
	PUSH CX
	PUSH SI
	PUSH DI
	
	; --------------------------------------------------------------
	; Controllo che la lista di variabili sia presente
	; --------------------------------------------------------------
	MOV AX,[Bootes.Script.Scripter.Data.VarsList]
	OR AX,AX
	JZ .Exit
	; Salto answare
	ADD AX,CurrentVar.Size
	
	; --------------------------------------------------------------
	; Ricerco la variabile nella lista di variabili
	; --------------------------------------------------------------
	MOV DI,AX
	MOV SI,[VarName]
	MOV CX,[Bootes.Script.Scripter.Data.VarCount]
	.lSearchVar:
		CMP WORD [CurrentVar.NamePointer],0
		JE .Exit
		CALL Bootes.String.Compare, SI,[CurrentVar.NamePointer]
		OR AX,AX
		JE .DeleteVariable
		ADD DI,CurrentVar.Size
		LOOP .lSearchVar
	JMP .Exit
	
	; --------------------------------------------------------------
	; Rimuovo la variabile
	; --------------------------------------------------------------
	.DeleteVariable:
	; Libero lo spazio occupato dalla variabile
	CALL Bootes.Memory.Free, [CurrentVar.NamePointer]
	; Rimuovo la variabile dall'elenco
	MOV SI,[Bootes.Script.Scripter.Data.VarCount]
	CMP SI,1
	JE .eReplaceVar
	.iReplaceVar:
		; Trovo il puntatore all'ultima variabile
		DEC SI
		MOV CX,SI
		SHL SI,3
		SHL CX,1
		SUB SI,CX
		ADD SI,[Bootes.Script.Scripter.Data.VarsList]
		PUSH SI
		; Copio il contenuto dell'ultima variabile
		MOV CX,3
		REP MOVSW
		; reimposto l'ultima avriabile
		POP DI
	.eReplaceVar:
		MOV WORD [CurrentVar.NamePointer],0
	.xReplaceVar:
	; Diminuisco il numero di variabili
	DEC WORD [Bootes.Script.Scripter.Data.VarCount]
	
	.Exit:
	POP DI
	POP SI
	POP CX
	POP AX
Return



;====================================================================
; Stampa nell'output standard la lista di variabili che iniziano
; con il nome specificato.
; --------------------------------------------------------------
; PARAMS
;	VarPartialName	Puntatore ad una stringa terminata da 0.
;====================================================================
Function Bootes.Script.Scripter.InfoThisVariable, VarPartialName
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	; --------------------------------------------------------------
	; Controllo che la lista di variabili sia presente
	; --------------------------------------------------------------
	MOV AX,[Bootes.Script.Scripter.Data.VarsList]
	OR AX,AX
	JZ .Error
	MOV DI,AX
	MOV CX,[Bootes.Script.Scripter.Data.VarCount]
	OR CX,CX
	JZ .Error
	CALL Bootes.IO.Standard.Output, Bootes.Script.Scripter.Data.InfoVariable.Header

	; --------------------------------------------------------------
	; Ciclo nella lista di variabili
	; --------------------------------------------------------------
	XOR DX,DX
	MOV SI,[VarPartialName]
	CALL Bootes.String.Length, SI
	OR AX,AX
	JNZ .eNullName
		MOV SI,Bootes.String.Null
		MOV BX,1
		JMP .lPrintVar
	.eNullName:
		MOV BX,AX
		INC BX
	.lPrintVar:
		CMP WORD [CurrentVar.NamePointer],0
		JE .xPrintVar
		CALL Bootes.String.Compare, [CurrentVar.NamePointer],SI
		OR AX,AX
		JZ @f
		CMP AX,BX
		JNE .Continue
		@@:
		; Nome
		CALL Bootes.String.Move, [CurrentVar.NamePointer],Bootes.Script.Scripter.Data.InfoVariable.Entry+2
		PUSH BX
		PUSH CX
		MOV CX,Bootes.Script.Scripter.Data.InfoVariable.Entry+23
		MOV BX,AX
		SUB CX,AX
		.Pad:
		MOV BYTE [BX],' '
		INC BX
		LOOP .Pad
		POP CX
		; Tipo e valore
		CMP BYTE [CurrentVar.ValueType],Bootes.System.Types.String
		JA .eString
			MOV BYTE [BX],'S'
			MOV WORD [BX+2],' '
			CALL Bootes.IO.Standard.Output, Bootes.Script.Scripter.Data.InfoVariable.Entry
			MOV BX,[CurrentVar.Value]
			CALL Bootes.IO.Standard.Output, BX
			CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
			JMP .xString
		.eString:
			MOV BYTE [BX],'N'
			MOV BYTE [BX+3],' '
			; E' un numero
			CALL Bootes.String.FromNumber, Bootes.Script.Scripter.Data.InfoVariable.Entry+26,0,[CurrentVar.Value],1,10
			MOV BX,AX
			MOV BYTE [BX],0
			CALL Bootes.IO.Standard.Output, Bootes.Script.Scripter.Data.InfoVariable.Entry
			CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
		.xString:
		POP BX
		INC DX
		.Continue:
		ADD DI,CurrentVar.Size
		LOOP .lPrintVar
	.xPrintVar:
	MOV AX,DX
	OR AX,AX
	JNZ .Exit
	
	.Error:
	ErrorSetup VariableNotFound
	XOR AX,AX
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
align 2
Bootes.Script.Scripter.Data:
	.FunctionParams				DW 0
	.LastResult					DW ?
	.LastResultType				DW 0
	;
	.VarsList					DW 0	; Puntatore alla lista di strutture variabile
	.VarCount					DW 1	; Numero di variabili dichiarate
	.AnswareName				DB "Answare",0
	.InvalidVarChars			DB '"()',0
	.MaxVarNameChars			= 20
	.VarSign					DB '=',0
	;
	.DirectiveEndSign			DB ' ',0
	.FunctionSign				DB '(',0
	.FunctionEndSign			DB ')',0
	.ParamsSplitter				DB ',',0
	.FunctionNoiseChars			DB " ()",0
	;
	.Directives					DB "help",0
								DB "using",0
								DB "valueof",0
								DB "delete",0
								DB 0
	.ValueOf.String				DB "String: ",0
	.ValueOf.Number				DB "Number: 0x0000",13,10,0
	.InfoVariable.Header		DB "Name                   Value",13,10,0
	.InfoVariable.Entry			DB "                       S: 0000000000",0