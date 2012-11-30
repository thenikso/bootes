;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Script.Helper
;
; Gestisce gli help per le funzioni e l'auto completamento delle stesse.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

; --------------------------------------------------------------
; Strutture e virtuals
; --------------------------------------------------------------
STRUC Bootes.Script.Helper.HelpInfo
{
	.Magic				DW HelpInfoMagic; Magic number per riconoscere la struttura
	.RemarksAddr		DW ?			; Puntatore a una serie di dati del tipo Remarks Entry
	.ParamsAddr			DW ?			; Puntatore a una serie di dati del tipo Parameter Entry
	.ReturnAddr			DW ?			; Puntatore a una serie di dati del tipo Return Entry
	.SeeAlsoAddr		DW ?			; Puntatore a una serie di dati del tipo References Entry
	
	; Remarks Entry:	Stringa terminata da 0.
	; Parameter Entry:	Stringa terminata da 0 contenente il nome del parametro e seguita da una seconda 
	;					stringa terminata da 0 contenente la descrizione del parametro 
	;					oppure 0 se non vi è descrizione.
	;					Una DWORD contenente 0 indica il termine delle descrizioni di parametri.
	; Return Entry:		Stringa terminata da 0 contenente la descrizion del ritorno della funzione.
	; References Entry:	DWORD terminate da 0 contenenti l'indirizzo delle strutture Bootes.Script.Comander.FunctionLink
	;					che descrivono funzioni correlate alla seguente.
}

VIRTUAL AT SI
	CurrentHelpInfo		Bootes.Script.Helper.HelpInfo
END VIRTUAL



;====================================================================
; Inizializza le funzioni dell'helper.
;====================================================================
align 4
Bootes.Script.Helper.Init:
	CALL Bootes.Memory.Alloc, Bootes.Stage2.Info.DefaultStringLen
	MOV [Bootes.Script.Helper.Data.Buffer],AX
RET



