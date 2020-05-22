#include <stdio.h>
#include "../bitio.h"

#define TEST_FILE_NAME_1_BYTE "test_1.bin"
#define TEST_FILE_NAME_2_BYTE "test_2.bin"
#define TEST_FILE_NAME_0_BYTE "test_0.bin"
#define data_value (uint64_t) 0x00000000DEADBEEF

/*
 * Generate e file named TEST_FILE_NAME and write one bit of useful data.
 * At the end of this test the file size shuold be 1 byte: 1 bit of 
 * useful data and 7 bits of padding.
 */
int test_1()
{
    struct bitio * f = bit_open(TEST_FILE_NAME_1_BYTE, 'w');
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
    
    return 0;
}

/*
 * Read the file previously created by test_1(), read one bit of useful data
 * from it, and compare it with the one writed by test_1().
 */
int test_2()
{
    uint64_t data_r;
    int stat=0;
    struct bitio * f = bit_open(TEST_FILE_NAME_1_BYTE, 'r');
    if (!f)
    {
        printf("Test 2 - Error on open!\n");
        return -1;
    }
    
    data_r = bit_read(f, 1, &stat);
    if (stat != 1)
    {
        printf("Test 2 - Error on read!\n");
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
int test_3()
{
    uint64_t data_r;
    int stat=0;
    struct bitio * f = bit_open(TEST_FILE_NAME_2_BYTE, 'w');
    if (!f)
    {
        printf("Test 3 - Error on first open!\n");
        return -1;
    }
    
    if (bit_write(f, data_value, 8) != 8)
    {
        printf("Test 1 - Error on write!\n");
        return -2;
    }
    
    if (bit_close(f))
    {
        printf("Test 3 - Error on first close!\n");
        return -3;
    }
    
    f = bit_open(TEST_FILE_NAME_2_BYTE, 'r');
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
int test_4()
{
	uint64_t data_r;
	int stat=0;
	struct bitio * f = bit_open(TEST_FILE_NAME_0_BYTE, 'w');
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
	
	f = bit_open(TEST_FILE_NAME_0_BYTE, 'r');
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
    
    res = test_1();
    if (!res)
        printf("test 1 ok\n");
    else
        printf("test 1 - error value: %d\n", res);
    
    res = test_2();
    if (!res)
        printf("test 2 ok\n");
    else
        printf("test 2 - error value: %d\n", res);
    
    res = test_3();
    if (!res)
        printf("test 3 ok\n");
    else
        printf("test 3 - error value: %d\n", res);
    
    res = test_4();
    if (!res)
        printf("test 4 ok\n");
    else
        printf("test 4 - error value: %d\n", res);
}
