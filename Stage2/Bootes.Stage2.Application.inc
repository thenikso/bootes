;====================================================================
; Align
;====================================================================
macro align value
{
   rb (value-1) - ($ + value-1) mod value
}

HelpInfoMagic	EQU 0x8B3C
IsPublicFunction = 0
HaveHelpRemarks = 0
HaveHelpParameters = 0
HaveHelpReturn = 0
HaveHelpSeeAlso = 0

HaveParametersType = 0
HaveParametersTypeCount = 0
HaveReturnType = 0


; --------------------------------------------------------------
; Macro di definizione helper
; --------------------------------------------------------------
MACRO HelpRemarks [text]
{
	COMMON
	if Bootes.Stage2.Info.HelpSupport = 1
	HaveHelpRemarks = $
	FORWARD
		IF ~ text EQ & text EQTYPE ""
			DB text
		END IF
	COMMON
		DB 0
	end if
}

MACRO HelpParams [type,name,desc]
{
	COMMON
	if Bootes.Stage2.Info.HelpSupport = 1
	HaveHelpParameters = $
	FORWARD
			IF name EQTYPE "" & desc EQTYPE ""
				DB name,0,desc,0
			END IF
	COMMON
		DB 0,0
	end if
	
	HaveParametersTypeCount = 0
	HaveParametersType = $
	FORWARD
	IF type EQTYPE 0
		DW type
		HaveParametersTypeCount = HaveParametersTypeCount+1
	END IF
}

MACRO HelpReturn type,[text]
{
	COMMON
	if Bootes.Stage2.Info.HelpSupport = 1
	HaveHelpReturn = $
	FORWARD
		IF ~ text EQ & text EQTYPE ""
			DB text
		END IF
	COMMON
		DB 0
	end if
	
	IF type EQTYPE 0
		HaveReturnType = type
	ELSE 
		HaveReturnType = 0
	END IF
}

MACRO HelpSeeAlso [ref]
{
	COMMON
	if Bootes.Stage2.Info.HelpSupport = 1
	HaveHelpSeeAlso = $
	FORWARD
		IF ref EQTYPE 0 & ~ ref EQ 0
			DW ref
		END IF
	COMMON
		DW 0
	end if
}



