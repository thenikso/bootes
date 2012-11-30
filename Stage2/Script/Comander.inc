;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Script.Comander
;
; Gestisce le chiamate di funzioni effettuate tramite stringhe.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2


; --------------------------------------------------------------
; Strutture
; --------------------------------------------------------------
STRUC Bootes.Script.Comander.NamespaceLink
{
	.Start:
	.Previous			DW ?	; Namespace precedente
	.Next				DW ?	; Namespace successivo
	.FirstFunction		DW ?	; Prima funzione del namespace
	.NidificationLevel	DW ?	; Livello di nidificazione del namespece
	.NameOffset			= $-.Start
	.Name				DB ?	; Nome del namespace terminato da 0
}

STRUC Bootes.Script.Comander.FunctionLink
{
	.Start:
	.Namespace			DW ?	; Puntatore ad un NamespaceLink
	.Next				DW ?	; Puntatore al prossimo FunctionLink
	
	.Addr				DW ?	; Puntatore alla funzione
	.ParamsCount		DW ?	; Numero di parametri per la funzione
	.ParamsTypeArray	DW ?	; Puntatore all'array contenente i tipi dei parametri
	.ReturnType			DW ?	; Tipo del dato di ritorno
	.HelpInfoAddr		DW ?	; Puntatore a una struttura di informazioni
	.NameOffset			= $-.Start
	.Name				DB ?	; Stringa terminata da 0 con il nome della funzione
}

STRUC Bootes.Script.Comander.FunctionParams
{
	.Start:
	.Count				DW ?	; Numero di parametri
	.ParamsOffset		= $-.Start
	.Param				DW ?	; Parametri
	.ParamType			DW ?
}

VIRTUAL AT BX
	NSpaceLink			Bootes.Script.Comander.NamespaceLink
END VIRTUAL

VIRTUAL AT BX
	FuncLink			Bootes.Script.Comander.FunctionLink
END VIRTUAL

VIRTUAL AT DI
	FuncParams			Bootes.Script.Comander.FunctionParams
END VIRTUAL




;====================================================================
; Aggiunge un namespace e le sue funzioni all'elenco dei comandi
; eseguibili.
; --------------------------------------------------------------
; PARAMS
;	NLink	Puntatore a una struttura NamespaceLink.
;====================================================================
Function Bootes.Script.Comander.AddNamespace, NLink
Begin
	PUSH AX
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI
	
	; --------------------------------------------------------------
	; Verifico la validità del nuovo namespace
	; --------------------------------------------------------------
	MOV BX,[NLink]
	OR BX,BX
	JZ .Error
	; Se il nidification level è già specificato il namespace non viene aggiunto
	MOV AX,[NSpaceLink.NidificationLevel]
	OR AX,AX
	JNZ .Error
	; Controllo se vi è già un namespace uguale
	LEA SI,[BX+NSpaceLink.NameOffset]
