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
#include <unistd.h>
#include <sys/types.h>

// allowed type for NUM_BIT_PER_WORD are: 8, 16, 32, 64. Any other value can lead to undefined behavior.
#define NUM_BIT_PER_WORD 8

// allowed type for buftype_t are: uint8_t, uint16_t, uint32_t, uint64_t. Any other value can lead to undefined behavior.
#if NUM_BIT_PER_WORD == 8
	typedef uint8_t buftype_t;
	#define MASK_BUFTYPE 0xFF
	#ifdef __APPLE__
		#define bitio_letoh(x) x
	#else
		#define bitio_letoh(x) x
	#endif
#endif
#if NUM_BIT_PER_WORD == 16
	typedef uint16_t buftype_t;
	#define MASK_BUFTYPE 0xFFFF
	#ifdef __APPLE__
		#define bitio_letoh(x) OSReadLittleInt16(x,0)
	#else
		#define bitio_letoh(x) le16toh(x)
	#endif
#endif
#if NUM_BIT_PER_WORD == 32
	typedef uint32_t buftype_t;
	#define MASK_BUFTYPE 0xFFFFFFFF
	#ifdef __APPLE__
		#define bitio_letoh(x) OSReadLittleInt32(x,0)
	#else
		#define bitio_letoh(x) le32toh(x)
	#endif
#endif
#if NUM_BIT_PER_WORD == 64
	typedef uint64_t buftype_t;
	#define MASK_BUFTYPE 0xFFFFFFFFFFFFFFFF
	#ifdef __APPLE__
		#define bitio_letoh(x) OSReadLittleInt64(x,0)
	#else
		#define bitio_letoh(x) le64toh(x)
	#endif
#endif

#define NUM_BIT_BUFTYPE (sizeof(buftype_t)*8)

struct bitio	// structure that rappresents the bitio object
{
	int bitio_mode;	// 'r' for read mode, 'w' for write mode
	unsigned bitio_rp, bitio_wp;	// index of the next bit to read and write
	buftype_t *buf;	// pointer to the buffer used to read/write data
	size_t len_buf;	// length (in bytes) of the buffer
};

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

struct bitio *
bit_open(char *buf, size_t len, char mode)
{
	struct bitio *ret=NULL;
	unsigned int pos;
	if (!buf || (mode!='r' && mode!='w') || len==0) {
		errno=EINVAL;	// errno will contain the error code
		goto fail;
	}
	ret=calloc(1, sizeof(struct bitio)); // memory is cleared
	if (!ret) // if calloc() fail
		goto fail;
	ret->buf = (buftype_t *) buf;
	ret->bitio_mode=mode;
	ret->bitio_rp=0; // reset read index
	if (mode=='r') {
		pos=remove_padding( *( ((char*)ret->buf)+len-1 ) );
		if (pos==9) {
			errno=EINVAL;
			goto fail; // non valid pad value
		}
		ret->bitio_wp=len*8 - pos; // reset write index
		ret->len_buf=len;
	}
	else {
		ret->len_buf=len;
		ret->bitio_wp=0;
	}
	return ret;

fail:	// undo partial action
	if (ret) {	// if struct bitio was allocated
		ret->buf=(buftype_t *)0;
		free(ret);	// release bitio memory
		ret=NULL;
	}
	return NULL;
}

