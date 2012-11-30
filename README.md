# Bootes
Bootes is a **16 bit operating system** that was intended to become a scriptable boot loader similar to (GRUB)[http://www.gnu.org/software/grub/].

## History
The project started out of curiosity and ended up becoming a bachelor degree thesys.

To be presented for the thesys, it gained the more aggressive name "iOS" before (some one else)[http://www.apple.com/ios/] used it!
Bootes proved to be a successful project for its new purpose of gaining University points.

It has been discontinued since then (2003) and is now open for learning purposes.
Bit and pieces of functionalities will be collected and put together in the GitHub repository of this project.
The original intentions for Bootes was to boot a C++ operating system (codenamed Chaos) that was never realized.

## Running Bootes
The repository contains a pre-build image of a bootable (floppy disk)[http://en.wikipedia.org/wiki/Floppy_disk] that can be used to run Bootes without the need of building it.

### Running in BOCHS
Download and install the latest version of (BOCHS)[http://bochs.sourceforge.net]. OS X users can use (Homebrew)[http://mxcl.github.com/homebrew/] and follow the instructions of `brew install bochs`.

Run BOCHS *from the bin directory*.

At this point BOCHS should load the provided bochsrc.txt configuration file and start Bootes.

### Running on a floppy
If for some reason you still have a hardware floppy disk reader, you can create a bootable Bootes floppy disk following this checklist:

- Build Bootes (see later)
- Format the floppy with FAT12
- Copy the file `bin/objects/Stage2` to the root of the floppy disk. *This must be the first file written to the disk and must not be renamed or modified*
- Use the command `dd -if=bin/Stage1.FAT12 of=<floppy> count=1` to write the Stage1 to the boot sector (I don't remember how that file got created)
  - `<floppy>` is something like `/dev/fd0` in Unix systems and `\\.\a` for Windows
  - The `dd` command for Windows is available (here)[http://www.chrysocome.net/dd]

At this point you should have a bootable floppy disk. If left inserted when rebooting the machine, Bootes should run.

## What to do in Bootes
Once in Bootes you should be presented with a prompt. Bootes uses name spaced commands and has **tab completion**!
You can also use the `tab` key to cycle through all sub-namespaces.

Explore available commands with:
- `iOS.Script.InfoNamespace()` gives a list of available namespaces.
- `iOS.Script.InfoNamespace("iOS.IO.Console")` will list all functions in the `iOS.IO.Console` namespace.

There is a special directive to get documentation on a command:
- `help iOS.IO.Console.Write` will give a description of the `iOS.IO.Console.Write` command

Hello wordl with shell variable usage:
```
myvar = "Hello world!\n"
iOS.IO.Console.Write( myvar )
```

To shutdown the machine:
- `iOS.Machine.Shutdown()` or `iOS.Machine.Reboot()`

## Building Bootes
To build Bootes you need (FASM)[http://flatassembler.net]. There is also a working (OS X FASM)[http://board.flatassembler.net/topic.php?t=13413&start=20] version in the FASM message board (by Zab, you'll need to register to the board to download the file).

The building process is described in `Makefile.bat` for a Windows console, it's reported here:
- `fasm Stage1\Stage1.asm bin\objects\Stage1` to build the Stage1
- `fasm Stage2\Stage2.asm bin\objects\Stage2` to build the Stage2
- `fasm Stage1\Loaders\Stage1.Loader.FAT32.asm bin\objects\Stage1.Loader.FAT32` to build something unknown
- `copy bin\objects\Stage1/b + bin\objects\Stage2/b  bin\Bootes.img` Windows command to concatenate 2 files using binary mode to read them