;	CALL Bootes.Script.Comander.GetNamespaceLink, SI
;	OR AX,AX
;	JZ @f
;		; TODO: Aggiungere le funzioni del namespece al NamespeceLink trovato
;		JMP .Error
;	@@:
	
	; --------------------------------------------------------------
	; Calcolo il nidification level del namespace
	; --------------------------------------------------------------
	CALL Bootes.Script.Comander.GetNidificationLevel, SI
	OR AX,AX
	JZ .Error
	MOV [NSpaceLink.NidificationLevel],AX

	; --------------------------------------------------------------
	; Inserisco
	; --------------------------------------------------------------
	; Recupero il primo NamespaceLink allo stesso livello
	MOV CX,AX
	SHL CX,1
	MOV DI,Bootes.Script.Comander.Data.NidLevelMarkArray-2
	ADD DI,CX
	MOV SI,[DI]
	OR SI,SI
	JNZ WORD .eFirstOfLevel
	.iFirstOfLevel:
		; Il NamespaceLink specificato è il primo del suo livello
		MOV [DI],BX
		; Ricerco il NamespaceLink di un livello più basso
		DEC AX
		.lFindPrevious:
			; Controllo fine livelli
			OR AX,AX
			JZ .xFindPrevious
			; Controllo presenza NamespaceLink precedente
			SUB DI,2
			MOV SI,[DI]
			OR SI,SI
			JNZ .xFindPrevious
			; Livello precedente
			DEC AX
			JMP .lFindPrevious
		.xFindPrevious:
		; Aggiorno il NamespaceLink precedente
		OR SI,SI
		JNZ .eFirst
		.iFirst:
			; Non ho nessuno prima
			MOV WORD [NSpaceLink.Previous],0
			; Imposto il NamespaceLink successivo
			MOV SI,[Bootes.Script.Comander.Data.FirstNamespace]
			MOV [NSpaceLink.Next],SI
			; Sostituisco il primo NamespaceLink con quello da aggiungere
			MOV [Bootes.Script.Comander.Data.FirstNamespace],BX
			; Aggiorno se necessario il NamespaceLink successivo
			OR SI,SI
			JZ @f
				XCHG SI,BX
				MOV [NSpaceLink.Previous],SI
			@@:
			; Aggiunto con successo
			JMP .Exit
		.eFirst:
			XCHG SI,BX
			; Trovo l'ultimo NamespaceLink appartenente allo stesso livello del NamespaceLink individuato da BX
			MOV CX,[NSpaceLink.NidificationLevel]
			MOV AX,BX
			.lFindLastOfLevel:
				; Controllo esistenza prossimo NamespaceLink
				MOV BX,[NSpaceLink.Next]
				OR BX,BX
				JZ .xFindLastOfLevel
				; Controllo appartenenza a livello richiesto
				CMP CX,[NSpaceLink.NidificationLevel]
				JNE .xFindLastOfLevel
				; Aggiorno il risultato
				MOV AX,BX
				JMP .lFindLastOfLevel
			.xFindLastOfLevel:
			; Imposto il NamespaceLink precedente a quello da aggiungere
			MOV BX,SI
			MOV [NSpaceLink.Previous],AX
			; Aggiorno il NamespaceLink precedente
			XCHG BX,AX
			MOV SI,[NSpaceLink.Next]
			MOV [NSpaceLink.Next],AX
			; Infine imposto il NamespaceLink successivo a quello da aggiungere
			MOV BX,AX
			MOV [NSpaceLink.Next],SI
		.xFirst:
		
		JMP .xFirstOfLevel
	.eFirstOfLevel:
		; Imposto il NamespaceLink da aggiungere come primo del suo livello
		MOV [DI],BX
		; Imposto il successivo
		MOV [NSpaceLink.Next],SI
		; Aggiorno il successivo
		XCHG SI,BX
		MOV AX,[NSpaceLink.Previous]
		MOV [NSpaceLink.Previous],SI
		; Imposto il precedente
		MOV BX,SI
		MOV [NSpaceLink.Previous],AX
		; Aggiorno il precedente
		OR AX,AX
		JNZ .eWasFirst
		.iWasFirst:
			; Il NamespaceLink era il primo namespace della lista
			MOV [Bootes.Script.Comander.Data.FirstNamespace],BX
			JMP .xWasFirst
		.eWasFirst:
			; C'erano altri NamesaceLink prima di quello al posto del quale ho inserito
			XCHG BX,AX
			MOV [NSpaceLink.Next],AX
		.xWasFirst:
	.xFirstOfLevel:
	
	.Error:
	.Exit:
	POP DI
	POP SI
	POP CX
	POP BX
	POP AX
Return



;====================================================================
; Restituisce il livello di nidificazione del comando specificato.
; --------------------------------------------------------------
; PARAMS
;	Comand		Putatore a una stringa terminata da 0 contenente il 
;				comando del quale calcolare il livello di 
;				nidificazione.
; RETURN
;	Livello di nidificazione del comando (minimo 1) oppure zero se
;	è avvenuto un errore.
;====================================================================
Function Bootes.Script.Comander.GetNidificationLevel, Comand
Begin
	PUSH BX
	PUSH CX
	
	XOR CX,CX
	MOV BX,[Comand]
	OR BX,BX
	JZ .Exit
	.lNidLevel:
		CALL Bootes.String.IndexOf, BX,Bootes.Script.Comander.Data.NameSeparator
		CMP AX,-1
		JE .xNidLevel
		INC CX
		INC AX
		ADD BX,AX
		JMP .lNidLevel
	.xNidLevel:
	INC CX
	.Exit:
	MOV AX,CX
	POP CX
	POP BX
Return



