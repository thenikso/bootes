;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Boot
;
;REMARKS-------------------------------------------------------------------------------------------
;  System start form here!
;  The Stage1 load the Stage2 that will then call the kernel.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
org Bootes.Stage1.Info.Origin
use16
include "..\Shared\DebugMacro.inc"
include "..\Shared\Shared.inc"

STRUC Bootes.Devices.Disk.PartitionEntry
{
  .Flag           DB ?  ; 0x80 - Active partition, 0x00 - Inactive partition
  
  .Start.Head     DB ?
  .Start.Sector   DB ?  ; Starting from 1
  .Start.Cylinder DB ?

  .Type           DB ?
  
  .End.Head       DB ?
  .End.Sector     DB ?
  .End.Cylinder   DB ?
  
  .Start          DW 0,0  ; Starting sector starting from 0 (LBA)
  .Length         DW 0,0  ; Number of sector for the partition
  
  .Size           = $-.Flag
}

VIRTUAL AT BX
  PEntry    Bootes.Devices.Disk.PartitionEntry
END VIRTUAL



;============================================================================================= CODE
Bootes.Stage1.Start:
  ; --------------------------------------------------------------
  ; BIOS Parameters Block
  ; --------------------------------------------------------------
  JMP Bootes.Stage1.Init
  NOP
  
  if Bootes.Stage1.Info.FileSystem eq FAT12
  ; FAT12 Header
    OEMName           DB "MSDOS5.0"
    BytesPerSector    DW 512
    SectorPerCluster  DB 1
    ReservedSectors   DW 1
    NumFATs           DB 2
    RootEntryCount    DW 224
    TotSec16          DW 2880
    Media             DB 0xF0
    FATsz16           DW 9
    SectorsPerTrack   DW 18
    NumHeads          DW 2
    HiddSec           DD 0
    TotSec32          DD 0
    DrivNum           DB 0
                      DB 0
                      DB 0x29
    VolID             DD 0x502EE8A8
    VolLabel          DB "NO NAME    "
    FileSysType       DB "FAT12   "
  else if Bootes.Stage1.Info.FileSystem eq FAT32
  ; FAT32 Header
    file "Binary\FAT32.bin":3,90-3
  end if
  
; times (Bootes.Stage1.Info.BPB.End-($-Bootes.Stage1.Start)) DB 0


Bootes.Stage1.Init:
;====================================================================
; (1) System initialization
;====================================================================
  JMP 0:@f
  @@:
  ; --------------------------------------------------------------
  ; Initialize stack and data segment
  ; --------------------------------------------------------------
  CLI
  XOR AX,AX
  MOV SS,AX
  MOV SP,0x7C00
  MOV DS,AX
  MOV ES,AX
  STI

  ; --------------------------------------------------------------
  ; Show waiting message
  ; --------------------------------------------------------------
  MOV SI,Bootes.Stage1.Data.Loading
  CALL Bootes.Stage1.Print

;====================================================================
; (3) Load Stage2 or the Loader
;====================================================================
  ; Select drive
  MOV AL,[Bootes.Stage1.Data.Stage2.Drive]
  CMP AL,0xFF
  JE @f
    MOV DL,AL
  @@:
  ; Select reading mode
  CALL Bootes.Stage1.SetReadMode
  OR AX,AX
  JZ .iLoadError
  ; Read the Stage2
  if ~ Bootes.Stage1.Info.Loader.Use = 1
    MOV BX,Bootes.Stage2.Info.Origin
    MOV EAX,[Bootes.Stage1.Data.Stage2.StartSector]
    MOV CX,[Bootes.Stage1.Data.Stage2.Sectors]
    @@:
      ; Reading command
      CALL Bootes.Stage1.ReadSector
      OR AX,AX
      JZ .iLoadError
      ; Next sector to read
      ADD BX,512
      LOOP @b
  
  ; Read the loader
  else
    ; Select sector to read
    if Bootes.Stage1.Info.Loader.Sector = 0
      MOV BX,Bootes.Stage1.PartitionTable
      MOV CX,4
      .lTestActive:
        CMP BYTE [PEntry.Flag],0x80
        JE .xTestActive
        ADD BX,PEntry.Size
        LOOP .lTestActive
      JMP .iLoadError
      .xTestActive:
      MOV EAX,DWORD [PEntry.Start]
    else
      MOV EAX,Bootes.Stage1.Info.Loader.Sector
    end if
    ; Read
    MOV BX,Bootes.Stage1.End
    CALL Bootes.Stage1.ReadSector
    OR AX,AX
    JZ .iLoadError
    ; Set the reading function for a loader
    MOV WORD [Bootes.Stage1.End+512-5],Bootes.Stage1.ReadSector
    ; Set the filesystem descriptor
    if ~ Bootes.Stage1.Info.FileSystem eq NONE
      MOV BX,Bootes.Stage1.Start
    else
      MOV BX,0
    end if
  end if
  
  JMP .Jump
  .iLoadError:
    MOV SI,Bootes.Stage1.Data.LoadError
    CALL Bootes.Stage1.Print
    JMP $

;====================================================================
; (4) Pass controll to Stage2 or to the Loader
;====================================================================
  .Jump:
  if ~ Bootes.Stage1.Info.Loader.Use = 1
  JMP Bootes.Stage2.Info.Origin
  else
  JMP WORD [Bootes.Stage1.End+512-3]
  end if


;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; private static void                                                             Bootes.Stage1.Print
;----------------------------------------------------------------------------------------------------
; reg SI: Address of the 0 terminated string to print.
;----------------------------------------------------------------------------------------------------
; Print to screen a string in DS:[E]SI using INT 0x10.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Bootes.Stage1.Print:
   MOV AH,0x0E     ; To call the character writing function with `int 0x10`
   MOV BX,0x0007   ; Set character background
   .loop:
   LODSB           ; LOaD Single Byte from DS:[E]SI in AL
   OR AL,AL
   JZ .xloop
   INT 0x10
   JMP .loop
   .xloop:
   RET

;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; private static void                                                                     SetReadMode
;
;PARAMS----------------------------------------------------------------------------------------------
; DL: Drive to read from.
;
;RETURN----------------------------------------------------------------------------------------------
; Non-zero if the operation was successful.
;
;REMARKS---------------------------------------------------------------------------------------------
; Select the correct reading function for the given drive.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Bootes.Stage1.SetReadMode:
  PUSH BX
  PUSH CX
  PUSH SI
  PUSH DX
  PUSH ES
  
  ; If it's not an HDD use CHS
  TEST DL,0x80
  JZ .CHSMode
  ; Check if LBA is supported
  PUSH DX
  MOV AX,0x4100
  MOV BX,0x55AA
  INT 0x13
  POP DX
  JC .CHSMode
  CMP BX,0xAA55
  JNE .CHSMode
  ; Check if INT 0x13 AH=0x42 is supported
  AND CX,1
  JZ .CHSMode
  .LBAMode:
    ; Try a read
    MOV SI,Bootes.Stage1.ReadSectorLBA.DiskAddressPacket
    MOV AX,0x4200
    INT 0x13
    JC .CHSMode
    ; The function works
    MOV WORD [Bootes.Stage1.ReadSector.Function],Bootes.Stage1.ReadSectorLBA
    JMP .Exit
  .CHSMode:
    ; Try to use l'INT 0x13 ah=0x08
    MOV AH,0x08
    INT 0x13
    JNC .SaveProbe
    ; Check again if it's not an HDD
    TEST DL,0x80
    JNZ .iDriveError
    ; Floppy
    .FloppyProbe:
      MOV SI,.ProbeValues
      MOV CL,0
      @@:
      ; Reset floppy
      MOV AX,0
      INT 0x13
      ; Next ProbeValue
      MOV CL,[SI]
      INC SI
      CMP CL,0
      JE .iDriveError
      ; Try a read
      MOV BX,Bootes.Stage2.Info.Origin
      MOV AX,0x0201
      MOV CH,0
      MOV DH,0
      INT 0x13
      JC @b
      MOV DH,1
    .SaveProbe:
    ; Save the number of sector per track
    AND CX,00111111b
    MOV [Bootes.Stage1.ReadSectorCHS.SecPerTrack],CX
    ; Save the number of heads
    MOVZX AX,DH
    INC AX
    MOV [Bootes.Stage1.ReadSectorCHS.Heads],AX
    .Done:
    MOV WORD [Bootes.Stage1.ReadSector.Function],Bootes.Stage1.ReadSectorCHS
  .Exit:
  MOV AX,1
  JMP .End
  
  .iDriveError:
  MOV AX,0
  
  .End:
  POP ES
  POP DX
  MOV [Bootes.Stage1.ReadSector.Drive],DL
  POP SI
  POP CX
  POP BX
  RET
  
  .ProbeValues  DB 36, 18, 15, 9, 0
  

