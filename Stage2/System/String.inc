;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.String
;
; Funzioni di supporto per le stringhe.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2


; --------------------------------------------------------------
; Funzioni esportate
; --------------------------------------------------------------
SysNamespace Bootes.String_Namespace, "String"
	Export Bootes.String.Length,1, "Length", Type.NumberNoError, Type.String
	Export Bootes.String.Compare,2, "Compare", Type.NumberNoError, Type.String, Type.String
	Export Bootes.String.Hash,1, "Hash", Type.Number, Type.String
	; TODO: Vedere quali funzioni rendere pubbliche e quali no
	Export Bootes.String.Split,2, "Split"
	Export Bootes.String.SplitRight,2, "SplitRight"
	Export Bootes.String.SplitLeft,2, "SplitLeft"
	Export Bootes.String.Trim,2, "Trim"
	Export Bootes.String.TrimLeft,2, "TrimLeft"
	Export Bootes.String.TrimRight,2, "TrimRight"
	Export Bootes.String.ToHex,1, "ToHex"
	Export Bootes.String.ToBin,1, "ToBin"
	Export Bootes.String.ToDec,1, "ToDec"



;====================================================================
; Inizializza il namespace delle stringhe
;====================================================================
Bootes.String.Init:
	CALL Bootes.Script.Comander.AddNamespace, Bootes.String_Namespace
RET



;====================================================================
; Restituisce la lunghezza della stringa terminata da 0 specificata.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da 0 della quale 
;			calcolare la lunghezza.
; RETURN
;	Lunghezza della stringa specificata.
;====================================================================
Function Bootes.String.Length, String
Begin
;	PUSH SI
;	PUSH CX
;	
;	MOV SI,[String]
;	XOR CX,CX
;	.lLen:
;		LODSB
;		OR AL,AL
;		JZ .xLen
;		INC CX
;		JMP .lLen
;	.xLen:
;	MOV AX,CX
;	
;	POP CX
;	POP SI
	
	CALL Bootes.String.GetEndPoint, [String]
	SUB AX,[String]
Return



;====================================================================
; Ritorna un puntatore posto alla fine della stringa data.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da 0.
;====================================================================
Function Bootes.String.GetEndPoint, String
Begin
	PUSH DI
	PUSH DX
	PUSH BX

	MOV DI,[String]
	@@:
	MOV BX,[DI]
	ADD DI,2
	LEA DX,[BX-0x0101]
	XOR BX,DX
	AND BX,0x8080
	JZ @b
	AND BX,DX
	JZ @b
	BSF BX,BX
	SUB BX,4
	SHR BX,3
	LEA AX,[DI+BX-2]
	
	.Exit:
	POP BX
	POP DX
	POP DI
Return



;====================================================================
; Copia la stringa specificata in una nuova zona di memoria.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da 0 da copiare.
; RETURN
;	Il puntatore alla stringa copiata oppure 0 se non è stato 
;	possibile copiare la stringa.
;====================================================================
Function Bootes.String.Copy, String
Begin
	CALL Bootes.String.Length, [String]
	INC AX
	; Alloco la memoria necessaria
	CALL Bootes.Memory.Alloc, AX
	OR AX,AX
	JZ .Exit
	; Copio la stringa
	PUSH AX
	CALL Bootes.String.Move, [String],AX
	POP AX
	.Exit:
Return



;====================================================================
; Copia la stringa specificata in una nuova zona di memoria.
; --------------------------------------------------------------
; PARAMS
;	String		Puntatore alla stringa terminata da 0 da copiare.
;	Destination	Puntatore alla zona di memoria nella quale copiare.
; RETURN
;	Il puntatore alla FINE della stringa copiata.
;====================================================================
Function Bootes.String.Move, String, Destination
Begin
	PUSH CX
	PUSH SI
	PUSH DI

	MOV SI,[String]
	MOV DI,[Destination]
	CALL Bootes.String.Length, SI
	MOV CX,AX
	INC CX
	REP MOVSB
	MOV AX,DI
	DEC AX
	
	POP DI
	POP SI
	POP CX
Return


