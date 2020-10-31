#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "error.h"
#include "inode.h"
#include "CFS_init.h"

//if blockSize, nBlocks or nInodes are equal to 0, it will be
//considered the value 256
int CFS_init(char* fsname, char blockSize, char nBlocks, char nInodes)
{
   FILE* fs = fopen(fsname, "wbx");
   //if (fs == NULL) {return errno;}
   if (fs == NULL) {
      if (errno == EEXIST) return ERR_FILE_EXIST;
      return -1;
   }

   //mapSize is the size in bytes of the map of blocks, where each block
   //corresponds to 1 bit. The map of blocks consists of the firstByte
   //(contains the root directory), and all other bytes. mapSize is the
   //number of all bytes except the first one
   unsigned char firstByte = 0x01;
   unsigned char mapSize = (nBlocks == 0)? 31 : (nBlocks - 1) / 8; //in C it's rounded down
   INODE rootInode =
   {
      1, //IS_USED
      1, //IS_DIR
      "/", //NAME
      0, //SIZE
      {0, 0, 0}, //DIRECT_BLOCKS
      {0, 0, 0}, //INDIRECT_BLOCKS
      {0, 0, 0}  //DOUBLE_INDIRECT_BLOCKS
   };

   int inodeVectorSize;
   //removing 1 inode from the vector size because that is the root inode
   if (nInodes == 0) inodeVectorSize = (256 - 1) * sizeof(INODE);
   else              inodeVectorSize = (nInodes - 1) * sizeof(INODE);

   int blockVectorSize;
   if ((blockSize == 0) && (nBlocks == 0)) blockVectorSize = 256 * 256;
   else if (blockSize == 0) blockVectorSize = 256 * nBlocks;
   else if (nBlocks   == 0) blockVectorSize = 256 * blockSize;
   else blockVectorSize = blockSize * nBlocks;

   //Creating vector of zeros to initialize file
   int zeroSize = (blockVectorSize > inodeVectorSize)? blockVectorSize : inodeVectorSize;
   unsigned char *zeros = (unsigned char*)malloc(zeroSize * sizeof(unsigned char));
   if (zeros == NULL) { fclose(fs); remove(fsname); return ERR_OUT_MEMORY;}
   for (int i = 0; i < zeroSize; i++) zeros[i] = 0;

   //---Writing on the file---
   //characteristics of filesystem
   fwrite(&blockSize, sizeof(unsigned char), 1              , fs);
   fwrite(&nBlocks  , sizeof(unsigned char), 1              , fs);
   fwrite(&nInodes  , sizeof(unsigned char), 1              , fs);

   //map of blocks - first byte with root and other bytes
   fwrite(&firstByte, sizeof(unsigned char), 1              , fs);
   fwrite(zeros     , sizeof(unsigned char), mapSize        , fs);

   //vector of inodes - root inode and other empty inodes
   fwrite(&rootInode, sizeof(unsigned char), sizeof(INODE)  , fs);
   fwrite(zeros     , sizeof(unsigned char), inodeVectorSize, fs);

   //inode index of root - index 0
   fwrite(zeros     , sizeof(unsigned char), 1              , fs);

   //vector of blocks
   fwrite(zeros     , sizeof(unsigned char), blockVectorSize, fs);


   //freeing resources
   free(zeros);
   fclose(fs);
   return 0;
}
