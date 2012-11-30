;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Stage1 FAT32 Loader
;
;REMARKS-------------------------------------------------------------------------------------------
; Loader for FAT32 filesystem.
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

include "Stage1.Loader.Header.inc"

file "..\Binary\FAT32.bin":3,FAT32BPB.Size-3

; --------------------------------------------------------------
; FAT32 BIOS Parameter Block Header
; --------------------------------------------------------------
STRUC Bootes.IO.FileSystem.FAT.BPB
{
  .Jump               RB 3
  .OEMName            DB "MSDOS5.0"
  .BytesPerSector     DW 512
  .SectorsPerCluster  DB 8
  .ReservedSectors    DW 0x24
  .NumFATs            DB 2
  .RootEntryCount     DW 0
  .TotSec16           DW 0
  .Media              DB 0xF8
  .FATsz16            DW 0
  .SectorsPerTrack    DW 0x3F
  .NumHeads           DW 0xFF
  .HiddSec            DD 0x06BC81BB
  .TotSec32           DD 0x40B708
  ; Only for FAT32
  .FATsz32      DD 0x1026
  .ExtFlags     DW 0
  .Version      DW 0
  .RootCluster  DD 2
  .FSInfo       DW 1 ; Sector where FAT32.Info is. Usually 1.
  .BkBootSector DW 6 ; From this sector and the 2 following there is a copy of the boot sector
                RB 12
  .DriveNumber    DB 0x80,0
  .BootSignature  DB 0x29
  .VolumeID       DD 0xA8622C99
  .VolumeLabel    DB "NO NAME    "
  .FileSystemType DB "FAT32   "
  
  .Size       = $-.Jump
}

VIRTUAL AT BX
  FAT32BPB  Bootes.IO.FileSystem.FAT.BPB
END VIRTUAL


; --------------------------------------------------------------
; Reading of Stage2 from FAT32 filesystem
; --------------------------------------------------------------
Stage1LoaderCode
MOV SI,Bootes.Stage1.Loader.Data.Dbg
CALL Bootes.Stage1.Loader.Print
  ; Get the pointer to the filesystem descriptor structure
  CMP BX,0
  JNE @f
    MOV BX,Bootes.Stage1.Loader.Header.Start
  @@:
  ; Get the root directory sector: HiddSec + ReservedSectors + FATsz32 * NumFATs
  MOVZX CX,BYTE [FAT32BPB.NumFATs]
  MOVZX EAX,WORD [FAT32BPB.FATsz32]
  XOR EDX,EDX
  MUL CX
  MOVZX EDX,WORD [FAT32BPB.ReservedSectors]
  ADD EDX,[FAT32BPB.HiddSec]
  MOV [Bootes.Stage1.Loader.Data.FATFirstSector],EDX
  ADD EAX,EDX
  MOV [Bootes.Stage1.Loader.Data.FATFirstDataSector],EAX
  
  ; Load root directory's content
  MOVZX CX,BYTE [FAT32BPB.SectorsPerCluster]
  MOV [Bootes.Stage1.Loader.Data.FATSectorPerCluster],CX
  MOV BX,Bootes.Stage1.Loader.End
  MOV SI,BX
  @@:
    CALL WORD [Bootes.Stage1.Loader.ReadFunctionHandler]
    OR AX,AX
    JZ .Error
    ; Next sector
    ADD BX,512
    LOOP @b
    
MOV SI,Bootes.Stage1.Loader.Data.Dbg
CALL Bootes.Stage1.Loader.Print

  ; Load Stage2 entry
  .lSearch:
    MOV DI,Bootes.Stage1.Loader.Data.Stage2Name
    ; Check for entry eof
    MOV AL,[SI]
    CMP AL,0
    JE .Error
    ; Entry not allocated
    CMP AL,0xE5
    JMP .lContinue
    ; Comparing entries
    PUSH SI
    MOV CX,Bootes.Stage1.Loader.Data.Stage2Name.Length
    REPE CMPSB
    POP SI
    JE .xSearch
  .lContinue:
    ADD SI,32
    JMP .lSearch
  .xSearch:
  
  ; Get initial Stage2 cluster
  MOV BX,SI
  MOV AX,[BX+20]
  SHL EAX,16
  MOV AX,[BX+26]
  
  ; Load the neccessary FAT portion
  PUSH EAX
  SHR EAX,9-2   ; N * 4 / 512
  ADD EAX,[Bootes.Stage1.Loader.Data.FATFirstSector]
  MOV BX,Bootes.Stage1.Loader.End+512
  CALL WORD [Bootes.Stage1.Loader.ReadFunctionHandler]
  OR AX,AX
  JZ .Error
  POP EAX

MOV SI,Bootes.Stage1.Loader.Data.Dbg
CALL Bootes.Stage1.Loader.Print

  ; Load cluster list to be red
  MOV DI,Bootes.Stage1.Loader.End
  XOR CX,CX
  @@:
    MOV [DI],EAX
    INC CX
    ADD DI,4
    MOV BX,AX
    SHL BX,2
    AND BX,111111111b   ; (N * 4) % 512
    ADD BX,Bootes.Stage1.Loader.End+512
    MOV EAX,[BX]
    CMP EAX,0x0FFFFFF8
    JB @b
  
  ; Load Stage2
  MOV SI,Bootes.Stage1.Loader.End
  MOV BX,Bootes.Stage2.Info.Origin
  .lRead:
    MOV EAX,[SI]
    ADD SI,4
    PUSH CX
    ; Load cluster in EAX to BX
    MOV CX,[Bootes.Stage1.Loader.Data.FATSectorPerCluster]
    PUSH CX
    SHR CX,1
    SUB EAX,2
    SHL EAX,CL
    ADD EAX,[Bootes.Stage1.Loader.Data.FATFirstDataSector]
    POP CX
    .lReadCluster:    
      CALL WORD [Bootes.Stage1.Loader.ReadFunctionHandler]
      OR AX,AX
      JZ .Error
      ADD BX,512
      LOOP .lReadCluster
    ; Next cluster
    POP CX
    LOOP .lRead
  
  ; Pass control to Stage2
  JMP Bootes.Stage2.Info.Origin
  
  .Error:
    JMP Bootes.Stage1.Loader.Footer.PrintError
    ;JMP $


align 2
;============================================================================================== DATA
Bootes.Stage1.Loader.Data:
  .FATSectorPerCluster  DW ?
  .FATFirstSector     DD ?
  .FATFirstDataSector   DD ?
  .Stage2Name       DB Bootes.Stage2.Info.FileName
              times 11-($-.Stage2Name) DB ' '
  .Stage2Name.Length    = 11
  
  .Dbg          DB "a",0

;===================================================================
; Loader Footer
;===================================================================
include "Stage1.Loader.Footer.inc"

Bootes.Stage1.Loader.End: