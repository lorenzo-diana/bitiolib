# bitio library
This library allows to read/write specific amount of Bits from/to a file.
It provide a structure that represent the file to which operate and a set of functions to perform operations on the file.

The functions provided are:
* bit_open()
* bit_read()
* bit_write()
* bit_flush()
* bit_close()

When the _bit_close()_ function is called the pad is automatically added to the end of the file.
When a file is read and the end is reached the pad is automatically discarded.

The _bit_read()_ and _bit_write()_ functions operate on an internal buffer, when the buffer is empty (or full) a disk access is performed.

This library does not allow to open a file both in read and write mode.
