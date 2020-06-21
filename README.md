# bitio library - plain buffer branch
This branch of the bitio library allows to read/write specific amount of Bits from/to a user provided memory buffer.
It provides a structure that represent the memory buffer to which operate and a set of functions to perform operations on it.

The functions provided are:
* bit_open()
* bit_read()
* bit_write()
* bit_close()

When the _bit_close()_ function is called the pad is automatically added to the end of the memory buffer.
When write mode is used the bitio library create the memory buffer to which data will be written.
When a memory buffer is read and the end is reached the pad is automatically discarded.

This library does not allow to open a memory buffer both in read and write mode.
