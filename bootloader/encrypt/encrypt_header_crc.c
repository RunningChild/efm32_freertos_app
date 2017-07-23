/*****************************************************************
 * This program takes will encrypt a firmware image for use with
 * the EFM32 AES Bootloader.
 *
 * The program reads a plain firmware binary file, adds a hash,
 * encrypts everything and stores the result in a new file.
 *
 * This program uses libtomcrypt to perform the
 * actual AES encryption. libtomcrypt is released under
 * the WTFPL license and can be downloaded from libtom.org
 *
 * When compiling you need to link against libtomcrypt, e.g.
 *
 *   gcc -o encrypt encrypt.c -ltomcrypt
 *
 * The program takes two arguments: the name of the original
 * binary (the plain file) and the filename to write the
 * encrypted image to. If the file exists it will be overwritten.
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <tomcrypt.h>

/** CRC table for the CRC-16. The poly is 0x8005 (x^16 + x^15 + x^2 + 1) */
uint16_t const crc16_table[256] =
    {
        0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
        0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
        0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
        0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
        0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
        0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
        0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
        0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
        0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
        0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
        0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
        0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
        0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
        0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
        0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
        0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
        0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
        0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
        0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
        0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
        0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
        0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
        0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
        0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
        0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
        0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
        0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
        0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
        0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
        0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
        0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
        0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
    };

static inline uint16_t crc16_byte(uint16_t crc, const uint8_t data)
{
    return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}


/**
 * crc16 - compute the CRC-16 for the data buffer
 * @crc: previous CRC value
 * @buffer: data pointer
 * @len: number of bytes in the buffer
 *
 * Returns the updated CRC value.
 */
uint16_t bd_crc16(uint16_t crc, uint8_t const *buffer, uint32_t len)//uint32_t _by_pbh ÐÞ¸Ä
{
    while (len--)
        crc = crc16_byte(crc, *buffer++);
    return crc;
}

/*===========================================================================*/
#define VERSION(a, b, c, d)             ((uint32_t)(  (a << 24) + (b << 16) + (c << 8) + d ))

#define APP_VERSION                     VERSION(5, 0, 0, 0)

/*===========================================================================*/

/* O_BINARY is only used on Windows.
 * Define it to zero on other systems
 * to make the code compatible when compiling */
#ifndef O_BINARY
#define O_BINARY 0
#endif


// Header is padded to fill one XMODEM packet
#define HEADER_SIZE (0x100)//FIRMWARE_HEADER_SIZE

#define AES_KEY_SIZE 32

#define HASH_KEY_SIZE 16

#define AES_BLOCKSIZE 16

#define XMODEM_PACKET_SIZE 128

// The AES encryption key an init vector
uint8_t encryptionKey[AES_KEY_SIZE];
uint8_t initVector[AES_BLOCKSIZE];

// A separate AES key and init vector for
// use when calculating the application hash
uint8_t hashKey[HASH_KEY_SIZE];
uint8_t hashInitVector[AES_BLOCKSIZE];

// This is the buffer for the entire encrypted file
uint8_t *buffer;

// Holds the size of the entire encrypted file
uint32_t encryptedSize;


bool readHexString(char *inputString, uint8_t *outputBytes, int numBytes)
{
  int i;

  char asciiByte[] = { 0 , 0, 0 };

  // Check if string is large enough
  if ( strlen(inputString) / 2 < numBytes ) {
    fprintf(stderr, "Error: hex string is too short!\n");
    return false;
  }

  for ( i=0; i<numBytes; i++ ) {
    strncpy(asciiByte, inputString, 2);
    if ( sscanf(asciiByte, "%hhx", &outputBytes[i]) < 1 ) {
      fprintf(stderr, "Error: key contains invalid characters: %s\n", asciiByte);
      return false;
    }
    inputString += 2;
  }

  return true;
}


/*****************************************************************
 * Reads the AES keys and init vectors from the config file
 *****************************************************************/