;====================================================================
; Appende la seconda stringa alla prima SENZA riallocare lo spazio
; necessario.
; --------------------------------------------------------------
; PARAMS
;	String1	Puntatore alla stringa terminata da 0 alla quale 
;			aggiungere.
;	String2	Puntatore alla stringa terminata da 0 da aggiungere.
;====================================================================
Function Bootes.String.Append, String1,String2
Begin
	PUSH AX
	PUSH CX
	PUSH SI
	PUSH DI
	
	; Recupero la posizione in cui scrivere
	CALL Bootes.String.GetEndPoint, [String1]
	MOV DI,AX
	; Lunghezza aggiunta
	MOV SI,[String2]
	CALL Bootes.String.Length, SI
	INC AX
	MOV CX,AX
	; Aggiungo
	REP MOVSB
	
	POP DI
	POP SI
	POP CX
	POP AX
Return



;====================================================================
; Confronta se le stringhe passate sono uguali.
; --------------------------------------------------------------
; PARAMS
;	String1	Puntatore alla stringa terminata da 0 da confrontare.
;	String2	Puntatore alla stringa terminata da 0 da confrontare.
; RETURN
;	Zero se le due stringhe sono uguali. Valore sorrispondente 
;	all'indice del primo carattere differente; positivo se la 
;	stringa 1 è maggiore della stringa 2 altrimenti negativo.
;====================================================================
Function Bootes.String.Compare, String1,String2
Begin
	PUSH CX
	PUSH SI
	PUSH DI
	; Riferimenti alle stringhe
	MOV SI,[String1]
	MOV DI,[String2]
	CALL Bootes.String.Length,SI
	MOV CX,AX
	CALL Bootes.String.Length,DI
	CMP AX,CX
	JBE @f
		MOV CX,AX
	@@:
	; Inizio il ciclo di confronto
	MOV AX,1
	.lCompare:		
		CMPSB
		JA .Above
		JB .Below
		INC AX
		LOOP .lCompare
	; Ritorno
	.Equal:
	XOR AX,AX
	JMP .Exit
	.Below:
	NEG AX
	.Above:
	.Exit:
	POP DI
	POP SI
	POP CX
Return


;====================================================================
; Restituisce un hash code della stringa passata.
; --------------------------------------------------------------
; PARAMS
;	String	Stringa della quale generare l'hash code.
; RETURN
;	Hash code a 16bit della stringa.
;====================================================================
Function Bootes.String.Hash, String
Begin
	PUSH SI
	PUSH CX
	
	MOV SI,[String]
	CALL Bootes.String.Length, SI
	MOV CX,AX
	;XOR AX,AX
;	CLC
;	.lHash:
;		ADC AL,[SI]
;		INC SI
;		SHL AX,5
;		LOOP .lHash

;	SHR CX,1
;	.lHash:
;		XOR AX,[SI]
;		ADD SI,2
;		SHL AX,5
;		LOOP .lHash

	.lHash:
		XOR AL,[SI]
		INC SI
		SHL AX,5
		LOOP .lHash
	
	POP CX
	POP SI
Return



;====================================================================
; Ritorna la posizione della prima occorrenza del carattere
; specificato.
; --------------------------------------------------------------
; EXAMPLE
;	String = "Name.space.Function()",0
;	Chars = '('
;	Return	AX = 19
;			String = "Name.space.Function()",0
; PARAMS
;	String	Puntatore alla stringa terminata da 0 nella quale 
;			cercare.
;	Chars	Puntatore al carattere da cercare.
; RETURN
;	Indice nella stringa al quale è stato trovato il carattere oppure
;	-1 se non è stato trovato il carattere corrispondente.
;====================================================================
Function Bootes.String.IndexOf, String,Char
Begin
	PUSH BX
	PUSH CX
	PUSH SI
	; --------------------------------------------------------------
	; Inizializzo i registri per effettuare il controllo
	; --------------------------------------------------------------
	; Riferimento alla stringa
	XOR AX,AX
	MOV SI,[String]
	OR SI,SI
	JZ .Error
	CMP BYTE [SI],0
	JE .Error
	CALL Bootes.String.Length, SI
	MOV CX,AX
	; Carattere
	MOV BX,[Char]
	MOV BX,[BX]
	
	; --------------------------------------------------------------
	; Ciclo di controllo
	; --------------------------------------------------------------
	.lCheck:
		LODSB
		CMP AL,BL
		JE .iFinded
		LOOP .lCheck
	JMP .Error
	.iFinded:
	MOV AX,SI
	DEC AX
	SUB AX,[String]
	JMP .Exit
	
	.Error:
	MOV AX,-1
	.Exit:
	POP SI
	POP CX
	POP BX
