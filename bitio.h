/*	Data structure and header functions of the bitio library.
 *	This library allows to read and write to file an arbitrary
 *	number of bits; it also add and remove the padding automatically.
 */
#ifndef _BITIO_H_12345931_
#define _BITIO_H_12345931_
#include<stdint.h>

// structure that rappresents the bitio memory buffer
struct bitio;

/* In read mode, use buf to read data from it.
 * In write mode, use buf to write data on it.
 * 
 *	@param	buf, in read mode is a pointer to a pointer that will
 *			contain the address to the memory buffer to be read.
 * 			In write mode is a pointer to a pointer that will be updated
 * 			with the address of the created memory buffer.
 * 			len, in read mode is the length of the buffer poited by buf.
 * 			In write mode it will contain the length of the created memory
 * 			buffer when bit_close() will be called.
 *			mode, a char that rappresent the mode to open the memory
 *			buffer: 'r' for read mode or 'w' for write mode.
 *
 *	@return	the pointer to the struct bitio rappresenting the opened
 * 			memory buffer or NULL on failure.
 */
struct bitio * bit_open(char *buf, size_t len, char mode);

/*	Try to write nb bit from x to the memory buffer rappresent by b.
 *  If nb is zero the function returns without doing anything;
 *	if nb is greater than 64 nb is set to 64.
 *
 *	@param	b, pointer to a bitio structure.
 *			nb, number of bits to be written in b.
 *			x, contain the bits we want to write to b. First bit to be write is
 *			the least significant bit, the second bit is the second least
 *			significant bit, and so on.
 *			Example: If we want to write only 1 bit.
 *				struct bitio = bit_open("filename");	// open the mem buf
 *				uint64_t val = 0x0000000000000001;		// set only the least
 *														// significant bit
 *				int res = bit_write(file, value, 1);	// write 1 bit
 *
 *	@return	On success a positive number rappresenting the number of bits
 *			written is returned. On failure -1 is returned.
 */
int bit_write(struct bitio *b, uint64_t x, unsigned int nb);

/*	Try to read nb bits from memory buffer rappresent by b and store the actual
 *  number of read bit in stat. If nb is zero the function returns without
 *	doing anything, if nb is greater than 64 nb is set to 64.
 *
 *	@param	b, pointer to a bitio structure.
 *			nb, number of bit to be read from b.
 *			stat, pointer to an integer that will contain tha actual number of
 *			bits read from b.
 *
 *	@return	An integer that contain the bit read from b. First bit read is
 *			stored in the least significant bit of the returned value, the
 *			second bit read is stored in the second least significant bit of the
 *			returned value, and so on.
 *			Example: If we read only 1 bit and then we apply right shift we lose
 *					 the bit read.
 *				int stat;
 *				struct bitio = bit_open("filename");		// open the mem buf
 *				uint64_t val = bit_read(file, 1, &stat);	// read 1 bit
 *				val >> 1; 								// lose the bit just read
 *
 *			How stat parameter is updated.
 *			stat will always contain the actual number of bits readed.
 *			On success stat is a positive value that represents the number of
 *			bits read and stored in the returned value.
 *			If the end of memory buffer is reached before reading nb bits,
 *			stat will contain the number of bit read multiplied by -1. If an
 *			error occurred before reading nb bits, stat will contain the number
 *			of bits read, left shifted by 8 position and multiplied by -1.
 * 			If the last byte of the memory buffer does not contain a corret
 * 			padding value, stat will contain the number of bits read,
 * 			left shiftes by 16 bits and multiplied by -1.
 */
uint64_t bit_read(struct bitio *b, unsigned int nb, int *stat);

/*	Close the memory buffer rappresented by b. The pad is added and then the
 *	data is flushed.
 *
 *	@param	b, pointer to a bitio structure.
 *			len, pointer to size_t variable that will contain the actual
 *			number of bytes written in b->buf.
 *
 *	@return	On success 0, otherwise -1.
 *
 *	How pad works.
 *	Pad is needed when the amount of bits to be written is not a multiple of 8.
 *	When the memory buffer is read it is not possible to know if the original
 *	number of bits written were or not a multiple of 8, so the pad is added
 *	every time. The pad consist of the first bit at 0 and the subsequent bits
 *  at 1.
 *	Examples:
 *		value of last byte:		 ---- -110
 *		bits of pad:			 1111 0
 *		resulting byte with pad: 1111 0110
 *
 *	 If the number of bits of the last byte to be write is multible of 8 we must
 *	 add 8 bit of pad.
 *		bit of pad:		1111 1110
 *
 *	How to remove pad.
 *	When bit_read() reads the last byte from the memory buffer it is passed to
 *	remove_padding() to get the number of bits to be removed.
 */
int bit_close(struct bitio *b, size_t *len);

#endif /* ! _BITIO_H_12345931_ */