bool readKeys(char *configPath)
{
  char lvalue[128];
  char rvalue[128];

  FILE *configFile = fopen(configPath, "r");

  if ( configFile == NULL ) {
    fprintf(stderr, "Unable to open config file %s\n", configPath);
    return false;
  }

  bool foundAesKey = false;
  bool foundInitVector = false;
  bool foundHashKey = false;
  bool foundHashInitVector = false;


  while ( !feof(configFile) ) {
    bool success = true;

    if ( fscanf(configFile, "%s = %s", lvalue, rvalue) < 2 )
      continue;

    if ( strcmp(lvalue, "AES_KEY") == 0 ) {

      if ( !readHexString(rvalue, encryptionKey, AES_KEY_SIZE) ) {
        fprintf(stderr, "Error parsing AES key: %s\n", rvalue);
        return false;
      }
      foundAesKey = true;

    } else if ( strcmp(lvalue, "AES_INITVECTOR") == 0 ) {

      if ( !readHexString(rvalue, initVector, AES_BLOCKSIZE) ) {
        fprintf(stderr, "Error parsing initialization vector: %s\n", rvalue);
        return false;
      }
      foundInitVector = true;

    } else if ( strcmp(lvalue, "HASH_KEY") == 0 ) {

      if ( !readHexString(rvalue, hashKey, HASH_KEY_SIZE) ) {
        fprintf(stderr, "Error parsing hash key: %s\n", rvalue);
        return false;
      }
      foundHashKey = true;

    } else if ( strcmp(lvalue, "HASH_INITVECTOR") == 0 ) {

      if ( !readHexString(rvalue, hashInitVector, AES_BLOCKSIZE) ) {
        fprintf(stderr, "Error parsing hash init vector: %s\n", rvalue);
        return false;
      }
      foundHashInitVector = true;

    } else {
      fprintf(stderr, "Unknown parameter: %s\n", lvalue);
    }

  }

  fclose(configFile);

  if ( !foundAesKey ) {
    fprintf(stderr, "Missing AES key!\n");
    return false;
  } else if ( !foundInitVector ) {
    fprintf(stderr, "Missing init vector\n");
    return false;
  } else if ( !foundHashKey ) {
    fprintf(stderr, "Missing hash key\n");
    return false;
  } else if ( !foundHashInitVector ) {
    fprintf(stderr, "Missing hash init vector\n");
    return false;
  } else {

    // All keys found
    return true;
  }
}


/*****************************************************************
 * This function calculates the hash for the application.
 * When this function returns successfully, the hash
 * is written to hashOutput.
 *
 * Returns false if the operation failed. In this case
 * the value of hashOutput should be ignored.
 *****************************************************************/
bool calculateHash(uint8_t *startAddr, int length, uint8_t *hashOutput)
{
  // Helper variables
  int cryptError, i, j;

  // This variable will always point to the current block
  // to be decrypted
  uint8_t *curBlock;

  // Calculate the number of AES blocks
  int aesBlocks = length / AES_BLOCKSIZE;

  // Input buffer used to hold the input block to the
  // encryption routine
  uint8_t inputBlock[AES_BLOCKSIZE];

  // Initialize key
  symmetric_key skey;
  cryptError = aes_setup(hashKey, HASH_KEY_SIZE, 0, &skey);

  if ( cryptError != CRYPT_OK ) {
    fprintf(stderr, "Error initializing crypto library\n");
    return false;
  }

  // hashOutput will always contain the last encrypted block
  // Initialize with the init vector for the first iteration
  memcpy(hashOutput, hashInitVector, AES_BLOCKSIZE);

  // Loop over all blocks
  for ( i=0; i<aesBlocks; i++ ) {

    // Get address of the current AES block
    curBlock = startAddr + i * AES_BLOCKSIZE;

    // XOR current block with previous cipher
    for ( j=0; j<AES_BLOCKSIZE; j++ ) {
      inputBlock[j] = curBlock[j] ^ hashOutput[j];
    }

    // Encrypt a block. Result is stored in hashOutput
    cryptError = aes_ecb_encrypt(inputBlock, hashOutput, &skey);

    if ( cryptError != CRYPT_OK ) {
      fprintf(stderr, "Error during hash calculation\n");
      return false;
    }
  }

  // Success
  return true;
}


/*****************************************************************
 * Reads the plain text file (the original application binary).
 * The file is loaded into the global buffer variable. Space
 * for the buffer is allocated by this function itself.
 *****************************************************************/
bool readPlainFile(char *fileName)
{
  int plainFileSize, i;

  // Open the file
  int fileHandle = open(fileName, O_RDONLY | O_BINARY);
  if ( fileHandle < 0 ) {
    fprintf(stderr, "Error opening file %s\n", fileName);
    return false;
  }

  // Get the size
  plainFileSize = lseek(fileHandle, 0, SEEK_END);
  if ( plainFileSize < 0 ) {
    fprintf(stderr, "Error opening file %s\n", fileName);
    return false;
  }
  lseek(fileHandle, 0, SEEK_SET);

  // Calculated the total size for the encrypted output
  encryptedSize = HEADER_SIZE + plainFileSize;

#if 1
  // Pad the size to matche a integer number of XMODEM packets
  // One XMODEM packet is 128 bytes
  while ( encryptedSize % XMODEM_PACKET_SIZE != 0 ) encryptedSize++;
#endif

  // Allocate memory for the encrypted file
  buffer = (uint8_t *)malloc(encryptedSize);

  // Create pointer to the where the image should be placed
  uint8_t *p = buffer + HEADER_SIZE;

  // Read the entire file
  int bytesLeft = plainFileSize;
  int bytesRead;

  while ( bytesLeft > 0 ) {
    bytesRead = read(fileHandle, p, bytesLeft);
    if ( bytesRead < 0 ) {
      fprintf(stderr, "Error reading file %s\n", fileName);
      return false;
    }

	if ( bytesRead == 0 ) {
		return false;
	}

    bytesLeft -= bytesRead;
    p += bytesRead;
  }

  close(fileHandle);

  return true;
}