Return



;====================================================================
; Ritorna la posizione della prima occorrenza di uno dei caratteri 
; specificati.
; --------------------------------------------------------------
; EXAMPLE
;	String = "Name.space.Function()",0
;	Chars = "(.",0
;	Return	AX = 4
;			String = "Name.space.Function()",0
; PARAMS
;	String	Puntatore alla stringa terminata da 0 nella quale 
;			cercare.
;	Chars	Puntatore a una serie di caratteri terminata da 0.
; RETURN
;	Indice nella stringa al quale è stato trovato il carattere oppure
;	-1 se non sono stati trovati caratteri corrispondenti.
;====================================================================
Function Bootes.String.IndexOfAny, String,Chars
Begin
	PUSH CX
	PUSH DX
	PUSH DI
	PUSH SI
	; --------------------------------------------------------------
	; Inizializzo i registri per effettuare il controllo
	; --------------------------------------------------------------
	; Recupero il numero di caratteri
	MOV DX,[Chars]
	CALL Bootes.String.Length, DX
	OR AX,AX
	JZ .Error
	MOV CX,AX
	; Riferimento alla stringa
	MOV SI,[String]
	CMP BYTE [SI],0
	JE .Error
	; --------------------------------------------------------------
	; Ciclo di controllo
	; --------------------------------------------------------------
	PUSH CX
	.lCheck:
		; Riprendo il numero di caratteri
		POP CX
		PUSH CX
		; Prendo il carattere corrente
		MOV AL,[SI]
		; Controllo se sono alla fine della stringa
		OR AL,AL
		JZ .xCheck
		; Lo confronto
		MOV DI,DX
		.lCharCheck:
			CMP AL,[DI]
			JE .iFinded
			INC DI
			LOOP .lCharCheck
		; Continuo
		INC SI
		JMP .lCheck
		; Sostituisco
		.iFinded:
		MOV AX,SI
		SUB AX,[String]
		POP CX
		JMP .Exit
	.xCheck:
	POP CX

	.Error:
	MOV AX,-1
	.Exit:
	POP SI
	POP DI
	POP DX
	POP CX
Return



;====================================================================
; Ritorna la posizione dell'ultima occorrenza nella stringa del 
; carattere specificato.
; --------------------------------------------------------------
; EXAMPLE
;	String = "Name.space.Function()",0
;	Chars = '.'
;	Return	AX = 10
;			String = "Name.space.Function()",0
; PARAMS
;	String	Puntatore alla stringa terminata da 0 nella quale 
;			cercare.
;	Chars	Puntatore a un carattere.
; RETURN
;	Indice nella stringa al quale è stato trovato il carattere oppure
;	-1 se è stato trovato il carattere corrispondente.
;====================================================================
Function Bootes.String.LastIndexOf, String,Char
Begin
	PUSH BX
	PUSH CX
	PUSH SI
	; --------------------------------------------------------------
	; Inizializzo i registri per effettuare il controllo
	; --------------------------------------------------------------
	; Riferimento alla stringa
	MOV SI,[String]
	OR SI,SI
	JZ .Error
	CMP BYTE [SI],0
	JE .Error
	CALL Bootes.String.Length, SI
	MOV CX,AX
	DEC AX
	ADD SI,AX
	; Carattere
	MOV BX,[Char]
	MOV BL,[BX]
	
	; --------------------------------------------------------------
	; Ciclo di controllo
	; --------------------------------------------------------------
	STD
	.lCheck:
		LODSB
		CMP AL,BL
		JE .iFinded
		LOOP .lCheck
	JMP .Error
	.iFinded:
	MOV AX,SI
	INC AX
	SUB AX,[String]
	JMP .Exit
	
	.Error:
	MOV AX,-1
	.Exit:
	CLD
	POP SI
	POP CX
	POP BX
Return


