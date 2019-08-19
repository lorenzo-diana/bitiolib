/*	Provide functions and data structure to open, close, read and write to a
 *	file an arbitrary number of bits. A pad is adding automatically in a
 *	transparent way for the programmer.
 */
// 08/04/2015 17:54:01
#include <errno.h>
#ifdef __APPLE__
	#include <libkern/OSByteOrder.h>
#else
	#include <endian.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define BITIO_BUF_WORDS 512
struct bitio	// the structure that rappresent the bitio file
{
	int bitio_fd;	// file descriptor
	int bitio_mode;	// 'r' for read mode, 'w' for write mode
	unsigned bitio_rp, bitio_wp;	// index of the next bit to read and write
	uint64_t buf[ BITIO_BUF_WORDS ];	// buf size is 64*512= 32768 bit
};

/*	Open a file specified by simbolic name in read or write mode.
 *
 *	@param	name, pointer to the string rappresenting the name of the file
 *			mode, a char that rappresent the mode to open the file: 'r' for
 *			read mode or 'w' for write mode.
 *
 *	@return	the pointer to the struct bitio rappresenting the opened file or
 *			NULL on failure.
 */
struct bitio *
bit_open(const char *name, char mode)
{
	struct bitio *ret=NULL;
	if (!name || (mode!='r' && mode!='w')) {
		errno=EINVAL;	// errno will contain the error code
		goto fail;
	}
	ret=calloc(1, sizeof(struct bitio)); // memory is cleared
	if (!ret) // if calloc() fail
		goto fail;
	ret->bitio_fd=open(name, mode=='r' ? O_RDONLY : O_WRONLY | O_TRUNC | O_CREAT, 0666); // read & write for everyone
	if (ret->bitio_fd<0) // if open() fail
		goto fail;
	ret->bitio_mode=mode;
	ret->bitio_rp=ret->bitio_wp; // reset read and write index
	return ret;

fail:	// undo partial action
	if (ret) {	// if struct bitio was allocated
		ret->bitio_fd=0;	// not necessary in a corret system
		free(ret);	// release bitio memory
		ret=NULL;
	}
	return NULL;
}

/*	Used to write on file the data in the buffer. If the number of bit in the
 *	buffer is not a multiple of 8 the least #bit % 8 bit remain in the buffer.
 *
 *	@param	b, pointer to the struct that rappresent a bitio file.
 *
 *	@return	On success an integer that rappresent the actual number of bit write
 *			in the file, on failure -1 is returned.
 */
int
bit_flush(struct bitio *b)
{
	int len_bytes, x, left;
	char *start, *dst;
	if (!b || (b->bitio_mode != 'w') || (b->bitio_fd < 0)) {
		errno=EINVAL;
		return -1;
	}
	len_bytes=b->bitio_wp/8;	// how many bytes we can flush
	if (len_bytes == 0)	// if we can't write any byte, return
		return 0;
	start=dst=(char*)b->buf;	// start from beginning of the buffer
	left=len_bytes;
	for(;;) {	// untill we write len_bytes bytes
		x=write(b->bitio_fd, start, left);	// try to write len_bytes bytes
		if (x<0)
			goto fail;
		start+=x;	// move forward buffer pointer
		left-=x;	// decrease counter
		if (left==0)	// if we have write len_bytes we can exit from for
			break;
	}
	if (b->bitio_wp%=8)	// if data in buffer is not multiple of 8
		dst[0]=start[0];	// move remaining bit on top of the buffer
	return len_bytes*8;	// return how many bit were written

fail:	// if write() fail
	for (x=0; x<left; x++)	// copy remaining byte on the top of the buffer
		dst[x]=start[x];
	if (b->bitio_wp%8)	// if data in buffer is not multiple of 8
		dst[x]=start[x];	// move remaining bit on top of the buffer
	b->bitio_wp-=(start-dst)*8;	// update write index
	return (start-dst)*8;	// return how many bit were written
}

/*	Get the number of bit that rappresent the pad.
 *
 *	@param	The byte that contain the pad.
 *
 *	@return	The number of bit to be deleted in order to remove the pad from the
 *			file. This number is between 1 and 8
 *
 *	To see how pad works look at bit_close() documentation.
 */
uint8_t
remove_padding(char last_byte) {
	int8_t count;
	for (count=6; count>=0; count--)	//while two consecutive bit are differen
		if ( (last_byte & (((uint8_t)1)<<count)) != (last_byte & (((uint8_t)1)<<(count+1))) )
			break ;
	
	return 8-count-1; // number of bit that make up the pad
}

/*	Try to read nb bit from file rappresent by b and store the actual number of
 *	read bit in stat. If nb is zero the function returns without doing anything,
 *	if nb is greater than 64 nb is set to 64.
 *
 *	@param	b, pointer to a bitio file.
 *			nb, number of bit to be read from b.
 *			stat, pointer to an integer that will contain tha actual number of
 *			bit read from b.
 *
 *	@return	An integer that contain the bit read from b. First bit read is
 *			stored in the first least significant bit of this integer, the
 *			second bit read is stored in the second least significant bit of the
 *			integer, and so on.
 *			Example: If we read only 1 bit and then we apply right shift we lose
 *					 the bit read.
 *				int stat;
 *				struct bitio = bit_open("filename");		// open the file
 *				uint64_t val = bit_read(file, 1, &stat);	// read 1 bit
 *				val >> 1; 							   // lose the bit just read
 *
 *			How stat parameter is update.
 *			stat will always contain the actual number of bit read.
 *			On success stat is a positive value that rappresent the number of
 *			bit read and store in the return value.
 *			If EOF is reached before reading nb bits stat will contain the
 *			number of bit read multiply by -1. If an error occurred before
 *			reading nb bits stat will contain the number of bit read left
 *			shifted by 8 position and multiply by -1.
 *
 *			Example: If stat is a positive value everything went well, otherwise
 *					 if EOF is reached this value is stored in the leas
 *					 significant byte of stat, if an error occurred then this
 *					 value is stored in the second least significant byte.
 *
 */
uint64_t
bit_read(struct bitio *b, unsigned int nb, int *stat)
{
	int x;
	unsigned int pos, ofs, bit_da_leggere, bit_da_leggere_old=0;
	int64_t pos_att, dim_file;
	uint64_t ris=0, d, mask;
	*stat=0;
	if (nb==0)	// we can read from 1 to 64 bit
		return 0;
	if (nb>64)
		nb=64;
	if (!b || b->bitio_mode!='r')	// if we are not in read mode, return
		return 0xFFFFFFFFFFFFFFFF;
	do {
		if (b->bitio_rp==b->bitio_wp) {	// if we have already read all the buf
			b->bitio_rp=b->bitio_wp=0;	// reset read index
			x=read(b->bitio_fd, (void *)b->buf, sizeof(b->buf));
			if (x<0) {	// on error
				*stat= ((*stat)<<8)*(-1);	// see documentation
				return ris;
			}
			if (x==0) { // EOF reached
				*stat= (*stat)*(-1);	// see documentation
				return ris;
			}
			if (x>0) {
				// in read mode write index rappresent last readeble bit
				b->bitio_wp+=x*8;	// update write index
				// determines whether we have read all the bytes of the file
				// read actual position in the file
				pos_att=lseek(b->bitio_fd, 0, SEEK_CUR);
				// read the position of the lastabyte in the file
				dim_file=lseek(b->bitio_fd, 0, SEEK_END);
				// reset the offest of the file in the previous position
				lseek(b->bitio_fd, pos_att, SEEK_SET);
				if (dim_file==pos_att) // if file is ended, delete padding
				// once pad is removed write index might not be multiple of 8 !
					b->bitio_wp-=remove_padding( *( ((char*)b->buf)+x-1 ) );
			}
		}
		pos=b->bitio_rp/(sizeof(b->buf[0])*8);	// get last word
		ofs=b->bitio_rp%(sizeof(b->buf[0])*8);	// offset in the last word
		#ifdef __APPLE__
    		d=OSReadLittleInt64(&b->buf[pos],0);
 		#else
 			d=le64toh(b->buf[pos]);	// read last word from buffer
		#endif
		
		/*
			Calculate how many bit we can read now:
			If write and read inexes are spaced more than 64 bit or they are in
			different word then max number of bit that we can read is 64-ofs,
			otherwise is the difference between the two indexes, that is
			b->bitio_wp - b->bitio_rp.
			Now we must compare this value with the max number of bit that we
			want to read, that is nb-bit_da_leggere_old.
		*/
		bit_da_leggere= ( (b->bitio_wp - b->bitio_rp >= 64) || (pos!=(b->bitio_wp/(sizeof(b->buf[0])*8))) ) ? ( (64-ofs)<(nb-bit_da_leggere_old) ? (64-ofs) : (nb-bit_da_leggere_old) ) : ( (b->bitio_wp - b->bitio_rp)<(nb-bit_da_leggere_old) ? (b->bitio_wp - b->bitio_rp) : (nb-bit_da_leggere_old) ) ;
		
		// make the mask used to write the bit we want in the word
		mask=(bit_da_leggere==64) ? (0xFFFFFFFFFFFFFFFF) : ( (((uint64_t)1)<<bit_da_leggere)-1 );
		// use the mask to read bit_da_leggere bit from d and shift them to the
		// right position and store in the result variable
		ris|=((d&(mask<<ofs))>>ofs)<<bit_da_leggere_old;
		*stat=(*stat)+bit_da_leggere;	// update status of the read operation
		bit_da_leggere_old+=bit_da_leggere;	// rememer how many bit we have read
		b->bitio_rp+=bit_da_leggere;	// update write index
	} while (nb!=(*stat));	// we pretend to read nb bit, unless we get an error
							// or we reach the end of the file
	return ris;	// return the word containing the read bit
}

/*	Try to write nb bit from x to the file rappresent by b. If nb is zero the
 *	function returns without doing anything, if nb is greater than 64 nb is set
 *	to 64.
 *
 *	@param	b, pointer to a bitio file.
 *			nb, number of bit to be write in b.
 *			x, contain the bit we want to write to b. First bit to be write is
 *			the least significant bit, the second bit is the second least
 *			significant bit, and so on.
 *			Example: If we want to write only 1 bit.
 *				struct bitio = bit_open("filename");	// open the file
 *				uint64_t val = 0x0000000000000001;		// set only the least
 *														// significant bit
 *				uint64_t val = bit_write(file, value, 1);	// write 1 bit
 *
 *	@return	On success nb is returned, -1 on failure.
 */
int
bit_write(struct bitio *b, uint64_t x, unsigned int nb)
{
	unsigned int pos, ofs, nb2=nb, bit_da_scrivere;
	uint64_t d, mask;
	if (nb==0)	// we can write from 1 to 64 bit
		return 0;
	if (nb>64)
		nb=64;
	if (!b || b->bitio_mode!='w')	// if we are not in write mode, return
		goto fail;
	do {
		if (b->bitio_wp==sizeof(b->buf)*8)	// if we reached the end of the buf
			if (bit_flush(b) < 0)			// flush the buffer
				goto fail;					// if error occur go to fail

		pos=b->bitio_wp/(sizeof(b->buf[0])*8);	// get last word
		ofs=b->bitio_wp%(sizeof(b->buf[0])*8);	// offset in the last word
		#ifdef __APPLE__
    		d=OSReadLittleInt64(&b->buf[pos],0);
 		#else
 			d=le64toh(b->buf[pos]);	// read last word from buffer
		#endif
		
		// calculate how many bit we can read now
		bit_da_scrivere=(64-ofs)<nb ? (64-ofs) : nb;
		// make the mask used to write the bit we want in the word
		mask= (bit_da_scrivere==64) ? (0xFFFFFFFFFFFFFFFF) : ( (((uint64_t)1)<<bit_da_scrivere)-1 );
		// use the mask to clean the positions in d where we will put new bit
		d&=~(mask<<ofs);
		// write the new bit at the correct offset in the word
		d|=((x&mask)<<ofs);

		x>>=bit_da_scrivere;	// remove the written bit from the input word
		b->bitio_wp+=bit_da_scrivere;	// update write index
		#ifdef __APPLE__
			OSWriteLittleInt64(&b->buf[pos],0,d);
		#else
			b->buf[pos]=htole64(d);	// update last word in the buffer
		#endif
	} while (nb-=bit_da_scrivere);	// continue until we write nb bit
	return nb2;	// return the number of bit written

fail:
	return -1;
}