/*****************************************************************
 * Calculates the appliation hash and writes the rest of
 * the header information.
 *****************************************************************/
bool writeHeader(void)
{
  int i;

  // Get a pointer to the size field
  uint32_t *fwSize = (uint32_t *)buffer;

  // Get a pointer to the hash field
  uint8_t *hash = buffer + 4;

  // Store the size of encrypted binary in header
  *fwSize = encryptedSize - HEADER_SIZE;

  // Calculate and store hash in header (words 1-4)
  if ( !calculateHash(buffer + HEADER_SIZE, encryptedSize - HEADER_SIZE, hash) ) {
    return false;
  }

#if 0
  // Fill rest of header with random bytes
  srand(time(NULL));
  for ( i=4+AES_BLOCKSIZE; i<HEADER_SIZE; i++ ) {
	buffer[i] = rand() % 0xff;
  }
#else
  uint32_t *verified = (uint32_t *)(buffer+4+AES_BLOCKSIZE);

  // Store verified in header
  *verified = 0x55555555;

  // Fill rest of header with 0xff byte
  for ( i=4+AES_BLOCKSIZE+4; i<HEADER_SIZE; i++ ) {
	buffer[i] = 0xff;
  }

#if 1
  //ota_update_crc
  uint32_t *ota_update_crc = (uint32_t *)(buffer+4+AES_BLOCKSIZE+4 + 4*1);
  uint16_t crc_result = 0;
#if 1
    crc_result = bd_crc16(crc_result, buffer, encryptedSize);
#else
	uint32_t address;
	uint32_t actual_length;
    for(address = 0; address < encryptedSize; address += 1024)
    {
        actual_length = MIN(encryptedSize-address, 1024);
		printf("[%d]actual_length: %d\n", address, actual_length);
        crc_result = bd_crc16(crc_result, buffer+address, actual_length);
    }
#endif
  *ota_update_crc = crc_result;

  //ota_update_flag
  uint32_t *ota_update_flag = (uint32_t *)(buffer+4+AES_BLOCKSIZE+4 + 4*0);
  *ota_update_flag = 0x55555555;

  //ota_update_length
  uint32_t *ota_update_length = (uint32_t *)(buffer+4+AES_BLOCKSIZE+4 + 4*2);
  *ota_update_length = encryptedSize;

  //ota_update_version
  uint32_t *ota_update_version = (uint32_t *)(buffer+4+AES_BLOCKSIZE+4 + 4*3);
  *ota_update_version = APP_VERSION;
#endif

#endif

  // Print header information
  printf("=== Header Information === \n");
  printf("Size: %d\n", *fwSize);

  printf("Hash: ");
  for ( i=0; i<AES_BLOCKSIZE; i++ ) {
    printf("%.2x", hash[i]);
  }
  printf("\n");

  printf("Verified: 0x%x\n", *verified);

  printf("Ota_update_flag: 0x%x\n", *ota_update_flag);
  printf("Ota_update_crc: 0x%x\n", *ota_update_crc);
  printf("Ota_update_length: 0x%x\n", *ota_update_length);
  printf("Ota_update_version: 0x%x\n", *ota_update_version);


  printf("=== End of Header Information ===\n");

  return true;
}

/*****************************************************************
 * Encrypt the entire firmware image (including header) with AES
 * in CBC mode. The image is encrypted in place,
 * when this function returns the buffer array will contain the
 * encrypted image.
 *****************************************************************/
bool encryptImage(void)
{
  // Helper variables
  int cryptError, i, j;

  // Compute number of AES blocks to encrypt
  int aesBlocks = encryptedSize / AES_BLOCKSIZE;

  // The pointer to the current block to be encrypted
  uint8_t *curBlock;

  // Pointer to the last encrypted block
  // Initialize with the init vector
  uint8_t *prevBlock = initVector;

  // Initialize key
  symmetric_key skey;
  cryptError = aes_setup(encryptionKey, AES_KEY_SIZE, 0, &skey);

  if ( cryptError != CRYPT_OK ) {
    fprintf(stderr, "Error initializing crypto library\n");
    return false;
  }

  // Loop over the entire image
  for ( i=0; i<aesBlocks; i++ ) {

    // Get address of the current AES block
    curBlock = buffer + i * AES_BLOCKSIZE;

    // XOR current block with the last encrypted block
    for ( j=0; j<AES_BLOCKSIZE; j++ ) {
      curBlock[j] = curBlock[j] ^ prevBlock[j];
    }

    // Encrypt block in place
    cryptError = aes_ecb_encrypt(curBlock, curBlock, &skey);

    // Store address of current block for next iteration
    prevBlock = curBlock;

    if ( cryptError != CRYPT_OK ) {
      fprintf(stderr, "Error during encryption\n");
      return false;
    }
  }

  return true;
}


