ADA SOURCE FILE ENCODINGS

These files are encoded in Latin-1, the default encoding for Ada.
It uses a special character called an "ash"
which is a combined "a" and "e". It is an 8-bit character with code $e6.
This character was used in Old English.
To view the special character æ properly,
use a Latin-1 X11 bitmapped font that ends in -iso8859-1.

You may also have to set the locale on the command line before starting
nedit: LANG=en_US nedit
   or  LC_ALL=en_US.iso88591 nedit

If you have done all of this and can't see the character properly, the file
is probably corrupted. This can happen if it is saved in the wrong 
character encoding.

