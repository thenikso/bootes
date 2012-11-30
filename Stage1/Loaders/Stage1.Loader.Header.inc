;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Header di un loader.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

org Bootes.Stage1.Info.Origin+512
use16
include "..\..\Shared\DebugMacro.inc"
include "..\..\Shared\Shared.inc"

Bootes.Stage1.Loader.Header:
	.Start:
	
	JMP WORD Bootes.Stage1.Loader.Footer.PrintError


MACRO Stage1LoaderCode
{
	Bootes.Stage1.Loader.Header.Code:
}