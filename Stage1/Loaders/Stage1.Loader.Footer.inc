;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Footer of a loader.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

Bootes.Stage1.Loader.Footer:

; --------------------------------------------------------------
; Padding
; --------------------------------------------------------------
.Padding:
times 512-(Bootes.Stage1.Loader.Footer.End-.PrintError)-($-Bootes.Stage1.Loader.Header.Start) DB 0

; --------------------------------------------------------------
; Error message
; --------------------------------------------------------------
.PrintError:
	MOV SI,.ErrorString
	CALL Bootes.Stage1.Loader.Print
	JMP $
	.ErrorString	DB Bootes.SystemName
					DB " Stage1 is needed for boot this partition!",0

Bootes.Stage1.Loader.Print:
	MOV AH,0x0E		; To call the write character function with `int 0x10`
	MOV BX,0x0007	; Set the character background
	.lPrint:
		LODSB
		OR AL,AL
		JZ .xPrint
		INT 0x10
		JMP .lPrint
	.xPrint:
	RET

; --------------------------------------------------------------
; Informations
; --------------------------------------------------------------
Bootes.Stage1.Loader.ReadFunctionHandler	DW ?
Bootes.Stage1.Loader.EntryPoint				DW Bootes.Stage1.Loader.Header.Code

; --------------------------------------------------------------
; Signature
; --------------------------------------------------------------
Bootes.Stage1.Loader.Footer.Signature:
DW Bootes.Stage1.Info.BootSignature

Bootes.Stage1.Loader.Footer.End: