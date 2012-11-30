Bootes.SystemName   equ "iOS"

Bootes.Stage1.Info:
  .Origin             = 0x7C00
  .BPB.Start          = 0x03
  .BPB.End            = 0x5A
  .UsePartitionTable  = 0
  .BootSignature      = 0xAA55
  ; Loaders
  .Loader.Use         = 0
  .Loader.Sector      = 1       ; 0 is the first sector of the active partition.
Bootes.Stage1.Info.FileSystem   equ FAT12
  
Bootes.Stage2.Info:
  ; General
  .Origin           = 0x500
  .64kPadding       = 0
  .StartSector      = 1 ;33 ; Only used if Loader.Use = 0
  .SectorsCount     = 0x2D
  .DefaultVideoMode = 3     ; 0 for predefined mode, 3 for textual mode
  ; Optional support
  .ErrorSupport     = 1
  .HelpSupport      = 1
  .MultitaskSupport = 1
  .GraphicSupport   = 1
  ; Memory
  .MaxMemoryBlocks  = .MaxTasks*3+100
  .PModeStack       = 0xFFFF
  .LModeStack       = 0xFFFF
  .PageMapLevel4    = 0x70000
  .DefaultStringLen = 512
  .MaxFileSize      = 1024
  .BackFrameBuffer  = 1024*1024*4
  ; Tasks
  .MaxTasks         = 10
  .TaskMaxStackSize = 512-24
  ; Script
  .MaxUsing         = 10  ; Maximum number of 'using'
  .MaxVars          = 20  ; Maximum number of variables
  .MaxStringVarLen  = 50  ; Maximum number of characters for a string variable including the tailing 0
  .MaxFuncParameters = 10 ; Maximum number of parameters for a function
  ; Console
  .InputColor     = 7
  .OutputColor    = 7
  .LineColor      = 8
  .ErrorColor     = 4
  .StringColor    = 8
  ; Code security
  .MaxCicles      = 2000  ; Maximum number of cycles before che InfiniteLoopCheck fails
Bootes.Stage2.Info.FileName equ "STAGE2"


;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Code macros
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

; InfiniteLoopCheck
; CX must NOT be used in a cycle and must be initialized to 0.
MACRO InfiniteLoopCheck ExitTo
{
  INC CX
  CMP CX,Bootes.Stage2.Info.MaxCicles
  JAE ExitTo
}

;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Multiboot structs
;:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
STRUC Multiboot.Info
{
  .Flags      DD ?
  ; With flags bit 0 set
  .LowerMemory  DD ?  ; In kB, max: 640 kB
  .UpperMemory  DD ?  ; Memory over 1 megabytes
  ; With flags bit 1 set
  .BootDevice:
    .BootDevice.Drive DB ?  ; BIOS drive number
    .BootDevice.Part1 DB ?  ; Top level partition (0xFF not used)
    .BootDevice.Part2 DB ?  ; Sub level partition (0xFF not used)
    .BootDevice.Part3 DB ?  ;
  ; With flags bit 2 set
  .CommandLine  DD ?  ; Prompt string address
  ; With flags bit 3 set
  .ModsCount  DD ?
  .ModsAddr   DD ?
  ; With flags bit 4 o 5 set
  .Syms       RD 4
  ; With flags bit 6 set
  .MemMapLength DD ?
  .MemMapAddr   DD ?
  ; With flags bit 7 set
  .DrivesLength DD ?
  .DrivesAddr   DD ?  ; Pointer to a Multiboot.Drives.Info structure
  ; With flags bit 8 set
  .ConfigTable  DD ?  ; ROM config table
  ; With flags bit 9 set
  .BootLoaderName DD ?
  ; With flags bit 10 set
  .AMPTableAddr DD ?  ; Pointer to a Multiboot.AMP.Info structure
  ; With flags bit 11 set
  .VBEInfo      DD ?  ; Pointer to a Vesa.Info structure
  .VBEModeInfo  DD ?  ; Pointer to a Vesa.Mode.Info structure
  .VBEMode      DD ?
  .VBEInterfaceSeg    DD ?
  .VBEInterfaceOffset DD ?
  .VBEInterfaceLength DD ?
}

STRUC Multiboot.Drives.Info
{
  .Size           DD ?  ; Size of this structure
  .DriveNumber    DB ?  ; BIOS Drive number
  .DriveMode      DB ?  ; 0-CHS 1-LBA
  .DriveCylinders DW ?
  .DriveHeads     DB ?
  .DriveSectors   DB ?
  .DrivePorts:          ; Collection of words terminated by 0
}

STRUC Multiboot.AMP.Info  ; Advanced Power Management
{
  .Version    DW ?
  .CSeg     DW ?
  .Offset     DD ?
  .CSeg16     DW ?
  .DSeg     DW ?
  .Flags      DW ?
  .CSegLength   DW ?
  .CSeg16Length DW ?
  .DSegLength   DW ?
}