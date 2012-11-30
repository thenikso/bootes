;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
; Bootes.IO.FileSystems.<FileSystemName>
;
; [File system implementation description]
;::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
align 2


; --------------------------------------------------------------
; FSEntries
; --------------------------------------------------------------
Bootes.IO.FileSystems.<FileSystemName>	Bootes.IO.FileSystems.FSHandler <FSid>,"FileSystemName",/
										Bootes.IO.FileSystems.<FileSystemName>.Check,/
										Bootes.IO.FileSystems.<FileSystemName>.Mount,/
										Bootes.IO.FileSystems.<FileSystemName>.Dir,/
										Bootes.IO.FileSystems.<FileSystemName>.Read,/
										Bootes.IO.FileSystems.<FileSystemName>.Write
; Altri file system implementati.


;====================================================================
; Inizializza la gestione del driver per il <FileSystemName>
;====================================================================
Bootes.IO.FileSystems.FAT.Init:
	CALL Bootes.IO.FileSystems.AddFS, Bootes.IO.FileSystems.<FileSystemName>
	; Inizilizzazione altri file system implementati.
	RET



;====================================================================
; Controlla se l'header specificato individua una partizione con
; questo file system.
; --------------------------------------------------------------
; PARAMS
;	Header		Puntatore alla zona di memoria contenente l'header
;				da ispezionare.
; RETURN
;	Zero se l'header non corrisponde altrimenti non-zero.
;====================================================================
Function Bootes.IO.FileSystems.<FileSystemName>.Check, Header
Begin
	; Implementazione
Return



;====================================================================
; Monta la partizione indicata inizilalizzando i parametri 
; necessari.
; --------------------------------------------------------------
; PARAMS
;	Partition	Puntatore alla struttura Bootes.Devices.Disk.Partition
;				da inizializzare.
; RETURN
;	Zero se l'inizializzazione non è stata possibile altrimenti
;	non-zero.
;====================================================================
Function Bootes.IO.FileSystems.<FileSystemName>.Mount, Partition
Begin
	; Implementazione
Return



;====================================================================
; Legge il file specificato dalla partizione indicata.
; --------------------------------------------------------------
; PARAMS
;	Partition	Puntatore alla struttura Bootes.Devices.Disk.Partition
;				dalla quale leggere.
;	Directory	Puntatore alla stringa terminata da zero contenente
;				il nome della directory della quale elencare il 
;				contenuto.
; RETURN
;	Zero se è avvenuto un errore altrimenti il puntatore al buffer
;	contenente la lista di nomi dei file presenti nella directory
;	corrente.
;====================================================================
Function Bootes.IO.FileSystems.<FileSystemName>.Dir, Partition,Directory
Begin
	; Implementazione
Return



;====================================================================
; Legge il file specificato dalla partizione indicata.
; --------------------------------------------------------------
; PARAMS
;	Partition	Puntatore alla struttura Bootes.Devices.Disk.Partition
;				dalla quale leggere.
;	FileName	Puntatore alla stringa terminata da zero contenente
;				il nome del file da leggere.
;	Buffer		Puntatore al buffer di dati dove verrà letto il file
;				oppure zero per far creare il buffer dalla funzione.
;	Size		Numero massimo di byte da leggere oppure zero per 
;				non porre limite.
; RETURN
;	Zero se è avvenuto un errore altrimenti il puntatore al buffer
;	nel quale è stato letto il file desiderato.
;====================================================================
Function Bootes.IO.FileSystems.<FileSystemName>.Read, Partition,FileName,Buffer,Size
Begin
	; Implementazione
Return



;====================================================================
; Scrive il file specificato nella partizione indicata con i dati nel
; buffer.
; --------------------------------------------------------------
; PARAMS
;	Partition	Puntatore alla struttura Bootes.Devices.Disk.Partition
;				nella quale scrivere.
;	FileName	Puntatore alla stringa terminata da zero contenente
;				il nome del file da scrivere.
;	Buffer		Puntatore al buffer di dati da scrivere nel file.
;	Size		Numero di byte da scrivere.
; RETURN
;	Zero se è avvenuto un errore altrimenti non zero.
;====================================================================
Function Bootes.IO.FileSystems.<FileSystemName>.Write, Partition,FileName,Buffer,Size
Begin
	; Implementazione
Return