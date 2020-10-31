#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "inode.h"
#include "CFS_add.h"

int CFS_add(char* fsname, char* filename, char* content)
{
   //Opening filesystem
   FILE* fs = fopen(fsname, "rb+");
   if (fs == NULL) return ERR_FILE_OPEN;

   //reading filesystem characteristics
   char c[10];
   int blockSize, nBlocks, nInodes;
   fread(c, sizeof(unsigned char), 3, fs);
   blockSize = (c[0] == 0)? 256: c[0];
   nBlocks   = (c[1] == 0)? 256: c[1];
   nInodes   = (c[2] == 0)? 256: c[2];

   //Checking if file is too big to be adressed by the inode used (only 3 direct blocks)
   int fileSize = (content == NULL)? 0 : (int)strlen(content);
   if (fileSize > blockSize * 3 || fileSize > 255) {fclose(fs); return ERR_INODE_SIZE;}

   //Checks if there is a free inode to store file
   int i;
   unsigned char freeInode;
   const unsigned int inodeStart = 3 + 1 + (nBlocks - 1) / 8; //start of the vector of inodes
   const unsigned int blockStart = inodeStart + nInodes * sizeof(INODE) + 1; //start of the vector of blocks
   for (i = 0; i < nInodes; i++)
   {
      unsigned char is_used;
      fseek(fs, inodeStart + i * sizeof(INODE), SEEK_SET);
      fread(&is_used, sizeof(unsigned char), 1, fs);
      if (!is_used) {freeInode = (unsigned char)i; break;}
   }
   if (i == nInodes) {fclose(fs); return ERR_NO_INODE;} //there is no inode free

   //---Checking if given filename is a valid path in the filesystem---
   i = 0;
   unsigned int dir = 0;   //index of inode of file directory - to be found
   unsigned int dirFree = 0; //will hold the byte index of a free space where
                             //the index of the inode of the file will be saved
   unsigned char direct_block[3];
   char* nextPath = NULL;
   char* topPath = getTopPath(filename, &nextPath);

   //checks if the whole tree branch given by filename exists
   for(;;)
   {
      //moves to the directory being searched - root in the beginning
      fseek(fs, inodeStart + dir * sizeof(INODE) + 13, SEEK_SET);
      fread(direct_block, sizeof(unsigned char), 3, fs);

      for (i = 0; i < 3; i++)
      {
         if (direct_block[i] == 0 && dir != 0) continue;
         int j; for (j = 0; j < blockSize; j++)
         {
            unsigned char curInode;
            //go to byte j on block pointed by direct_block[i]
            fseek(fs, blockStart + direct_block[i] * blockSize + j, SEEK_SET);
            fread(&curInode, sizeof(unsigned char), 1, fs); //reads inode index on block
            if (curInode == 0)
            {
               if (nextPath == NULL && dirFree == 0)
                  dirFree = blockStart + direct_block[i] * blockSize + j;
               i = 3; break;
            }

            //go to inode given by the block read before
            unsigned char is_dir;
            fseek(fs, inodeStart + curInode * sizeof(INODE) + 1, SEEK_SET);
            fread(&is_dir, sizeof(unsigned char), 1, fs);
            fread(c, sizeof(unsigned char), 10, fs);
            if (strcmp(c, topPath) == 0) //if name is the same as topPath
            {
               if (nextPath == NULL) {fclose(fs); return ERR_FILE_EXIST;}
               if (!is_dir) {fclose(fs); return ERR_INV_PATH;}
               dir = curInode;
               break;
            }
         }
         if (j != blockSize) break;
      }
      if (i == 3)
      {
         if (nextPath == NULL) break;
         fclose(fs); return ERR_INV_PATH;
      }
      topPath = getTopPath(NULL, &nextPath);
   }
   //topPath now holds the filename without its parent directories
   //dir holds the inode index of the dir where the file will be written
   //direct_block holds the index of blocks belonging to dir

   //checking if filename is too big
   if (strlen(topPath) >= 10) {fclose(fs); return ERR_FILENAME;}

   //Checking if the directory where the file is to be written is full
   unsigned char free_direct_block = 0; //used to check if there is
                                        //still a direct_block to write
   if (!dirFree)
   {
      for (i = (dir == 0); i < 3; i++) {if (direct_block[i] == 0) break;}
      if (i == 3) {fclose(fs); return ERR_DIR_FULL;} //dir can't hold any more files
      free_direct_block = (unsigned char)(i + 1);
   }

   //calculating how many blocks needed to store file
   unsigned char fnBlocks = (unsigned char)((fileSize == 0)? 0 : 1 + (fileSize - 1) / blockSize);
   if (free_direct_block) fnBlocks++; //one more block to the dir

   //Checking if there are enough free blocks and allocating them
   unsigned char *freeBlocks = (unsigned char*)malloc(fnBlocks * sizeof(unsigned char));
   if (freeBlocks == NULL) {fclose(fs); return ERR_OUT_MEMORY;}
   unsigned char availableBlocks = 0;
   unsigned char mask, map;

   for (i = 0; i < nBlocks; i++)
   {
      //Calculates mask and reads byte from map
      if (i % 8 == 0)
      {
         fseek(fs, 3 + i / 8, SEEK_SET);
         fread(&map, sizeof(unsigned char), 1, fs);
         mask = 1;
      }
      else mask *= 2;
      if ((mask & map) == 0)
      {
         if (availableBlocks == fnBlocks) break;
         freeBlocks[availableBlocks++] = (unsigned char)i;
      }
   }

   //if there's not enough free blocks
   if (availableBlocks != fnBlocks) {free(freeBlocks); fclose(fs); return ERR_OUT_BLOCKS;}

   //allocating blocks in map
   for (i = 0; i < fnBlocks; i++)
   {
      fseek(fs, 3 + freeBlocks[i] / 8, SEEK_SET);
      fread(&map, sizeof(unsigned char), 1, fs);
      mask = 1 << (freeBlocks[i] % 8);
      map |= mask;
      fseek(fs, -1, SEEK_CUR);
      fwrite(&map, sizeof(unsigned char), 1, fs);
   }

   //----updating directory with inode of new file----
   unsigned char *blockData = (unsigned char*)malloc(blockSize * sizeof(unsigned char));
   if (blockData == NULL) {free(freeBlocks); fclose(fs); return ERR_OUT_MEMORY;}
   if (free_direct_block) //if a new block is needed to store information
   {
      //writing new block index on direct_block
      fseek(fs, inodeStart + dir * sizeof(INODE) + 13 + (free_direct_block - 1), SEEK_SET);
      fwrite(freeBlocks, sizeof(unsigned char), 1, fs);

      //fills new block with 0's
      for(int j = 0; j < blockSize; j++) {blockData[j] = 0;}
      fseek(fs, blockStart + freeBlocks[0] * blockSize, SEEK_SET);
      fwrite(blockData, sizeof(unsigned char), blockSize, fs);
      dirFree = blockStart + freeBlocks[0] * blockSize ;
   }
   //updating directory size
   unsigned char dirSize;
   fseek(fs, inodeStart + dir * sizeof(INODE) + 12, SEEK_SET);
   fread(&dirSize, sizeof(unsigned char), 1, fs);
   dirSize++; fseek(fs, -1, SEEK_CUR);
   fwrite(&dirSize, sizeof(unsigned char), 1, fs);
   //writing file inode on a free byte of dir block
   fseek(fs, dirFree, SEEK_SET);
   fwrite(&freeInode, sizeof(unsigned char), 1, fs);

   //----assigning block to file and creates its inode----
   for (i = 0; i < 3; i++)
   {
      int k = (free_direct_block != 0) + i;
      if (k < fnBlocks) direct_block[i] = freeBlocks[k];
      else direct_block[i] = 0;
   }

   INODE fileInode = {
      1,  //IS_USED
      (content == NULL),  //IS_DIR
      {0},   //NAME
      (char)fileSize,  //SIZE
      {direct_block[0], direct_block[1], direct_block[2]},  //DIRECT_BLOCKS
      {0, 0, 0},  //INDIRECT_BLOCKS
      {0, 0, 0}   //DOUBLE_INDIRECT_BLOCKS
   };

   char *name = topPath;
   for (i = 0; i < 10; i++) {fileInode.NAME[i] = *name; if (*name != 0) name++;}
   fseek(fs, inodeStart + freeInode * sizeof(INODE), SEEK_SET);
   fwrite(&fileInode, sizeof(INODE), 1, fs);

   if (content != NULL)
   {
      unsigned char* cc = (unsigned char*) content;
      for (i = 0; i < fnBlocks - (free_direct_block != 0); i++)
      {
         for(int j = 0; j < blockSize; j++) {blockData[j] = *cc; if (*cc != 0) cc++;}
         fseek(fs, blockStart + direct_block[i] * blockSize, SEEK_SET);
         fwrite(blockData, sizeof(unsigned char), blockSize, fs);
      }
   }

   free(freeBlocks);
   free(blockData);
   fclose(fs);
   return 0;
}

//Get dir/file name on top of path, for example:
//getTopPath("test/robot/sapo.txt", &nextPath) : will return the string "test"
//and set nextPath to "robot/sapo.txt"
//the function does not create new arrays to store the strings but changes any
// '/' with a \0 and returns pointers to parts of the original string.
char* getTopPath(char* path, char** nextPath)
{
   if (nextPath == NULL) return NULL;
   if (path != NULL) *nextPath = path;
   if (*nextPath == NULL) return NULL;
   char *c = *nextPath;
   char *r = NULL;
   while (*c != 0)
   {
      //Removes any '/' on the beginning
      if (*c == '/') *(c++) = 0;

      //get first name before /
      else
      {
         r = c++;
         while (*c != '/' && *c != 0) c++;
         while (*c == '/') *(c++) = 0;
         if   (*c == 0) *nextPath = NULL;
         else *nextPath = c;
         return r;
      }
   }
   *nextPath = NULL;
   return NULL;
}