;====================================================================
; Stampa sull'output corrente l'help relativo alla funzione 
; specificata se possibile oppure l'help generale.
; --------------------------------------------------------------
; PARAMS
;	FLink	Puntatore alla struttura Bootes.Script.Comander.FunctionLink
;			della funzione per la quale mostrare l'help
; RETURN
;	Zero se la funzione non contiene help altrimenti non zero.
;====================================================================
Function Bootes.Script.Helper.Help, FLink
Begin
	if Bootes.Stage2.Info.HelpSupport = 1
	PUSH BX
	PUSH SI
	PUSH DI
	
	XOR AX,AX
	MOV BX,[FLink]
	MOV SI,[FuncLink.HelpInfoAddr]
	; Controllo che ci sia una struttura HelpInfo
	OR SI,SI
	JZ .Exit
	; Controllo che la struttura sia valida
	CMP WORD [CurrentHelpInfo.Magic],HelpInfoMagic
	JNE .Exit
	
	; --------------------------------------------------------------
	; Esempio d'uso
	; --------------------------------------------------------------
	; Scritta "Usage"
	CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.UsageString
	; Namespace
	MOV AX,[FuncLink.Namespace]
	ADD AX,NSpaceLink.NameOffset
	CALL Bootes.IO.Standard.Output, AX
	CALL Bootes.IO.Standard.Output, Bootes.Script.Comander.Data.NameSeparator
	; Nome funzione
	MOV AX,BX
	ADD AX,FuncLink.NameOffset
	CALL Bootes.IO.Standard.Output, AX
	CALL Bootes.IO.Standard.Output, Bootes.Script.Scripter.Data.FunctionSign
	; Eventuali parametri
	MOV DI,[CurrentHelpInfo.ParamsAddr]
	OR DI,DI
	JZ .UsageNoHelpParams
		JMP @f
		.lPrintParams:
			CALL Bootes.IO.Standard.Output, Bootes.Script.Scripter.Data.ParamsSplitter
			@@:
			CALL Bootes.IO.Standard.Output, DI
			CALL Bootes.String.GetEndPoint, DI
			INC AX
			; Salto la descrizione
			CALL Bootes.String.GetEndPoint, AX
			INC AX
			MOV DI,AX
			CMP BYTE [DI],0
			JNE .lPrintParams
	.UsageNoHelpParams:
	CALL Bootes.IO.Standard.Output, Bootes.Script.Scripter.Data.FunctionEndSign
	CALL Bootes.IO.Standard.Output, Bootes.String.CrLf

	; --------------------------------------------------------------
	; Remarks
	; --------------------------------------------------------------
	MOV DI,[CurrentHelpInfo.RemarksAddr]
	OR DI,DI
	JZ @f
		CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.RemarksString
		CALL Bootes.IO.Standard.Output, DI
		CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
	@@:
	
	; --------------------------------------------------------------
	; Descrizione parametri
	; --------------------------------------------------------------
	MOV DI,[CurrentHelpInfo.ParamsAddr]
	OR DI,DI
	JZ .NoHelpParams
		CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.ParametersString
		JMP @f
		.lPrintParamsDesc:
			CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.NewTab
			@@:
			CMP BYTE [DI],0
			JE @f
			CALL Bootes.IO.Standard.Output, DI
			CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.NewTabTab
			CALL Bootes.String.GetEndPoint, DI
			INC AX
			MOV DI,AX
			CMP BYTE [DI],0
			JE .eHaveDesc
			.iHaveDesc:
				CALL Bootes.IO.Standard.Output, DI
				JMP .xHaveDesc
			.eHaveDesc:
				CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.NoDescription
			.xHaveDesc:
			; Prossimo parametro
			CALL Bootes.String.GetEndPoint, DI
			INC AX
			MOV DI,AX
			JMP .lPrintParamsDesc
		@@:
		CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
	.NoHelpParams:
	
	; --------------------------------------------------------------
	; Remarks
	; --------------------------------------------------------------
	MOV DI,[CurrentHelpInfo.ReturnAddr]
	OR DI,DI
	JZ @f
		;ADD DI,2
		CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.ReturnString
		CALL Bootes.IO.Standard.Output, DI
		CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
	@@:
	
	; --------------------------------------------------------------
	; See Also
	; --------------------------------------------------------------
	MOV DI,[CurrentHelpInfo.SeeAlsoAddr]
	OR DI,DI
	JZ @f
		CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.SeeAlsoString
		.lPrintSeeAlso:
			MOV BX,[DI]
			OR BX,BX
			JZ .xPrintSeeAlso
			; Scrivo
			CALL Bootes.IO.Standard.Output, Bootes.Script.Helper.Data.NewTab
			; Namespace
			MOV AX,[FuncLink.Namespace]
			ADD AX,NSpaceLink.NameOffset
			CALL Bootes.IO.Standard.Output, AX
			CALL Bootes.IO.Standard.Output, Bootes.Script.Comander.Data.NameSeparator
			; Nome funzione
			MOV AX,BX
			ADD AX,FuncLink.NameOffset
			CALL Bootes.IO.Standard.Output, AX
			; Prossimo
			ADD DI,2
			JMP .lPrintSeeAlso
		.xPrintSeeAlso:
		CALL Bootes.IO.Standard.Output, Bootes.String.CrLf
	@@:
	
	MOV AX,1
	.Exit:
	POP DI
	POP SI
	POP BX
	else
	ErrorSetup NoHelp
	XOR AX,AX
	end if
Return