/*****************************************************************
 * Writes the encrypted image to file. Returns false if writing
 * failed for any reason.
 *****************************************************************/
bool writeEncryptedFile(char *fileName)
{
  // The size of the final output file
  int outputSize = encryptedSize;

  // The number of bytes written so far
  int bytesWritten = 0;

  // Helper variable
  int writeResult;

  // Open encrypted file for writing
  int fileHandle = open(fileName, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
  if ( fileHandle == -1 ) {
    fprintf(stderr, "Unable to open file %s\n", fileName);
    return false;
  }

  // Loop until entire file has been written
  while ( bytesWritten < outputSize ) {

    // Write encrypted image
    writeResult = write(fileHandle, buffer + bytesWritten, encryptedSize - bytesWritten);

    if ( writeResult < 0 ) {
      fprintf(stderr, "Error writing to file %s\n", fileName);
      return false;
    }

    // Increase the counter
    bytesWritten += writeResult;
  }

  // Close file handle
  if ( close(fileHandle) != 0 ) {
    fprintf(stderr, "Error writing to file %s\n", fileName);
    return false;
  }

  printf("Encrypted image written to %s\n", fileName);

  return true;
}


void writeCArray(const char *format, uint8_t *values, int num, FILE *outFile)
{
  int i;
  char buf[200];
  char strValue[50];

  strcpy(buf, "");

  for ( i=0; i<num; i++ ) {
    if ( i > 0 ) {
      strcat(buf, ", ");
    }
    sprintf(strValue, "0x%.2x", values[i]);
    strcat(buf, strValue);
  }

  fprintf(outFile, format, buf);
}



bool writeBootloaderKeys(char *keyPath)
{
  uint32_t *p;

  FILE *keyFile = fopen(keyPath, "w");

  if ( !keyFile ) {
    fprintf(stderr, "Failed to open bootloader key file %s\n", keyPath);
    return false;
  }

  fprintf(keyFile, "#include <stdint.h>\n\n");

  writeCArray("uint8_t encryptionKey[]  = {%s};\n",
      encryptionKey, AES_KEY_SIZE, keyFile);

  writeCArray("uint8_t initVector[]     = {%s};\n",
      initVector, AES_BLOCKSIZE, keyFile);

  writeCArray("uint8_t hashKey[]        = {%s};\n",
      hashKey, HASH_KEY_SIZE, keyFile);

  writeCArray("uint8_t hashInitVector[] = {%s};\n",
      hashInitVector, AES_BLOCKSIZE, keyFile);

  fclose(keyFile);
  return true;
}



bool file_exists(char *path)
{
  FILE *f = fopen(path, "r");

  if ( f ) {
    fclose(f);
    return true;
  } else {
    return false;
  }
}

void printUsage(char *program)
{
  fprintf(stderr,"Usage: %s <plainfile> <headerfile>\n"
		  "       %s --create-bootloader-keys\n", program, program);
}


int main(int argc, char **argv)
{

  if ( argc == 2 ) {
	if ( strcmp(argv[1], "--create-bootloader-keys") == 0 ) {
	  if ( !readKeys("keys.txt") ) {
		fprintf(stderr, "Aborted.\n");
		return 1;
	  }
	  if ( !writeBootloaderKeys("aes_keys.c") ) {
	    fprintf(stderr, "Aborted.\n");
	    return 1;
	  }
	  return 0;
    } else {
      printUsage(argv[0]);
      return 1;
    }
  } else if ( argc != 3 ) {
    printUsage(argv[0]);
    return 1;
  }

  char *plainPath = argv[1];
  char *encryptedPath = argv[2];


  if ( !readKeys("keys.txt") ) {
    fprintf(stderr, "Aborted.\n");
    return 1;
  }

  if ( !readPlainFile(plainPath) ) {
    fprintf(stderr, "Aborted.\n");
    return 1;
  }

  if ( !writeHeader() ) {
    fprintf(stderr, "Aborted.\n");
    return 1;
  }

#if 0
  if ( !encryptImage() ) {
    fprintf(stderr, "Aborted.\n");
    return 1;
  }
#endif

  if ( !writeEncryptedFile(encryptedPath) ) {
    fprintf(stderr, "Aborted.\n");
    return 1;
  }


  return 0;
}
