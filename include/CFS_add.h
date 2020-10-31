#ifndef CFS_ADD_H
#define CFS_ADD_H

/*This functions adds a file or directory to the filesystem given by
fsname, if the argument content is NULL it will add a directory, it
will add a file with the content given otherwise
   On success the function returns 0, otherwise it will return an error
code correspondig to a given error. The error codes are given bellow:
ERR_FILE_OPEN   - error opening file
ERR_INODE_SIZE  - file size exceeds the limits of the inodes
ERR_NO_INODE    - there is no inode free to store the file
ERR_FILE_EXIST  - a file with the same name already exists
ERR_INV_PATH    - invalid path given in filename argument
ERR_FILENAME    - filename is too big to be stored on the inode (char[10])
ERR_DIR_FULL    - directory can't hold any more files
ERR_OUT_MEMORY  - not enough memory available, error on malloc
ERR_OUT_BLOCKS  - not enough free blocks to store
*/

int CFS_add(char* fsname, char* filename, char* content);
char* getTopPath(char* path, char** nextPath);

#endif /* CFS_ADDFILE_H */
