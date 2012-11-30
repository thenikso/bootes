fasm Stage1\Stage1.asm bin\Stage1
fasm Stage2\Stage2.asm bin\Stage2
fasm Stage1\Loaders\Stage1.Loader.FAT32.asm bin\Stage1.Loader.FAT32
pause Copia in Bochs
copy bin\Stage1/b + bin\Stage2/b  bin\Bootes.img
pause