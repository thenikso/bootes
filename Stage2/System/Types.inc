; --------------------------------------------------------------
; Definizione tipi
; --------------------------------------------------------------
Bootes.System.Types:
	.Void				= 0
	.String				= 1
	.Number				= 2

Bootes.System.Types.Flags:
	.NoError			= 0x0100

Type.Null					EQU Bootes.System.Types.Void
Type.String					EQU Bootes.System.Types.String
Type.Number					EQU Bootes.System.Types.Number
Type.NullNoError			EQU (Bootes.System.Types.Void or Bootes.System.Types.Flags.NoError)
Type.StringNoError			EQU (Bootes.System.Types.String or Bootes.System.Types.Flags.NoError)
Type.NumberNoError			EQU (Bootes.System.Types.Number or Bootes.System.Types.Flags.NoError)