;====================================================================
; Divide la stringa nei punti in cui viene trovato uno dei caratteri
; specificati. La stringa viene modificata sostituendo i caratteri
; indicati con 0.
; --------------------------------------------------------------
; EXAMPLE
;	String = "Name.space.Function((ParamA,ParamB)",0
;	Chars = ".(,)",0
;	Return	AX = 6
;			String = "Name",0,"space",0,"Function",0,0,"ParamA",0,
;					"ParamB",0,0
;	Se due caratteri sono trovati consecutivamente viene inserita una
;	stringa vuota come da esempio.
; PARAMS
;	String	Puntatore alla stringa terminata da 0 da dividere.
;	Chars	Puntatore a una serie di caratteri terminata da 0.
; RETURN
;	Numero di sottostringhe ricavate.
;====================================================================
Function Bootes.String.Split, String,Chars
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH DI
	PUSH SI
	; --------------------------------------------------------------
	; Inizializzo i registri per effettuare il controllo
	; --------------------------------------------------------------
	; Riferimento alla stringa
	XOR AX,AX
	MOV SI,[String]
	CMP BYTE [SI],0
	JE .Exit
	; Recupero il numero di caratteri
	MOV DX,[Chars]
	CALL Bootes.String.Length, DX
	OR AX,AX
	JZ .Exit
	MOV CX,AX
	; Numero di stringhe trovate
	MOV BX,1
	; --------------------------------------------------------------
	; Ciclo di controllo
	; --------------------------------------------------------------
	PUSH CX
	.lCheck:
		; Riprendo il numero di caratteri
		POP CX
		PUSH CX
		; Prendo il carattere corrente
		MOV AL,[SI]
		; Controllo se sono alla fine della stringa
		OR AL,AL
		JZ .xCheck
		; Lo confronto
		MOV DI,DX
		.lCharCheck:
			CMP AL,[DI]
			JE .iFinded
			INC DI
			LOOP .lCharCheck
		; Continuo
		INC SI
		JMP .lCheck
		; Sostituisco
		.iFinded:
		MOV BYTE [SI],0
		INC BX
		INC SI
		JMP .lCheck
	.xCheck:
	POP CX
	; --------------------------------------------------------------
	; Risultato
	; --------------------------------------------------------------
	MOV AX,BX

	.Exit:
	POP SI
	POP DI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Divide la stringa nel punto in cui viene trovato uno dei caratteri
; specificati per la prima volta partendo da destra.
; --------------------------------------------------------------
; EXAMPLE
;	String = "Name.space.Function",0
;	Chars = ".",0
;	Return	AX = 2
;			String = "Name.space",0,"Function",0
;	Se due caratteri sono trovati consecutivamente viene inserita una
;	stringa vuota.
; PARAMS
;	String	Puntatore alla stringa terminata da 0 da dividere.
;	Chars	Puntatore a una serie di caratteri terminata da 0.
; RETURN
;	Numero di sottostringhe ricavate.
;====================================================================
Function Bootes.String.SplitRight, String,Chars
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH DI
	PUSH SI
	; --------------------------------------------------------------
	; Inizializzo i registri per effettuare il controllo
	; --------------------------------------------------------------
	; Recupero il numero di caratteri
	MOV DX,[Chars]
	CALL Bootes.String.Length, DX
	OR AX,AX
	JZ .Exit
	MOV CX,AX
	; Riferimento alla stringa
	MOV SI,[String]
	; Lunghezza stringa
	CALL Bootes.String.Length, SI
	OR AX,AX
	JZ .Exit
	; Salvo il primo carattere della stringa e lo sostituisco con 0
	MOV BL,[SI]
	MOV BYTE [SI],0
	; Mi porto alla fine della stringa
	DEC AX
	ADD SI,AX
	; --------------------------------------------------------------
	; Ciclo di controllo
	; --------------------------------------------------------------
	PUSH CX
	.lCheck:
		; Riprendo il numero di caratteri
		POP CX
		PUSH CX
		; Prendo il carattere corrente
		MOV AL,[SI]
		; Controllo se sono alla fine della stringa
		OR AL,AL
		JZ .xCheck
		; Lo confronto
		MOV DI,DX
		.lCharCheck:
			CMP AL,[DI]
			JE .iFinded
			INC DI
			LOOP .lCharCheck
		; Continuo
		DEC SI
		JMP .lCheck
		; Sostituisco e concludo
		.iFinded:
		MOV BYTE [SI],0
	.xCheck:
	POP CX
	; --------------------------------------------------------------
	; Risultato
	; --------------------------------------------------------------
	OR AL,AL
	JNZ @f
		MOV AX,1
		JMP .Exit
	@@:
	MOV AX,2

	.Exit:
	; Ripristino il primo carattere
	MOV SI,[String]
	MOV [SI],BL
	POP SI
	POP DI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Divide la stringa nel punto in cui viene trovato uno dei caratteri