;====================================================================
; Restituisce, se possibile, il riferimento alla struttura 
; Bootes.Script.Comander.NamespaceLink che descrive il namespece
; specificato.
; --------------------------------------------------------------
; PARAMS
;	NSString	Puntatore a una stringa terminata da 0 che indica il 
;				namespace da usare.
; RETURN
;	Puntatore a una struttura Bootes.Script.Comander.NamespaceLink
;	oppure zero se non è stata trovata la struttura corrispondente.
;====================================================================
Function Bootes.Script.Comander.GetNamespaceLink, NSString
Begin
	PUSH BX
	PUSH CX
	PUSH SI

	; --------------------------------------------------------------
	; Ricerco il primo NamespaceLink con livello appropriato
	; --------------------------------------------------------------
	MOV SI,[NSString]
	; Calcolo il livello di nidificazione
	CALL Bootes.Script.Comander.GetNidificationLevel, SI
	OR AX,AX
	JZ .InternalError
	MOV CX,AX
	; Prendo il primo NamespaceLink con tale livello
	SHL AX,1
	MOV BX,Bootes.Script.Comander.Data.NidLevelMarkArray-2
	ADD BX,AX
	MOV BX,[BX]
	OR BX,BX
	JZ .InternalError
	
	; --------------------------------------------------------------
	; Ricerco all'interno del livello
	; --------------------------------------------------------------
	.lFindName:
		; Controllo fine lista
		OR BX,BX
		JZ .InternalError
		; Controllo fine livello di nidificazione
		CMP [NSpaceLink.NidificationLevel],CX
		JNE .InternalError
		; Confronto il nome per intero
		LEA AX,[BX+NSpaceLink.NameOffset]
		CALL Bootes.String.Compare, AX,SI
		OR AX,AX
		JZ .Exit
		; Prossimo nome
		MOV BX,[NSpaceLink.Next]
		JMP .lFindName
	
	.InternalError:
	ErrorSetup NamespaceNotFound
	XOR BX,BX
	.Exit:
	MOV AX,BX
	POP SI
	POP CX
	POP BX
Return



;====================================================================
; Aggiunge un namespace alla lista degli using in modo da poter
; usare le sue funzioni senza bisogno di scrivere il namespace.
; --------------------------------------------------------------
; PARAMS
;	NSString	Puntatore a una stringa terminata da 0 che indica il 
;				namespace da usare.
; RETURN
;	Zero se non è stato possibile aggiungere un namespace, 
;	altrimenti non-zero.
;====================================================================
Function Bootes.Script.Comander.Using, NSString
Begin
	PUSH BX
	PUSH SI

	; --------------------------------------------------------------
	; Verifico se l'inserimento è possibile
	; --------------------------------------------------------------
	; Verifico se c'è spazio
	MOV BX,Bootes.Script.Comander.Data.UsingNamespaces
	ADD BX,(Bootes.Stage2.Info.MaxUsing-1)*2
	MOV AX,[BX]
	OR AX,AX
	JNZ .Error
	; Verifico se è già inserito
	MOV AX,[NSString]
	OR AX,AX
	JZ .Error
	MOV SI,Bootes.Script.Comander.Data.UsingNamespaces
	.lAlreadyUsing:
		MOV BX,[SI]
		OR BX,BX
		JZ .xAlreadyUsing
		; Confronto i nomi
		ADD BX,NSpaceLink.NameOffset
		PUSH AX
		CALL Bootes.String.Compare, BX,AX
		OR AX,AX
		POP AX
		JZ .Exit
		ADD SI,2
		JMP .lAlreadyUsing
	.xAlreadyUsing:
	
	; --------------------------------------------------------------
	; Cerco il namespace tra quelli usati
	; --------------------------------------------------------------
	MOV SI,[NSString]
	CALL Bootes.Script.Comander.GetNamespaceLink, SI
	OR AX,AX
	JZ .Error
	MOV BX,AX

	; --------------------------------------------------------------
	; Aggiungo agli using
	; --------------------------------------------------------------
	MOV SI,Bootes.Script.Comander.Data.UsingNamespaces
	; Cerco il primo using libero
	.lFindUsing:
		LODSW
		OR AX,AX
		JZ .xFindUsing
		JMP .lFindUsing
	.xFindUsing:
	; Aggiungo
	SUB SI,2
	MOV [SI],BX
	; Valore di ritorno
	INC AX
	
	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	POP SI
	POP BX
Return



;====================================================================
; Aggiunge la funzione specificata all'elendo dei comandi 
; disponibili. Se la funzione non punta un NamespaceLink non può
; essere aggiunta.
; --------------------------------------------------------------
; PARAMS
;	FLink	Puntatore a una struttura FunctionLink.
; RETURN
;	Zero se è avvenuto un errore, altrimenti non-zero.
;====================================================================
Function Bootes.Script.Comander.AddFunction, FLink
Begin
Return
	