;====================================================================
; Completa, se possibile, la stringa specificata con il comando più
; simile.
; Livello 1]
; La funzione calcola il livello di nidificazione del namespace 
; indicato e procede con il controllo di tutti i namespace a tale
; livello completando con il primo corrisopndente a seconda del
; numero di ripetizione.
; Livello 2]
; Si procede a cercare tra le funzioni del namespace già definito a 
; seconda della ripetizione.
; Livello 3]
; Se il comando è una funzione si procede a completare il parametro
; qualora possibile oppure a chiudere la parentesi.
; --------------------------------------------------------------
; PARAMS
;	Comand		Puntatore alla stringa terminata da 0 contenente il 
;				comando da completare.
;	Repetition	Numero della ripetizione del tentativo di
;				completamento. Questo ingresso deve essere diverso da
;				0 solo se la stringa puntata da Command è uguale a
;				quella computata con Repetition=0.
; RETURN
;	Putatore alla stringa terminata da 0 contenente il testo che 
;	completa il comando dato altrimenti zero se la funzione non è 
;	stata in grado di completare la stringa.
;====================================================================
Function Bootes.Script.Helper.Complete, Comand,Repetition
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	PUSH DI

	MOV SI,[Comand]
	OR SI,SI
	JZ .Error
	CMP BYTE [SI],0
	JE .Error
	; --------------------------------------------------------------
	; Cambio rapido livello di ricerca
	; --------------------------------------------------------------
	MOV DX,[Repetition]
	OR DX,DX
	JZ .Level1
		MOV AX,[Bootes.Script.Helper.Data.LastLevel]
		OR AX,AX
		JZ .Level1
		SUB DX,[Bootes.Script.Helper.Data.LastLevSwitchReps]
		JMP AX
	
	; --------------------------------------------------------------
	; Livello 1] Controllo namespace
	; --------------------------------------------------------------
	.Level1:
	; Aggiorno parametri di cambio rapido livello
	MOV WORD [Bootes.Script.Helper.Data.LastLevel],0
	; Prendo il primo NamespaceLink di tale livello
	CALL Bootes.Script.Comander.GetNidificationLevel, SI
	MOV CX,AX
	SHL AX,1
	MOV BX,Bootes.Script.Comander.Data.NidLevelMarkArray-2
	ADD BX,AX
	MOV BX,[BX]
	OR BX,BX
	JZ .Level2
	; Cerco una corrispondena parziale
	CALL Bootes.String.Length, SI
	XCHG CX,AX
	.lPartialNS:
		; Salvo i valori originali
		PUSH CX
		PUSH SI
		; Controllo che il NamespaceLink esista
		OR BX,BX
		JZ .xPartialNS
		; Effettuo il controllo
		LEA DI,[BX+NSpaceLink.NameOffset]
		REPE CMPSB
		JNE @f
			; Trovata una corrispondenza parziale
			OR DX,DX
			JZ .xPartialNS
			DEC DX
		@@:
		; Prossimo NamespaceLink
		MOV BX,[NSpaceLink.Next]
		POP SI
		POP CX
		JMP .lPartialNS
	.xPartialNS:
	POP SI
	POP CX
	; Vedo se ho trovato qualcosa
	OR BX,BX
	JZ .Level2
	; Completo con il namespace trovato
	LEA SI,[BX+NSpaceLink.NameOffset]
	ADD SI,CX
	CALL Bootes.String.Length, SI
	OR AX,AX
	JZ .Error
	MOV CX,AX
	MOV AX,[Bootes.Script.Helper.Data.Buffer]
	; TODO: Contrlli (?)
	MOV DI,AX
	REP MOVSB
	; Termino la stringa
	MOV BYTE [DI],0
	JMP .Exit
	
	; --------------------------------------------------------------
	; Livello 2] Controllo funzioni
	; --------------------------------------------------------------
	.Level2:
	; Aggiorno parametri di cambio rapido livello
	MOV WORD [Bootes.Script.Helper.Data.LastLevel],.Level2Resume
	MOV AX,[Repetition]
	SUB AX,DX
	MOV WORD [Bootes.Script.Helper.Data.LastLevSwitchReps],AX
	.Level2Resume:
	; Prendo il namespace
	CALL Bootes.String.SplitRight, SI,Bootes.Script.Comander.Data.NameSeparator
	CMP AX,1
	JE .Level3
	JB .Error
	CALL Bootes.Script.Comander.GetNamespaceLink, SI
	MOV BX,AX
	; Recupero nome parziale funzione
	CALL Bootes.String.Length, SI
	MOV DI,SI
	ADD DI,AX
	; Ripristino il .
	MOV AL,[Bootes.Script.Comander.Data.NameSeparator]
	STOSB
	; Recupero la prima funzione del namespacce
	OR BX,BX
	JZ .Level3
	MOV BX,[NSpaceLink.FirstFunction]
	OR BX,BX
	JZ .Level3
	; Recupero informazioni sul nome parziale
	CALL Bootes.String.Length, DI
	MOV CX,AX
	OR AX,AX
	JNZ .lPartialF
	; Ciclo funzioni
	.lAllFunctions:
		; Controllo ripetizione
		OR DX,DX
		JZ .CompleteF
		DEC DX
		; Prossima funzione
		MOV BX,[FuncLink.Next]
		OR BX,BX
		JZ .Level3
		JMP .lAllFunctions
	; Ricerca funzione parziale
	.lPartialF:
		; Salvo i valori originali
		PUSH DI
		PUSH CX
		; Controllo che la funzione esista
		OR BX,BX
		JZ .xPartialF
		; Effettuo il controllo
		LEA SI,[BX+FuncLink.NameOffset]
		REPE CMPSB
		JNE @f
			; Trovata corrispondenza parziale
			OR DX,DX
			JZ .xPartialF
			DEC DX
		@@:
		; Prossima FunctionLink
		MOV BX,[FuncLink.Next]
		POP CX
		POP DI
		JMP .lPartialF
	.xPartialF:
	POP CX
	POP DI
	; Vedo se ho trovato qualcosa
	MOV SI,[Comand]
	OR BX,BX
	JZ .Level3
	; Completo con la funzione trovata
	.CompleteF:
	LEA SI,[BX+FuncLink.NameOffset]
	ADD SI,CX
	CALL Bootes.String.Length, SI
	MOV CX,AX
	MOV AX,[Bootes.Script.Helper.Data.Buffer]
	; TODO: Contrlli (?)
	MOV DI,AX
	REP MOVSB
	; Aggiungo parentesi
	MOV BYTE [DI],'('
	INC DI
	CMP WORD [FuncLink.ParamsCount],0
	JNE @f
		; Chiudo la parentesi (non ci sono parametri da aggiungere)
		MOV BYTE [DI],')'
		INC DI
	@@:
	MOV BYTE [DI],0
	; Ritorno
	JMP .Exit

	; --------------------------------------------------------------
	; Livello 3] Alrti controlli
	; --------------------------------------------------------------
	.Level3:
	; Aggiorno parametri di cambio rapido livello
	MOV WORD [Bootes.Script.Helper.Data.LastLevel],.Level3Resume
	MOV AX,[Repetition]
	SUB AX,DX
	MOV WORD [Bootes.Script.Helper.Data.LastLevSwitchReps],AX
	.Level3Resume:



	.Error:
	MOV AX,0
	.Exit:
	POP DI
	POP SI
	POP DX
	POP CX
	POP BX
Return



Function Bootes.Script.Helper.TypeCheck, Flink,Params
Begin
	; TODO: vedi struttura parametri in Bootes.Script.Comander
Return

Bootes.Script.Helper.Data:
	.Buffer					DW 0	; Buffer per salvare i completamenti
	.LastLevel				DW 0	; Livello di ricerca già raggiunto con Repetition>0
	.LastLevSwitchReps		DW 0	; Numero di ripetizioni effettuate durante l'ultimo cambio di livello

	.UsageString			DB "Usage:\n\t",0
	.RemarksString			DB "\nRemarks:\n\t",0
	.ParametersString		DB "\nParameters:\n\t",0
	.NewTab					DB "\n\t",0
	.NewTabTab				DB "\n\t\t",0
	.NoDescription			DB "No description avaiable.",0
	.ReturnString			DB "Return:\n\t",0
	.SeeAlsoString			DB "\nSeeAlso:",0