; specificati per la prima volta partendo da sinistra.
; --------------------------------------------------------------
; EXAMPLE
;	String = "Name.space.Function",0
;	Chars = ".",0
;	Return	AX = 2
;			String = "Name",0,"space.Function",0
;	Se due caratteri sono trovati consecutivamente viene inserita una
;	stringa vuota.
; PARAMS
;	String	Puntatore alla stringa terminata da 0 da dividere.
;	Chars	Puntatore a una serie di caratteri terminata da 0.
; RETURN
;	Numero di sottostringhe ricavate.
;====================================================================
Function Bootes.String.SplitLeft, String,Chars
Begin
	PUSH CX
	PUSH DX
	PUSH DI
	PUSH SI
	; --------------------------------------------------------------
	; Inizializzo i registri per effettuare il controllo
	; --------------------------------------------------------------
	; Recupero il numero di caratteri
	MOV DX,[Chars]
	CALL Bootes.String.Length, DX
	OR AX,AX
	JZ .Exit
	MOV CX,AX
	; Riferimento alla stringa
	MOV SI,[String]
	XOR AX,AX
	CMP AL,[SI]
	JE .Exit
	; --------------------------------------------------------------
	; Ciclo di controllo
	; --------------------------------------------------------------
	PUSH CX
	.lCheck:
		; Riprendo il numero di caratteri
		POP CX
		PUSH CX
		; Prendo il carattere corrente
		MOV AL,[SI]
		; Controllo se sono alla fine della stringa
		OR AL,AL
		JZ .xCheck
		; Lo confronto
		MOV DI,DX
		.lCharCheck:
			CMP AL,[DI]
			JE .iFinded
			INC DI
			LOOP .lCharCheck
		; Continuo
		INC SI
		JMP .lCheck
		; Sostituisco
		.iFinded:
		MOV BYTE [SI],0
		MOV AX,2
		POP CX
		JMP .Exit
	.xCheck:
	POP CX
	MOV AX,1

	.Exit:
	POP SI
	POP DI
	POP DX
	POP CX
Return



;====================================================================
; Ricava il puntatore della stringa successiva a quella data.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore a una stringa terminata da 0.
; RETURN
;	Puntatore al primo carattere della stringa seguente a quella
;	data.
;====================================================================
Function Bootes.String.GetNext, String
Begin
	PUSH BX
	PUSH CX
	MOV BX,[String]
	CALL Bootes.String.Length,BX
	MOV CX,100
	ADD BX,AX
	.lZero:
		INC BX
		CMP BYTE [BX],0
		LOOPE .lZero
	MOV AX,BX
	POP CX
	POP BX
Return


;====================================================================
; Elimina i caratteri specificati dall'inizio e dalla fine della 
; stringa specificata.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore a una stringa terminata da 0. Questa stringa
;			può venir modificata dalla funzione.
;	Chars	Puntatore a una serie di caratteri terminata da 0 che 
;			verranno eliminati dall'inizio e dalla fine della 
;			stringa specificata. Se viene passato 0 verranno 
;			eliminati gli spazi.
; RETURN
;	Puntatore al nuovo inizio della stringa.
;====================================================================
Function Bootes.String.Trim, String,Chars
Begin
	CALL Bootes.String.TrimRight, [String],[Chars]
	CALL Bootes.String.TrimLeft, [String],[Chars]
Return