;====================================================================
; Calling convention
;====================================================================
MACRO Function name,[arg]			; define procedure
{
COMMON
	IF USED name | IsPublicFunction = 1
		; --------------------------------------------------------------
		; Definizione Bootes.Script.Comander.FunctionLink
		; --------------------------------------------------------------
		IF IsPublicFunction = 1
			align 2
			; Dichiaro l'etichetta che verrà referenziata dal prossimo export
			LABEL next_func
			
			LOCAL ..thisf_fnext
			next_func EQU ..thisf_fnext
			
			; Puntatore al namespace
			DW cur_nspace
			; Puntatore alla prossima funzione
			IF DEFINED ..thisf_fnext
				DW ..thisf_fnext	; Next
			ELSE
				DW 0
			END IF
			; Puntatore alla funzione
			DW name
			; Numero di argomenti
			LOCAL ..argcount
			..argcount = 0
			IF ~ arg EQ 
			FORWARD
				..argcount = ..argcount + 1
			COMMON
			END IF
			DW ..argcount
			; Puntatore all'array dei tipi
			IF HaveParametersType <> 0 & HaveParametersTypeCount <> 0 & ..argcount = HaveParametersTypeCount
				DW HaveParametersType
			ELSE
				DW 0
			END IF
			HaveParametersType = 0
			; Tipo di ritorno
			DW HaveReturnType
			HaveReturnType = 0
			; Puntatore alla struttura di help
			IF DEFINED name#.HelpInfo
				DW name#.HelpInfo
			ELSE
				DW 0
			END IF
			; Nome della funzione in stringa
			DB `name
			DB 0
			
			IsPublicFunction = 0
		END IF
		
		; --------------------------------------------------------------
		; Definizione Bootes.Script.Helper.HelpInfo
		; --------------------------------------------------------------
		IF Bootes.Stage2.Info.HelpSupport = 1 & (HaveHelpParameters | HaveHelpRemarks | HaveHelpSeeAlso | HaveHelpReturn)
		align 2
		name#.HelpInfo:
				DW HelpInfoMagic
				;
				IF HaveHelpRemarks
					DW HaveHelpRemarks
					HaveHelpRemarks = 0
				ELSE
					DW 0
				END IF
				;
				IF HaveHelpParameters
					DW HaveHelpParameters
					HaveHelpParameters = 0
				ELSE
					DW 0
				END IF
				;
				IF HaveHelpReturn
					DW HaveHelpReturn
					HaveHelpReturn = 0
				ELSE
					DW 0
				END IF
				;
				IF HaveHelpSeeAlso
					DW HaveHelpSeeAlso
					HaveHelpSeeAlso = 0
				ELSE
					DW 0
				END IF
		END IF

		; --------------------------------------------------------------
		; Inizio funzione
		; --------------------------------------------------------------
		align 4
		name:		

		; --------------------------------------------------------------
		; Argomenti della funzione
		; --------------------------------------------------------------
		all@args FIX arg
		VIRTUAL AT BP+4
			IF ~ arg EQ
			REVERSE
				first@args FIX arg
			FORWARD
				LOCAL ..arg
				..arg dw ?
				arg EQU ..arg
			COMMON
			END IF
		..ret = $ - (BP+4)
		END VIRTUAL
		
		; --------------------------------------------------------------
		; Variabili interne alla funzione
		; --------------------------------------------------------------
		LOCAL ..data,..size
		if defined ..size
			VIRTUAL AT BP-..size
		else
			if ..ret
				PUSH BP
				MOV BP,SP
			end if
    	end if
		..data:
		
		; --------------------------------------------------------------
		; Macro di entrata ed uscita dalla funzione
		; --------------------------------------------------------------
		MACRO Begin size,level
		\{ 
			IF size EQ & level EQ
				RB (2 - ($-..data) and 1b) and 1b
				IF DEFINED ..size
					..size = $ - ..data
					END VIRTUAL
				ELSE
					..size = $ - ..data
				END IF
				IF ..ret | DEFINED ..size
					PUSH BP
					MOV BP,SP
					IF ..size
						SUB SP,..size
					END IF
				END IF
			ELSE
				enter size,level
			END IF
		\}

		MACRO exit
		\{
			; Ritorno
			IF ..ret | DEFINED ..size
				LEAVE
			END IF
			IF ..ret
				retn ..ret
			ELSE
				retn
			END IF 
		\}
}

macro rstargs [arg]
{ restore arg }
 
macro EndFunction				; end procedure definition
{
		purge exit
		purge Begin
		MACRO rstargs#first@args \{ \}
		rstargs all@args
		purge rstargs#first@args
	end if	; end if used name (Function MACRO)
}
 
macro Return
 { exit
   EndFunction }


macro CALL proc,[arg] 		; invoke procedure (indirect)
 { common
    if ~ arg eq
   reverse
     pushw arg
   common
     call proc
    else
     call proc
    end if }
   


;====================================================================
; Exporting
;====================================================================
MACRO Namespace Nspace,Name
{
	local ..cur_nspace, ..first_func
	cur_nspace equ ..cur_nspace
	next_func equ ..first_func
	
	Nspace:
	cur_nspace:
	DW 0	; Prev
	DW 0	; Next
	if defined ..first_func
		DW ..first_func	; First function
	else
		DW 0
	end if
	DW 0	; Nidification Level
	DB Name,0
}

MACRO SysNamespace Nspace,PartialName
{
	local ..cur_nspace, ..first_func
	cur_nspace equ ..cur_nspace
	next_func equ ..first_func
	
	Nspace:
	cur_nspace:
	DW 0	; Prev
	DW 0	; Next
	if defined ..first_func
		DW ..first_func	; First function
	else
		DW 0
	end if
	DW 0	; Nidification Level
	DB Bootes.SystemName
	DB '.'
	DB PartialName,0
}

MACRO Public
{
	IF DEFINED cur_nspace
		IsPublicFunction = 1
	ELSE
		Debug.Print.String "WARNING: Namespace not defined."
	END IF
}
		
MACRO Export Functi,ParamN,Name,RetTy,[ParTy]
{
	COMMON
	IF DEFINED cur_nspace
		; Dichiaro l'etichetta che verrà referenziata dal prossimo export
		LABEL next_func
		Functi#.FunctionLink:
		
		LOCAL ..thisf_fnext
		next_func EQU ..thisf_fnext
		
		DW cur_nspace	; namespace
		IF DEFINED ..thisf_fnext
			DW ..thisf_fnext	; Next
		ELSE
			DW 0
		END IF
		DW Functi
		DW ParamN
		LOCAL ..party_label
		IF DEFINED ..party_label
			DW ..party_label
		ELSE
			DW 0
		END IF
		IF ~ RetTy EQ & RetTy EQTYPE 0
			DW RetTy
		ELSE
			DW 0
		END IF
		IF DEFINED Functi#.HelpInfo
			DW Functi#.HelpInfo
		ELSE
			DW 0
		END IF
		DB Name,0
		;
		party_count = 0
		FORWARD
			IF ParTy EQTYPE 0
				party_count = party_count+1
			END IF
		COMMON
		IF ParamN = party_count
			..party_label:
			FORWARD
			IF ~ ParTy EQ
				DW ParTy
			END IF
			COMMON
		END IF
	END IF
}


;====================================================================
; Fix
;====================================================================
;_% fix {
;%_ fix }