;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Devices.Keyboard
;
; Gestisce la tastiera.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2

; --------------------------------------------------------------
; Funzioni esportate
; --------------------------------------------------------------
SysNamespace Bootes.Devices.Keyboard_Namespace, "Devices.Keyboard"
	Export Bootes.Devices.Keyboard.WaitKey,0, "WaitKey", Type.Number



;====================================================================
; Inizializza la tastiera.
;====================================================================
Bootes.Devices.Keyboard.Init:
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Devices.Keyboard_Namespace
RET



;====================================================================
; Aspetta che un carattere venga digitato sulla tastiera e lo 
; restituisce.
; --------------------------------------------------------------
; RETURN
;	Carattere premuto.
;====================================================================
Bootes.Devices.Keyboard.WaitKey:
	; Aspetto un carattere
	.lKey:
		MOV AX,0x0100
		INT 0x16
		JZ .lKey
	; Prendo il carattere
	XOR AX,AX
	INT 0x16
RET



;====================================================================
; Da il primo carattere disponibile nel buffer della tastiera.
; --------------------------------------------------------------
; RETURN
;	Carattere premuto oppure 0 se non c'è alcun carattere.
;====================================================================
Bootes.Devices.Keyboard.GetKey:
	MOV AX,0x0100
	INT 0x16
	JZ .Error
	; Prendo il carattere
	XOR AX,AX
	INT 0x16
	JMP .Exit
	.Error:
	XOR AX,AX
	.Exit:
RET