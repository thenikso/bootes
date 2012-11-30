fasm Stage1\Stage1.asm bin\objects\Stage1
fasm Stage2\Stage2.asm bin\objects\Stage2
fasm Stage1\Loaders\Stage1.Loader.FAT32.asm bin\objects\Stage1.Loader.FAT32
copy bin\objects\Stage1/b + bin\objects\Stage2/b  bin\Bootes.img
pause Run BOCHS from the bin directory using the provided bochsrc.txt