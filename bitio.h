/*	Data structure and header functions of the bitio library.
 *	This library allows to read and write to file an arbitrary
 *	number of bits; it also add and remove the padding automatically.
 */
#ifndef _BITIO_H_12345931_
#define _BITIO_H_12345931_
#include<stdint.h>

// structure that rappresents the bitio file
struct bitio;

/*	Open a file specified by simbolic name in read or write mode.
 * 
 *	@param	name, pointer to the string rappresenting the name of the file
 *			mode, a char that rappresent the mode to open the file: 'r' for
 *			read mode or 'w' for write mode.
 *
 *	@return	the pointer to the struct bitio rappresenting the opened file or
 *			NULL on failure.
 */
struct bitio * bit_open(const char *name, char mode);

/*	Try to write nb bit from x to the file rappresent by b. If nb is zero the
 *	function returns without doing anything; if nb is greater than 64 nb is set
 *	to 64.
 *
 *	@param	b, pointer to a bitio file.
 *			nb, number of bits to be written in b.
 *			x, contain the bits we want to write to b. First bit to be write is
 *			the least significant bit, the second bit is the second least
 *			significant bit, and so on.
 *			Example: If we want to write only 1 bit.
 *				struct bitio = bit_open("filename");	// open the file
 *				uint64_t val = 0x0000000000000001;		// set only the least
 *														// significant bit
 *				int res = bit_write(file, value, 1);	// write 1 bit
 *
 *	@return	On success nb is returned, -1 on failure.
 */
int bit_write(struct bitio *b, uint64_t x, unsigned int nb);

/*	Try to read nb bits from file rappresent by b and store the actual number of
 *	read bit in stat. If nb is zero the function returns without doing anything,
 *	if nb is greater than 64 nb is set to 64.
 *
 *	@param	b, pointer to a bitio file.
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
 *				struct bitio = bit_open("filename");		// open the file
 *				uint64_t val = bit_read(file, 1, &stat);	// read 1 bit
 *				val >> 1; 								// lose the bit just read
 *
 *			How stat parameter is updated.
 *			stat will always contain the actual number of bits readed.
 *			On success stat is a positive value that represents the number of
 *			bits read and stored in the returned value.
 *			If EOF is reached before reading nb bits, stat will contain the
 *			number of bit read multiplied by -1. If an error occurred before
 *			reading nb bits, stat will contain the number of bits read, left
 *			shifted by 8 position and multiplied by -1. If the last byte of
 * 			the file does not contain a corret padding value, stat will
 * 			contain the number of bits read, left shiftes by 16 bits and
 * 			multiplied by -1.
 */
uint64_t bit_read(struct bitio *b, unsigned int nb, int *stat);

/*	Close the file rappresented by b. The pad is added and then the buffer
 *	is flushed.
 *
 *	@param	b, pointer to a bitio file.
 *
 *	@return	On success 0, otherwise -1.
 *
 *	How pad works.
 *	Pad is needed when the amount of bits to be written is not a multiple of 8.
 *	When the file is read it is not possible to know if the original number
 *	of bits written were or not a multiple of 8, so the pad is added every
 *	time. The pad consist of the first bit at 0 and the subsequent bits at 1.
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
 *	When bit_read() reads the last byte from the file it is passed to
 *	remove_padding() to get the number of bits to be removed.
 */
int bit_close(struct bitio *b);

/*	Write to file the data in the buffer. If the number of bits in the
 *	buffer is not a multiple of 8 the least #bit % 8 bits  will remain
 *	in the buffer.
 *
 *	@param	b, pointer to the struct that rappresent a bitio file.
 *
 *	@return	On success an integer that rappresent the actual number of
 *			bits writed in the file, on failure -1 is returned.
 */
int bit_flush(struct bitio *b);

#endif /* ! _BITIO_H_12345931_ */
