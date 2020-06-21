#include <stdio.h>
#include <stdlib.h>
#include "../bitio.h"

#define data_value (uint64_t) 0x00000000DEADBEEF

/*
 * Generate e file named TEST_FILE_NAME and write one bit of useful data.
 * At the end of this test the file size shuold be 1 byte: 1 bit of 
 * useful data and 7 bits of padding.
 */
int test_1(char **buf, size_t *len)
{
    struct bitio * f = bit_open(buf, len, 'w');
    if (!f)
    {
        printf("Test 1 - Error on open!\n");
        return -1;
    }
    
    if (bit_write(f, data_value, 1) != 1)
    {
        printf("Test 1 - Error on write!\n");
        return -2;
    }
    
    if (bit_close(f))
    {
        printf("Test 1 - Error on close!\n");
        return -3;
    }
    
    if (*len!=1)
	{
		printf("Test 1 - Error data length!\n");
		return -5;
	}
    
    return 0;
}

/*
 * Read the file previously created by test_1(), read one bit of useful data
 * from it, and compare it with the one writed by test_1().
 */
int test_2(char **buf, size_t *len)
{
    uint64_t data_r;
    int stat=0;
    struct bitio * f = bit_open(buf, len, 'r');
    if (!f)
    {
        printf("Test 2 - Error on open!\n");
        return -1;
    }
    
    data_r = bit_read(f, 1, &stat);
    if (stat != 1)
    {
        printf("Test 2 - Error on read!\n");
		printf("stat: %d\n", stat);
		printf("data: %d\n", stat);
        return -2;
    }
    
    if (data_r != (data_value & 0x0000000000000001))
    {
        printf("Test 2 - Error on readed data!\n");
        return -4;
    }
    
    if (bit_close(f))
    {
        printf("Test 2 - Error on close!\n");
        return -3;
    }
    
    return 0;
}

/*
 * Perfotm the same as test_1() and test_2() with 8 bits of useful data.
 * At the end of this test the file size shuold be 2 byte: 8 bits of
 * useful data and 8 bits of padding.
 */
int test_3(char **buf, size_t *len)
{
    uint64_t data_r;
    int stat=0;
	struct bitio * f = bit_open(buf, len, 'w');
    if (!f)
    {
        printf("Test 3 - Error on first open!\n");
        return -1;
    }
    
    if (bit_write(f, data_value, 8) != 8)
    {
        printf("Test 3 - Error on write!\n");
        return -2;
    }
    
    if (bit_close(f))
    {
        printf("Test 3 - Error on first close!\n");
        return -3;
    }
    
    if (*len!=2)
	{
		printf("Test 3 - Error data length!\n");
		return -5;
	}
	
    f = bit_open(buf, len, 'r');
    if (!f)
    {
        printf("Test 3 - Error on second open!\n");
        return -1;
    }
    
    data_r = bit_read(f, 8, &stat);
    if (stat != 8)
    {
        printf("Test 3 - Error on read!\n");
        return -2;
    }
    
    if (data_r != (data_value & 0x00000000000000FF))
    {
        printf("Test 3 - Error on readed data!\n");
        return -4;
    }
    
    if (bit_close(f))
    {
        printf("Test 3 - Error on second close!\n");
        return -3;
    }
    
    return 0;
}

/*
 * Perfotm the same as test_3() with 0 bits of useful data.
 * At the end of this test the file size shuold be 1 byte: 8 bits
 * of padding.
 */
int test_4(char **buf, size_t *len)
{
	uint64_t data_r;
	int stat=0;
	struct bitio * f = bit_open(buf, len, 'w');
	if (!f)
	{
		printf("Test 4 - Error on first open!\n");
		return -1;
	}
	
	if (bit_write(f, data_value, 0) != 0)
	{
		printf("Test 4 - Error on write!\n");
		return -2;
	}
	
	if (bit_close(f))
	{
		printf("Test 4 - Error on first close!\n");
		return -3;
	}
	
	if (*len!=1)
	{
		printf("Test 4 - Error data length!\n");
		return -5;
	}
	
	f = bit_open(buf, len, 'r');
	if (!f)
	{
		printf("Test 4 - Error on second open!\n");
		return -1;
	}
	
	data_r = bit_read(f, 0, &stat);
	if (stat != 0)
	{
		printf("Test 4 - Error on read!\n");
		return -2;
	}
	
	if (data_r != 0)
	{
		printf("Test 4 - Error on readed data!\n");
		return -4;
	}
	
	if (bit_close(f))
	{
		printf("Test 4 - Error on second close!\n");
		return -3;
	}
	
	return 0;
}

int main()
{
    int res;
    size_t len_buf;
    char *buf=0;
    
    res = test_1(&buf, &len_buf);
	if (!res)
		printf("test 1 ok\n");
	else
		printf("test 1 - error value: %d\n", res);
	
	res = test_2(&buf, &len_buf);
	if (!res)
		printf("test 2 ok\n");
	else
		printf("test 2 - error value: %d\n", res);
	free(buf);
	
	res = test_3(&buf, &len_buf);
	if (!res)
		printf("test 3 ok\n");
	else
		printf("test 3 - error value: %d\n", res);
	free(buf);
	
	res = test_4(&buf, &len_buf);
	if (!res)
		printf("test 4 ok\n");
	else
		printf("test 4 - error value: %d\n", res);
	free(buf);
	
	return 0;
}
