; --------------------------------------------------------------
; Funzioni esportate
; --------------------------------------------------------------
SysNamespace Bootes.Script_Namespace, "Script"
	Export Bootes.Script.Comander.InfoNamespace,0, "InfoNamespace", Type.Null
	Export Bootes.Script.Scripter.InfoThisVariable,1, "InfoVariable", Type.Null, Type.String
	Export Bootes.Script.Comander.InfoThisNamespace,1, "InfoNamespace", Type.Null, Type.String
	Export Bootes.Script.Parser.ParseFile,1, "ParseFile", Type.Null, Type.String
	Export Bootes.Script.Comander.Using,1, "Using", Type.Null, Type.String


;====================================================================
; Inizializza il supporto agli script
;====================================================================
align 4
Bootes.Script.Init:
	CALL Bootes.Script.Comander.AddNamespace, Bootes.Script_Namespace
	CALL Bootes.Script.Scripter.Init
	CALL Bootes.Script.Helper.Init
	RET


include "Comander.inc"
include "Scripter.inc"
include "Parser.inc"
include "Helper.inc"