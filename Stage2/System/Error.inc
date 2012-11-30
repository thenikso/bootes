;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.Error
;
; Gestisce le stringhe di errore specifiche. E' possibile escludere questa funzionalità dalla 
; compilazione agendo sul flag Bootes.Stage2.Info.ErrorSupport.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

; --------------------------------------------------------------
; Stringe di errore
; --------------------------------------------------------------
Bootes.Error.Strings:
	.GeneralCommandError		DB "Error executing command",0
	
	if Bootes.Stage2.Info.ErrorSupport = 1
	
	; Memory
	.AllocFailError				DB "Unable to allocate memory",0
	.OutOfMemoryError			DB "Out of memory",0
	; Comander
	.NamespaceNotFoundError		DB "Namespace not found",0
	.FunctionNotFoundError		DB "Function not found",0
	; Script
	.InvalidParamNumError		DB "Invalid number of parameters",0
	.MissingBracketError		DB "Bracket(s) missing",0
	.BadParameterError			DB "Bad parameter",0
	.NoHelpError				DB "No help informations avaiable",0
	.VariableNotFoundError		DB "Variable not found",0
	.InvalidExpressionError		DB "Invalid expression",0
	.ParseMissingSemicolonError	DB "Missing semicolon ;",0
	.ParseError					DB "One or more error occurred in parsing process",0
	; Disk
	.DiskMountError				DB "Error initializing disk",0
	.UnknowPartitionError		DB "Unknow partition",0
	.NoDiskMountedError			DB "No mounted disks",0
	; File
	.UnableToMountError			DB "Unable to mount partition",0
	.FileNotFoundError			DB "File not found",0
	.DirNotFoundError			DB "Directory not found",0
	.FileReadError				DB "Error reading file",0
	; Boot
	.OSKeyNotFoundError			DB "Invalid OS key",0
	.InvalidBootError			DB "Invalid boot location",0
	
	end if


; --------------------------------------------------------------
; Macro
; --------------------------------------------------------------
MACRO ErrorSetup name,ext
{
	if Bootes.Stage2.Info.ErrorSupport = 1
		if name EQTYPE ""
			LOCAL ..jmperr, ..errstr
			JMP ..jmperr
			..errstr:
			DB name
			..jmperr:
			MOV WORD [Bootes.Error.Data.LastError],..errstr
		else
			if DEFINED Bootes.Error.Strings.#name#Error
				MOV WORD [Bootes.Error.Data.LastError],Bootes.Error.Strings.#name#Error
			else if DEFINED Bootes.Error.Strings.#name
				MOV WORD [Bootes.Error.Data.LastError],Bootes.Error.Strings.#name
			else if DEFINED name
				MOV WORD [Bootes.Error.Data.LastError],name
			else
				Debug.Print.String "WARNING: Error string symbol " # `name # " not defined"
			end if
			if ~ ext EQ 
				MOV WORD [Bootes.Error.Data.LastErrorExtension],ext
			end if
		end if
	end if
}

MACRO ErrorStringPointer reg
{
	if Bootes.Stage2.Info.ErrorSupport = 1
		MOV reg,WORD [Bootes.Error.Data.LastError]
		MOV WORD [Bootes.Error.Data.LastError],Bootes.Error.Strings.GeneralCommandError
	else
		MOV reg,WORD Bootes.Error.Strings.GeneralCommandError
	end if
}

; --------------------------------------------------------------
; Data
; --------------------------------------------------------------
Bootes.Error.Data:
	.LastError					DW Bootes.Error.Strings.GeneralCommandError
	.LastErrorExtension			DW 0