;====================================================================
; Elimina i caratteri specificati dall'inizio della stringa 
; specificata.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore a una stringa terminata da 0. Questa stringa
;			può venir modificata dalla funzione.
;	Chars	Puntatore a una serie di caratteri terminata da 0 che 
;			verranno eliminati dall'inizio e dalla fine della 
;			stringa specificata. Se viene passato 0 verranno 
;			eliminati gli spazi.
; RETURN
;	Puntatore al nuovo inizio della stringa. Oppure il puntatore 
;	all'inizio della stringa originale se non sono stati trovati
;	caratteri da eliminare.
;====================================================================
Function Bootes.String.TrimLeft, String,Chars
Begin
	PUSH BX
	PUSH SI
	
	MOV SI,[String]
	; Controllo se bisogna eliminare spazi
	MOV BX,[Chars]
	OR BX,BX
	JNZ .eTrimSpace
	.iTrimSpace:
		MOV BL,' '
		.lTrimSpace:
			LODSB
			CMP AL,BL
			JE .lTrimSpace
		DEC SI
		JMP .xTrimSpace
	.eTrimSpace:
		; Recupero il numero dei caratteri
		CALL Bootes.String.Length, BX
		OR AX,AX
		JZ .xTrimSpace
		PUSH CX
		PUSH DX
		MOV CX,AX
		MOV DX,BX
		PUSH CX
		.lTrim:
			POP CX
			PUSH CX
			LODSB
			MOV BX,DX
			.lTrimCheck:
				CMP AL,[BX]
				JE .lTrim
				INC BX
				LOOP .lTrimCheck
		POP CX
		DEC SI
		POP DX
		POP CX
	.xTrimSpace:
	MOV AX,SI
	
	POP SI
	POP BX
Return



;====================================================================
; Elimina i caratteri specificati dalla fine della stringa 
; specificata.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore a una stringa terminata da 0. Questa stringa
;			può venir modificata dalla funzione.
;	Chars	Puntatore a una serie di caratteri terminata da 0 che 
;			verranno eliminati dall'inizio e dalla fine della 
;			stringa specificata. Se viene passato 0 verranno 
;			eliminati gli spazi.
; RETURN
;	Puntatore alla FINE della stringa se sono stati trovati dei 
;	caratteri da eliminare.	Altrimenti 0.
;====================================================================
Function Bootes.String.TrimRight, String,Chars
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	
	MOV SI,[String]
	CALL Bootes.String.GetEndPoint, SI
	; Fine originale della stringa
	MOV CX,AX
	; Lunghezza stringa
	SUB AX,SI
	OR AX,AX
	JZ .Error
	MOV DX,AX
	; Ultimo carattere stringa
	MOV SI,CX
	DEC SI
	; Set direction flag (per procedere al contrario con LODSB)
	STD
	; Controllo se bisogna eliminare spazi
	MOV BX,[Chars]
	OR BX,BX
	JNZ .eTrimSpace
	.iTrimSpace:
		MOV BL,' '
		.lTrimSpace:
			OR DX,DX
			JZ @f
			DEC DX
			LODSB
			CMP AL,BL
			JE .lTrimSpace
		INC SI
		INC SI
		@@:
		MOV BYTE [SI],0
		JMP .xTrimSpace
	.eTrimSpace:
		; Recupero il numero dei caratteri
		CALL Bootes.String.Length, BX
		OR AX,AX
		JZ .xTrimSpace
		PUSH CX
		MOV CX,AX
		PUSH CX
		.lTrim:
			OR DX,DX
			JZ @f
			DEC DX
			;
			POP CX
			PUSH CX
			LODSB
			MOV BX,[Chars]
			.lTrimCheck:
				CMP AL,[BX]
				JE .lTrim
				INC BX
				LOOP .lTrimCheck
		INC SI
		INC SI
		@@:
		POP CX
		MOV BYTE [SI],0
		POP CX
	.xTrimSpace:
	MOV AX,SI
	CMP AX,CX
	JB .Exit
	
	.Error:
	XOR AX,AX
	.Exit:
	CLD
	POP SI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Converte tutti i caratteri alfabetici di una stringa in maiuscolo.