;====================================================================
; Ritorna, se possibile, il puntatore al FuncitonLink che descrive
; la funzione indicata.
; --------------------------------------------------------------
; PARAMS
;	Name	Puntatore a una stringa terminata da 0 contenente il nome
;			della funzione da cercare. Questa stringa potrebbe essere
;			modificata dalla funzione.
;	ParamsCount	Numero di parametri della funzione specificata. Se 
;			viene specificato -1 verrà ritornata la prima funzione
;			trovata con il nome specificato.
; RETURN
;	Il puntatore a una struttura FunctionLink oppure 0 se è avvenuto 
;	un errore.
;====================================================================
Function Bootes.Script.Comander.GetFunction, Name,ParamsCount
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI
	
	; --------------------------------------------------------------
	; Divido il comando dal namespace
	; --------------------------------------------------------------
	MOV SI,[Name]
	CALL Bootes.String.SplitRight, SI,Bootes.Script.Comander.Data.NameSeparator
	OR AX,AX
	JZ .Error

	; --------------------------------------------------------------
	; Cerco il namespace
	; --------------------------------------------------------------
	CMP AX,1
	JE .eNamespace
	.iNamespace:
	; Cerco il namespace
		CALL Bootes.Script.Comander.GetNamespaceLink, SI
		OR AX,AX
		JZ .Error
		MOV BX,AX
		; Ripristino il .
		CALL Bootes.String.Length, SI
		ADD SI,AX
		MOV AL,[Bootes.Script.Comander.Data.NameSeparator]
		MOV [SI],AL
		INC SI
		
		JMP .xNamespace
	.eNamespace:
	; Cercerò negli using
		MOV DX,Bootes.Script.Comander.Data.UsingNamespaces
		MOV BX,DX
		MOV BX,[BX]
		OR BX,BX
		JZ .Error
	.xNamespace:

	; --------------------------------------------------------------
	; Cerco la funzione
	; --------------------------------------------------------------
	; Recupero il primo puntatore alle funzione
	MOV BX,[NSpaceLink.FirstFunction]
	.lFindFunc:
		; Controllo che il FunctionLink esista
		OR BX,BX
		JNZ @f
		.TryNextUsing:
			; Utilizzare gli using?
			OR DX,DX
			JZ .Error
			; Prossimo namespace
			ADD DX,2
			MOV BX,DX
			MOV BX,[BX]
			OR BX,BX
			JZ .Error
			; Prima funzione
			MOV BX,[NSpaceLink.FirstFunction]
			JMP .lFindFunc
		@@:
		; Controllo il nome
		LEA AX,[BX+FuncLink.NameOffset]
		CALL Bootes.String.Compare, AX,SI
		OR AX,AX
		JZ .xFindFunc
		; Prossima funzione
		MOV BX,[FuncLink.Next]
		JMP .lFindFunc
	.xFindFunc:
	; --------------------------------------------------------------
	; Controllo il numero di parametri
	; --------------------------------------------------------------
	MOV AX,[ParamsCount]
	CMP AX,-1
	JE .Done
	CMP AX,[FuncLink.ParamsCount]
	JE .Done
		; Provo a cercare un overload della funzione
		MOV BX,[FuncLink.Next]
		JMP .lFindFunc
	.Done:
	MOV AX,BX
	
	; Uscita
	JMP .Exit
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
; Esegue, se possibile, la funzione specificata.
; --------------------------------------------------------------
; PARAMS
;	RetVal	Puntatore a una zona di memoria dove salvare il risultato
;			della funzione chiamata.
;	CmdStr	Puntatore alla stringa che contiene il nome della 
;			funzione da	eseguire terminato da 0.
;	Params	Puntatore a una struttura FunctionParams che contiene
;			i parametri da passare alla funzione.
; RETURN
;	-1 se non è stato possibile eseguire il comando, altrimenti
;	viene ritornato il risultato della funzione chiamata duplicato
;	all'indirizzo RetVal.
;====================================================================
Function Bootes.Script.Comander.Run, RetVal,FLink,Params,RetValType
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI

	; --------------------------------------------------------------
	; Riferimento al FunctionLink
	; --------------------------------------------------------------
	MOV BX,[FLink]
	OR BX,BX
	JZ .Error
	; --------------------------------------------------------------
	; Verifico la validità della funzione
	; --------------------------------------------------------------
	MOV DI,[Params]
	MOV AX,[FuncLink.ParamsCount]
	OR AX,AX
	JNZ .eNoParams
	.iNoParams:
		; Non ci sono parametri, verifico se non ne sono stati passati
		XOR SI,SI
		CMP AX,DI
		JZ .xNoParams
	.eNoParams:
		; Controllo che il numero dei parametri passati sia corretto
		CMP AX,[FuncParams.Count]
		JNE .Error
		MOV SI,[FuncLink.ParamsTypeArray]
		OR SI,SI
		JZ .xNoParams
		MOV CX,AX
		SHL CX,1
		SUB CX,2
		ADD SI,CX
	.xNoParams:
	; --------------------------------------------------------------
	; Push dei parametri per la funzione
	;	AX = Numero di parametri per la funzione
	;	BX = FunctronLink che descrive la funzione
	;	DI = FunctionParams che contiene i parametri per la funione
	; --------------------------------------------------------------
	OR AX,AX
	JZ .xPushParams
	MOV CX,AX
	; Mi porto all'ultimo parametro
	SHL AX,2
	ADD DI,AX	; Non ho aggiunto i primi 2 byte del campo count
	XOR DX,DX
	; Push dei parametri
	.lPushParams:
		; Controllo tipo
		OR SI,SI
		JZ @f
			MOV AL,[DI]
			CMP AL,[SI]
			JNE .ErrorPop
			SUB SI,2
		@@:
		SUB DI,2
		; Push
		PUSH WORD [DI]
		INC DX
		SUB DI,2
		LOOP .lPushParams
	.xPushParams:
	; --------------------------------------------------------------
	; Chiamo la funzione
	; --------------------------------------------------------------
	MOV AX,[FuncLink.Addr]
	OR AX,AX
	JZ .Error	; TODO: Togliere la funzione (ed eventualmente il namespace)
	CALL AX
	; --------------------------------------------------------------
	; Imposto il valore di ritorno
	; --------------------------------------------------------------
	MOV DI,[RetVal]
	OR DI,DI
	JZ .xSetReturn
		MOV [DI],AX
	.xSetReturn:
	MOV DI,[RetValType]
	OR DI,DI
	JZ .xSetReturnType
		MOV DX,[FuncLink.ReturnType]
		MOV [DI],DX
	.xSetReturnType:
	
	; Uscita
	CMP AX,-1
	JNE .Exit
	SUB AX,2
	JMP .Exit
	.ErrorPop:
	OR DX,DX
	JZ .Error
	MOV CX,DX
	@@:POP AX
	LOOP @b
	.Error:
	MOV AX,-1
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Stampa a video i namespace registrati
;====================================================================
align 4
Bootes.Script.Comander.InfoNamespace:
	PUSH AX
	PUSH BX
	
	MOV BX,[Bootes.Script.Comander.Data.FirstNamespace]
	.lPrintNS:
		; Controllo fine ciclo
		OR BX,BX
		JZ .xPrintNS
		; Stampo namespace
		LEA AX,[BX+NSpaceLink.NameOffset]
		CALL Bootes.IO.Standard.Output, AX
		CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
		; Prossimo Namespace
		MOV BX,[NSpaceLink.Next]
		JMP .lPrintNS
	.xPrintNS:
	
	POP BX
	POP AX
	RET