;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; private static void                                                                      ReadSector
;
;PARAMS----------------------------------------------------------------------------------------------
; EAX: Sector to be read (LBA).
; ES:BX: Destination buffer.
;
;RETURN----------------------------------------------------------------------------------------------
; EAX contains the next sector to be read on success. 0 on error.
;
;REMARKS---------------------------------------------------------------------------------------------
; Read a sector from the drive set up with SetReadMode.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
Bootes.Stage1.ReadSector:
  PUSH EAX
  PUSH DX
  
  MOV DL,[.Drive]
  CALL [.Function]
  
  POP DX
  ; Check if the function was successful
  OR AX,AX
  JZ .Error
  POP EAX
  INC EAX
  RET
  
  .Error:
  POP EAX
  XOR AX,AX
  RET
  
  .Function DW ?
  .Drive    DB ?
  
  

Bootes.Stage1.ReadSectorCHS:
  PUSH CX
  PUSH DX
  
  XOR DX,DX
  DIV WORD [.SecPerTrack]
  INC DL
  MOV CL,DL ; Sector read!

  XOR DX,DX
  DIV WORD [.Heads]
  MOV CH,AL ; Track
  MOV AL,DL ; Head
  POP DX
  MOV DH,AL

  MOV AX,0x0201 ; To prepare reading function with `INT 0x13`
  INT 0x13
  JC .iLoadError
  JMP .End
  
  .iLoadError:
  XOR AX,AX
  
  .End:
  POP CX
  RET
  
  .Heads        DW ?
  .SecPerTrack  DW ?
  

Bootes.Stage1.ReadSectorLBA:
  PUSH SI
  MOV [.DiskAddressPacket.Start],EAX
  MOV [.DiskAddressPacket.Buffer],BX
  MOV SI,.DiskAddressPacket
  MOV AX,0x4200
  INT 0x13
  POP SI
  JC .iLoadError
  MOV AX,1
  RET
  .iLoadError:
  XOR AX,AX
  RET
  
  .DiskAddressPacket:
  .DiskAddressPacket.Size   DB 0x10, 0
  .DiskAddressPacket.Blocks DW 1
  .DiskAddressPacket.Buffer DW Bootes.Stage2.Info.Origin,0
  .DiskAddressPacket.Start  DD 0,0
  

;============================================================================================== DATA
Bootes.Stage1.Data:
    .Loading      DB 13,10,'Loading...',0
    .LoadError    DB 'Error',0

  ; Stage2 sectors
  Debug.Print.Addr "Drive byte"
  .Stage2.Drive   DB 0xFF ; Specify Stage2 drive. 0xFF to specity the boot drive

  if ~ Bootes.Stage1.Info.Loader.Use = 1
  Debug.Print.Addr "Stage2 LBA sectors"
  ; Sectors to be red by Stage1
  .Stage2.StartSector DD Bootes.Stage2.Info.StartSector   ; Start
  .Stage2.Sectors     DW Bootes.Stage2.Info.SectorsCount  ; Number of sectors
  end if

;===================================================================
; Padding
;===================================================================
BootSectFree = ((512-(Bootes.Stage1.End-Bootes.Stage1.PartitionTable))-($-Bootes.Stage1.Start))
times BootSectFree DB 0
Debug.Print.String "Spazio codice disponinile: ",BootSectFree," bytes"

;===================================================================
; Partitions
;===================================================================
Bootes.Stage1.PartitionTable:
if Bootes.Stage1.Info.UsePartitionTable = 1
  file "Binary\MBR.bin":446,64
end if
  
;===================================================================
; Signature
;===================================================================
DW Bootes.Stage1.Info.BootSignature

Bootes.Stage1.End: