/*	Provide functions and data structure to open, close, read and write to a
 *	file an arbitrary number of bits. A pad is added and removed automatically.
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

// size of the internal buf where data are stored before flush it to the file
#define BITIO_BUF_WORDS 512

struct bitio	// structure that rappresents the bitio file
{
	FILE *bitio_fd;	// file descriptor
	int bitio_mode;	// 'r' for read mode, 'w' for write mode
	unsigned bitio_rp, bitio_wp;	// index of the next bit to read and write
	uint64_t buf[ BITIO_BUF_WORDS ];	// buf size is 64*512= 32768 bit
};

struct bitio *
bit_open(char **buf, size_t *len, char mode)
{
	struct bitio *ret=NULL;
	if (!buf || (mode!='r' && mode!='w')) {
		errno=EINVAL;	// errno will contain the error code
		goto fail;
	}
	ret=calloc(1, sizeof(struct bitio)); // memory is cleared
	if (!ret) // if calloc() fail
		goto fail;
	if (mode=='r')
		ret->bitio_fd=fmemopen(*buf, *len, "rb");
	else
		ret->bitio_fd=open_memstream(buf, len);
	if (!ret->bitio_fd) // if open() fail
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

int
bit_flush(struct bitio *b)
{
	int len_bytes, x, left;
	char *start, *dst;
	if (!b || (b->bitio_mode != 'w') || (!b->bitio_fd)) {
		errno=EINVAL;
		return -1;
	}
	len_bytes=b->bitio_wp/8;	// how many bytes we can flush
	if (len_bytes == 0)	// if we can't write any byte, return
		return 0;
	start=dst=(char*)b->buf;	// start from beginning of the buffer
	left=len_bytes;
	for(;;) {	// untill we write len_bytes bytes
        x=fwrite(start, 1, left, b->bitio_fd);	// try to write len_bytes bytes
        if (ferror(b->bitio_fd))
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
	if (dst!=start) {
		for (x=0; x<left; x++)	// copy remaining byte on the top of the buffer
			dst[x]=start[x];
		if (b->bitio_wp%8)	// if data in buffer is not multiple of 8
			dst[x]=start[x];	// move remaining bit on top of the buffer
		b->bitio_wp-=(start-dst)*8;	// update write index
		return (start-dst)*8;	// return how many bit were written
	}
	return 0;
}

/*	Returns the number of bits that rappresent the pad.
 *
 *	@param	The byte that contain the pad.
 *
 *	@return	The number of bits to be deleted in order to remove the pad from the
 *			file. This number is between 1 and 8. If 9 is returned this means
 *			that last_byte does not cotain a valid pad value.
 *
 *	@note	To see how pad works look at bit_close() documentation.
 */
uint8_t
remove_padding(char last_byte) {
	uint8_t val=0x80, count=1;
	while (val & last_byte) {
		val>>=1;
		count++;
	}
	return count;
}

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
			x=fread((void *)b->buf, 1, sizeof(b->buf), b->bitio_fd);
            // if (x!=sizeof(b->buf)) then check for efo and error
            if (!x && feof(b->bitio_fd)) { // EOF reached
                *stat= (*stat)*(-1);	// see documentation
                return ris;
            }
            if (ferror(b->bitio_fd)) {   // on error
                *stat= ((*stat)<<8)*(-1);	// see documentation
                return ris;
            }
			if (x>0) {
				// in read mode write index rappresent last readeble bit
				b->bitio_wp+=x*8;	// update write index
				// determines whether we have read all the bytes of the file
				// read actual position in the file
				pos_att=ftell(b->bitio_fd);
				// read the position of the lastabyte in the file
                fseek(b->bitio_fd, 0, SEEK_END);
                dim_file=ftell(b->bitio_fd);
				// reset the offest of the file in the previous position
				fseek(b->bitio_fd, pos_att, SEEK_SET);
				if (dim_file==pos_att) { // if file is ended, delete padding
				// once pad is removed write index might not be multiple of 8 !
					pos=remove_padding( *( ((char*)b->buf)+x-1 ) );
					if (pos==9) {
						*stat= ((*stat)<<16)*(-1);	// see documentation
						return ris;
					}
					b->bitio_wp-=pos;
				}
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
			Calculate how many bits we can read now:
			If write and read inexes are spaced more than 64 bit or they are in
			different word then max number of bits that we can read is 64-ofs,
			otherwise is the difference between the two indexes, that is
			b->bitio_wp - b->bitio_rp.
			Now we must compare this value with the max number of bits that we
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
		mask=(bit_da_scrivere==64) ? (0xFFFFFFFFFFFFFFFF) : ( (((uint64_t)1)<<bit_da_scrivere)-1 );
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

int
bit_close(struct bitio *b)
{
	char *des=(char *)b->buf;
	char pad, mask;
	int i;
	if (!b || (!b->bitio_fd))	// if file is not properly initialized
		return -1;
	if (b->bitio_mode=='w') {	// we add pad only in write mode
		if ((i=b->bitio_wp%8)) {	// if bit are not multiple of 8
			mask=~(((char)0x80)>>(8-i-1));
			pad=des[b->bitio_wp/8] & mask;	// get the last uncomplete byte, and clean it
			for (++i; i<8; i++)
				pad |= (((char)1)<<i);
		}
		else {	// if bits are multiple of 8
			if (b->bitio_wp==sizeof(b->buf)*8)	// if the buf is full
				if (bit_flush(b) < 0)			// flush the buffer
					goto fail;					// if error occur go to fail
			pad=0xFE;	// shortcut to set the pad
		}
		
		des[b->bitio_wp/8]=pad;	// update last byte
		b->bitio_wp+=8-(b->bitio_wp%8);	// update write index
		
		i=bit_flush(b);	// flush the buffer
		if (b->bitio_wp)	// flush must update write index to 0
			goto fail;
	}
	fclose(b->bitio_fd);	// close file
	b->bitio_fd=0;
	free(b);
	return 0;	// on success return 0

fail:
	fclose(b->bitio_fd);	// close the file anyway
	b->bitio_fd=0;
	free(b);
	return -1;
}