; La stringa passata viene modificata.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da 0 da convertire.
; RETURN
;	Puntatore alla stringa di partenza modificata.
;====================================================================
Function Bootes.String.ToUpperCase, String
Begin
	PUSH SI
	MOV SI,[String]
	.lUpper:
		MOV AL,[SI]
		CMP AL,'z'
		JA .Next
		CMP AL,'a'
		JB .Next
		ADD AL,('A'-'a')
		MOV [SI],AL
		.Next:
		INC SI
		OR AL,AL
		JNZ .lUpper
	MOV AX,[String]
	POP SI
Return



;====================================================================
; Converte una stringa contenente un numero esadecimale nel numero
; che rappresenta.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da 0 da convertire.
; RETURN
;	Numero contenuto nella stringa con la base specificata. Se è
;	avvenuto un errore viene ritornato il puntatore alla stringa.
;====================================================================
Function Bootes.String.ToHex, String
Begin
	PUSH BX
	PUSH SI	
	
	MOV SI,[String]
	XOR BX,BX
	.lConvert:
		; Controllo fine numero
		LODSB
		OR AL,AL
		JZ .xConvert
		; Libero il risultato
		SHL BX,4
		; Converto il valore
		SUB AL,'0'
		CMP AL,9
		JBE @f
			SUB AL,7
			TEST AL,0xF0
			JNZ .Error
		@@:
		; Incremento il risultato
		OR BL,AL
		JMP .lConvert
	.xConvert:
	MOV AX,BX
	
	JMP .Exit
	.Error:
	MOV AX,[String]
	.Exit:
	POP SI
	POP BX
Return



;====================================================================
; Converte una stringa contenente un numero binario nel numero
; che rappresenta.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da 0 da convertire.
; RETURN
;	Numero contenuto nella stringa con la base specificata. Se è
;	avvenuto un errore viene ritornato il puntatore alla stringa.
;====================================================================
Function Bootes.String.ToBin, String
Begin
	PUSH BX
	PUSH SI	
	
	MOV SI,[String]
	XOR BX,BX
	.lConvert:
		; Controllo fine numero
		LODSB
		OR AL,AL
		JZ .xConvert
		; Libero il risultato
		SHL BX,1
		; Converto il valore
		SUB AL,'0'
		CMP AL,1
		JA .Error
		; Incremento il risultato
		OR BL,AL
		JMP .lConvert
	.xConvert:
	MOV AX,BX
	
	JMP .Exit
	.Error:
	MOV AX,[String]
	.Exit:
	POP SI
	POP BX
Return



;====================================================================
; Converte una stringa contenente un numero decimale nel numero
; che rappresenta.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla stringa terminata da 0 da convertire.
; RETURN
;	Numero contenuto nella stringa con la base specificata. Se è
;	avvenuto un errore viene ritornato il puntatore alla stringa.
;====================================================================
Function Bootes.String.ToDec, String
Begin
	PUSH BX
	PUSH CX
	PUSH DX
	PUSH SI
	
	MOV CL,10
	MOV SI,[String]
	MOV BH,0
	XOR AX,AX
	.lConvert:
		; Controllo fine numero
		MOV BL,[SI]
		OR BL,BL
		JZ .xConvert
		INC SI
		; Libero il risultato
		MUL CX
		; Converto il valore
		SUB BL,'0'
		CMP BL,9
		JA .Error
		; Incremento il risultato
		ADD AX,BX
		JMP .lConvert
	.xConvert:
	
	JMP .Exit
	.Error:
	MOV AX,[String]
	.Exit:
	POP SI
	POP DX
	POP CX
	POP BX
Return



