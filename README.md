# bitio library - momory buffer branch
This branch of the bitio library allows to read/write specific amount of Bits from/to a memory buffer.
It provides a structure that represent the memory buffer to which operate and a set of functions to perform operations on it.

The functions provided are:
* bit_open()
* bit_read()
* bit_write()
* bit_flush()
* bit_close()

When the _bit_close()_ function is called the pad is automatically added to the end of the memory buffer.
When write mode is used the bitio library create the memory buffer to which data will be written.
When a memory buffer is read and the end is reached the pad is automatically discarded.

The _bit_read()_ and _bit_write()_ functions operate on an internal buffer, when this buffer is empty (or full) a access operation on the memory buffer is performed.

This library does not allow to open a memory buffer both in read and write mode.