Function Bootes.Script.Comander.InfoThisNamespace, NSString
Begin
	PUSH BX
	; Prendo il namespace
	MOV AX,[NSString]
	CALL Bootes.String.Trim, AX,0
	CALL Bootes.Script.Comander.GetNamespaceLink, AX
	OR AX,AX
	JZ .Exit
	MOV BX,AX
	; Titolo
	ADD AX,NSpaceLink.NameOffset
	CALL Bootes.IO.Standard.Output, AX
	CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
	; Elenco funzioni
	MOV BX,[NSpaceLink.FirstFunction]
	.lPrintFuncInfo:
		OR BX,BX
		JZ .Exit
		LEA AX,[BX+FuncLink.NameOffset]
		CALL Bootes.IO.Standard.Output, Bootes.String.Tab
		CALL Bootes.IO.Standard.Output, AX
		CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
		MOV BX,[FuncLink.Next]
		JMP .lPrintFuncInfo
		
	.Exit:
	POP BX
Return



;====================================================================
; Data
;====================================================================
Bootes.Script.Comander.Data:
	.FirstNamespace				DW 0							; Primo namespace
	.NidLevelMarkArray			RW 10							; Array contenente i riferimenti ai primi namespace con un differente nid. level
	.UsingNamespaces			RW Bootes.Stage2.Info.MaxUsing	; Namespace da usare
	;
	.NameSeparator				DB '.',0