buftype_t
bit_read(struct bitio *b, unsigned int nb, int *stat)
{
	unsigned int pos, ofs, bit_da_leggere, bit_da_leggere_old=0;
	buftype_t ris=0, d, mask;
	*stat=0;
	if (nb==0)	// we can read from 1 to NUM_BIT_BUFTYPE bit
		return 0;
	if (nb>NUM_BIT_BUFTYPE)
		nb=NUM_BIT_BUFTYPE;
	if (!b || b->bitio_mode!='r')	// if we are not in read mode, return
		return MASK_BUFTYPE;
	do {
		if (b->bitio_rp==b->bitio_wp) {	// end of the buffer
			*stat= (*stat)*(-1);	// see documentation
           	return ris;
        }
        
		pos=b->bitio_rp/NUM_BIT_BUFTYPE;	// get last word
		ofs=b->bitio_rp%NUM_BIT_BUFTYPE;	// offset in the last word
		
 		d=bitio_letoh(b->buf[pos]);	// read last word from buffer
		
		/*
			Calculate how many bits we can read now:
			If write and read inexes are spaced more than NUM_BIT_BUFTYPE
			bit or they are in different word then max number of bits that
			we can read is NUM_BIT_BUFTYPE-ofs, otherwise is the difference
			between the two indexes, that is b->bitio_wp - b->bitio_rp.
			Now we must compare this value with the max number of bits that we
			want to read, that is nb-bit_da_leggere_old.
		*/
		bit_da_leggere= ( (b->bitio_wp - b->bitio_rp >= NUM_BIT_BUFTYPE) || (pos!=(b->bitio_wp/NUM_BIT_BUFTYPE)) ) ? ( (NUM_BIT_BUFTYPE-ofs)<(nb-bit_da_leggere_old) ? (NUM_BIT_BUFTYPE-ofs) : (nb-bit_da_leggere_old) ) : ( (b->bitio_wp - b->bitio_rp)<(nb-bit_da_leggere_old) ? (b->bitio_wp - b->bitio_rp) : (nb-bit_da_leggere_old) ) ;
		
		// make the mask used to write the bit we want in the word
		mask=(bit_da_leggere==NUM_BIT_BUFTYPE) ? (MASK_BUFTYPE) : ( (((uint8_t)1)<<bit_da_leggere)-1 );
		// use the mask to read bit_da_leggere bit from d and shift them to the
		// right position and store in the result variable
		ris|=((d&(mask<<ofs))>>ofs)<<bit_da_leggere_old;
		*stat=(*stat)+bit_da_leggere;	// update status of the read operation
		bit_da_leggere_old+=bit_da_leggere;	// rememer how many bit we have read
		b->bitio_rp+=bit_da_leggere;	// update read index
	} while (nb!=(*stat));	// we pretend to read nb bit,
							// unless we reach the end of the buffer
	return ris;	// return the word containing the read bits
}

int
bit_write(struct bitio *b, buftype_t x, unsigned int nb)
{
	unsigned int pos, ofs, nb2, bit_da_scrivere;
	buftype_t d, mask;
	if (nb>NUM_BIT_BUFTYPE)
		nb=NUM_BIT_BUFTYPE;
	if (!b || b->bitio_mode!='w')	// if we are not in write mode, return
		goto fail;
	if (b->bitio_wp+nb > b->len_buf*8 - 1)
		nb=b->len_buf*8 - 1 - b->bitio_wp;
	if (nb==0)	// we can write from 1 to NUM_BIT_BUFTYPE bit
		return 0;
	nb2=nb;
	do {
		if (b->bitio_wp==b->len_buf*8 - 1)	// if we reached the end of the buf
			return nb2-nb;			// return the actual number of bit written

		pos=b->bitio_wp/NUM_BIT_BUFTYPE;	// get last word
		ofs=b->bitio_wp%NUM_BIT_BUFTYPE;	// offset in the last word
		
 		d=bitio_letoh(b->buf[pos]);	// read last word from buffer
		
		// calculate how many bit we can read now
		bit_da_scrivere=(NUM_BIT_BUFTYPE-ofs)<nb ? (NUM_BIT_BUFTYPE-ofs) : nb;
		// make the mask used to write the bit we want in the word
		mask=(bit_da_scrivere==NUM_BIT_BUFTYPE) ? (MASK_BUFTYPE) : ( (((uint8_t)1)<<bit_da_scrivere)-1 );
		// use the mask to clean the positions in d where we will put new bit
		d&=~(mask<<ofs);
		// write the new bit at the correct offset in the word
		d|=((x&mask)<<ofs);

		x>>=bit_da_scrivere;	// remove the written bit from the input word
		b->bitio_wp+=bit_da_scrivere;	// update write index
		
		b->buf[pos]=d;	// update last word in the buffer
	} while (nb-=bit_da_scrivere);	// continue until we write nb bit,
									// or we reach the end of the buf
	return nb2;	// return the number of bits written

fail:
	return -1;
}

int
bit_close(struct bitio *b, size_t *len)
{
	char *des=(char *)b->buf;
	char pad, mask;
	int i;
	if (!b)	// if file is not properly initialized
		return -1;
	if (b->bitio_mode=='w') {	// we add pad only in write mode
		if ((i=b->bitio_wp%8)) {	// if bit are not multiple of 8
			mask=~(((char)0x80)>>(8-i-1));
			pad=des[b->bitio_wp/8] & mask;	// get the last uncomplete byte, and clean it
			for (++i; i<8; i++)
				pad |= (((char)1)<<i);
		}
		else	// if bits are multiple of 8
			pad=0xFE;	// shortcut to set the pad
		
		des[b->bitio_wp/8]=pad;	// update last byte
		b->bitio_wp+=8-(b->bitio_wp%8);	// update write index
		
		*len = b->bitio_wp/8; // number of bytes used in the buffer, pad included
	}

	free(b);
	return 0;	// on success return 0
}