;====================================================================
; Converte un numero in una stringa che inizia 
; dall'indirizzo indicato. Non è garantito che la stringa generata 
; terminerà con 0.
; --------------------------------------------------------------
; PARAMS
;	String	Puntatore alla memoria dove verrà salvata la stringa.
;	Num		Word da convertire.
;	Bits	Numero di bit del numero da convertire o numero di cifre
;			minime nel caso decimale.
;	Base	Base numerica della conversione.
; RETURN
;	Puntatore al termine della stringa creata.
;====================================================================
Function Bootes.String.FromNumber, String,NumHi,NumLo,Bits,Base
Begin
	PUSH BX
	PUSH CX
	PUSH SI
	PUSH DI	
	
	; Recupero parametri
	; TODO: migliorare controllo
	MOV CX,[Bits]
	CMP CX,32
	JA .Error
	MOV DI,[String]
	MOV AX,[NumHi]
	SHL EAX,16
	MOV AX,[NumLo]

	; --------------------------------------------------------------
	; Switch base
	; --------------------------------------------------------------
	MOV BX,[Base]
	CMP BX,16
	JE .FromHex
	CMP BX,10
	JE .FromDec
	JMP .Error
	
	; --------------------------------------------------------------
	; Conversioni
	; --------------------------------------------------------------
	.FromBin:
		; TODO
		JMP .Error
	.FromOct:
		; TODO
		JMP .Error
	.FromDec:
		PUSH DX
		XOR EDX,EDX
		MOV CH,0

		.FromDec.1000000000:
		MOV EBX,1000000000
		DIV EBX
		CMP CX,10
		JAE @f
		OR AL,AL
		JZ .FromDec.100000000
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB

		.FromDec.100000000:
		MOV EBX,100000000
		MOV EAX,EDX
		XOR EDX,EDX
		DIV EBX
		CMP CX,9
		JAE @f
		OR AL,AL
		JZ .FromDec.10000000
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB

		.FromDec.10000000:
		MOV EBX,10000000
		MOV EAX,EDX
		XOR EDX,EDX
		DIV EBX
		CMP CX,8
		JAE @f
		OR AL,AL
		JZ .FromDec.1000000
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB


		.FromDec.1000000:
		MOV EBX,1000000
		MOV EAX,EDX
		XOR EDX,EDX
		DIV EBX
		CMP CX,7
		JAE @f
		OR AL,AL
		JZ .FromDec.100000
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB

		.FromDec.100000:
		MOV EBX,100000
		MOV EAX,EDX
		XOR EDX,EDX
		DIV EBX
		CMP CX,6
		JAE @f
		OR AL,AL
		JZ .FromDec.10000
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB


		.FromDec.10000:
		MOV EBX,10000
		MOV EAX,EDX
		XOR EDX,EDX
		DIV EBX
		CMP CX,5
		JAE @f
		OR AL,AL
		JZ .FromDec.1000
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB

		.FromDec.1000:
		MOV EBX,1000
		MOV EAX,EDX
		XOR EDX,EDX
		DIV EBX
		CMP CX,4
		JAE @f
		OR AL,AL
		JZ .FromDec.100
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB

		.FromDec.100:
		MOV EBX,100
		MOV EAX,EDX
		XOR EDX,EDX
		DIV EBX
		CMP CX,3
		JAE @f
		OR AL,AL
		JZ .FromDec.10
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB

		.FromDec.10:
		MOV EBX,10
		MOV EAX,EDX
		XOR EDX,EDX
		DIV EBX
		CMP CX,2
		JAE @f
		OR AL,AL
		JZ .FromDec.1
		@@:
			MOV CH,1
			ADD AL,'0'
			STOSB

		.FromDec.1:
		MOV EAX,EDX
		POP DX
		CMP CX,1
		JAE @f
		OR AL,AL
		JZ .From
		@@:
		ADD AL,'0'
		STOSB
		JMP .From
	.FromHex:
		; Controllo il numero di bits
		TEST CX,11100b
		JZ .Error
		; Preparo il numero
		PUSH CX
		SUB CX,4
		ROR EAX,CL
		; Calcolo i cicli
		POP CX
		SHR CX,2
		.lHexPrint:
			; Trovo il carattere
			MOV BL,AL
			AND AL,0x0F
			ADD AL,'0'
			CMP AL,'9'
			JBE @f
				ADD AL,7
			@@:
			; Scrivo la cifra
			STOSB
			; Prissima cifra
			MOV AL,BL
			ROL EAX,4
			LOOP .lHexPrint
	.From:
		
	; Risultato
	MOV AX,DI
	
	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
	POP DI
	POP SI
	POP CX
	POP BX
Return


; --------------------------------------------------------------
; Stringhe comuni
; --------------------------------------------------------------
Bootes.String:
	.Null			DB 0
	.CrLf:
	.Return			DB 13,10,0
	.Tab			DB "\t",0	; TODO: Mettere il codice carattere e sistemare console di conseguenza
	.Space			DB ' ',0
