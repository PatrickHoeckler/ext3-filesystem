#ifndef ERROR_H
#define ERROR_H

/*
Error codes used by the program
This approach was just an exercise to learn about using symbolic error codes,
It's not something i would use in a more serious project
*/

#define ERR_FILE_OPEN  0x01
#define ERR_INODE_SIZE 0x02
#define ERR_NO_INODE   0x03
#define ERR_FILE_EXIST 0x04
#define ERR_INV_PATH   0x05
#define ERR_FILENAME   0x06
#define ERR_DIR_FULL   0x07
#define ERR_OUT_MEMORY 0x08
#define ERR_OUT_BLOCKS 0x09

#endif /* ERROR_H */