/*	Close the file rappresented by b, if necessary flush the last bit before
 *	close it. Pad is added if necessary.
 *
 *	@param	b, pointer to a bitio file.
 *
 *	@return	On success 0, otherwise -1.
 *
 *	How pad works.
 *	When we have to write last incomplete byte to the file we complete it by
 *	using a pad. We add from 1 to 7 bit of pad, all these bit have the same
 *	value and this value is the complement of the last bit write by bit_write().
 *	Examples: if last bit write by bit_write() is 1, then the value of the bit
 *			  in the pad will be 0. If is 0 the pad will be all 1.
 *		last bit write:			 1
 *		value of last byte:		 ---- -110
 *		bit of pad:				 0000 0
 *		resulting byte with pad: 0000 0110
 *
 *		last bit write:			 0
 *		value of last byte:		 --01 0110
 *		bit of pad:				 11
 *		resulting byte with pad: 1101 0110
 *
 *	 If the number of bit of the last byte to be write is multible of 8 we must
 *	 add 8 bit of pad.
 *		last bit write:	1
 *		bit of pad:		0000 0000
 *
 *	How to remove pad.
 *	When bit_read() reads the last byte from the file we can get the number of
 *	bit of pad by remove_padding() and remove them.
 *	To find how many bit we must delete we work only on the last byte. We start
 *	reading bit from the most significant bit and count how many consecutive bit
 *	of the same value we found from it to the least singificant one, this value
 *	is the number of bit to be removed from the last byte. This value is from 1
 *	to 8.
 */
int
bit_close(struct bitio *b)
{
	char *des=(char *)b->buf;
	char fill, d;
	int i;
	if (!b || (b->bitio_fd < 0))	// if file is not properly initialized
		return -1;
	if (b->bitio_mode=='w' && b->bitio_wp!=0) {	// we add pad only in write mode
		if (b->bitio_wp%8) {	// if bit are multiple of 8
			fill=des[b->bitio_wp/8];	// get the last uncomplete byte
			d=fill & (((char)1)<<((b->bitio_wp-1)%8));	// get last bit written
		}
		else {	// if bit are multiple of 8
			fill=des[(b->bitio_wp/8)-1];	// get the last uncomplete byte
			d=fill & 0x80;	// get last bit written
		}
		for (i=b->bitio_wp%8; i<8; i++)
			if (d) // last valid bit=1
				fill &= ~(((char)1)<<i);
			else
				fill |= (((char)1)<<i);
		des[b->bitio_wp/8]=fill;	// update last byte
		b->bitio_wp+=8-(b->bitio_wp%8);	// update write index
		
		bit_flush(b);	// flush the buffer
		if (!b->bitio_wp)	// flush must update write index to 0
			goto fail;
	}
	close(b->bitio_fd);	// close file
	b->bitio_fd=0xFFFFFFFF;
	free(b);
	b=NULL;
	return 0;	// on success return 0

fail:
	close(b->bitio_fd);	// close the file anyway
	free(b);
	b->bitio_fd=0;
	b=NULL;
	return -1;
}
