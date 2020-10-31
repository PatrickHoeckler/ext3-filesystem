//Standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//Other libraries
#include "error.h"
#include "inode.h"
#include "CFS_init.h"
#include "CFS_add.h"

#define INBUFSIZE 512

void stringToUpper(char* string);
char* getNextArg(char* arguments, char** nextArg);

int main()
{
   char inbuf[INBUFSIZE] = "";
   printf("\tCriador de Filesystem\n");
   for(;;)
   {
      //Receives command from stdin
      printf("->");
      fgets(inbuf, INBUFSIZE, stdin);

      //Removing \n from the end of string
      char *c = inbuf + strlen(inbuf) - 1;
      if (*c == '\n') *c = 0;

      //Separating command from arguments
      char *nextArg = NULL;
      char *command = getNextArg(inbuf, &nextArg);
      if (command == NULL) continue;

      //Separating arguments
      int argc = 0;
      char *argv[4];
      for (argc = 0; argc < 4; argc++)
      {
         if (nextArg == NULL) break;
         argv[argc] = getNextArg(NULL, &nextArg);
         if (argv[argc] == NULL) break;
      }

      //if (nextArg != NULL) there's at least one more argument
      if (nextArg != NULL) argc++;

      //executing command
      stringToUpper(command);
      if (strcmp(command, "EXIT") == 0) break;

      //creates a new EXT3 filesystem
      else if (strcmp(command, "INIT") == 0)
      {
         if (argc != 4) {
            printf("Numero incorreto de argumentos, precisa ser chamado com:\n");
            printf("  INIT [filesystem] [size] [blocks] [inodes]\n");
            printf("Aonde:\n");
            printf("  [filesystem] indica o nome do sistema de arquivos EXT3 para ser criado\n");
            printf("  [size] indica o tamanho dos blocos no sistema de arquivos\n");
            printf("  [blocks] indica a quantidade de blocos no sistema de arquivos\n");
            printf("  [inodes] indica o numero de inodes no sistema de arquivos\n");
            printf("Os valores dos 3 ultimos argumentos devem estar entre 0-255,\n");
            printf("se 0 for passado, o valor considerado sera 256.\n\n");
            continue;
         }
         char blockSize = (char)atoi(argv[1]);
         char nBlocks   = (char)atoi(argv[2]);
         char nInodes   = (char)atoi(argv[3]);
         int r = CFS_init(argv[0], blockSize, nBlocks, nInodes);
         if      (r == ERR_FILE_EXIST) printf("Arquivo '%s' ja existe\n\n", argv[0]);
         else if (r == ERR_OUT_MEMORY) printf("Falta de memoria\n\n");
         else if (r != 0)      printf("Erro ao criar o filesystem\n\n");
      }
      else
      {
         int r;
         //Add file to filesystem
         if (strcmp(command, "ADDFILE") == 0)
         {
            if (argc != 3) {
               printf("Numero incorreto de argumentos, precisa ser chamado com:\n");
               printf("  ADDFILE [filesystem] [filename] [content]\n");
               printf("Aonde:\n");
               printf("  [filesystem] indica o nome do sistema de arquivos EXT3 para adicionar o arquivo\n");
               printf("  [filename] indica o nome do arquivo para ser criado\n");
               printf("  [content] indica o conteudo do arquivo\n\n");
               continue;
            }
            r = CFS_add(argv[0], argv[1], argv[2]);
         }

         //Add directory to filesystem
         else if (strcmp(command, "ADDDIR" ) == 0)
         {
            if (argc != 2) {
               printf("Numero incorreto de argumentos, precisa ser chamado com:\n");
               printf("  ADDDIR [filesystem] [dirname]\n");
               printf("Aonde:\n");
               printf("  [filesystem] indica o nome do sistema de arquivos EXT3 para adicionar o diretorio\n");
               printf("  [dirname] indica o nome do diretorio para ser criado\n\n");
               continue;
            }
            r = CFS_add(argv[0], argv[1], NULL);
         }
         else
         {
            printf("%s - comando desconhecido\n", command);
            printf("Os comandos validos sao (case-insensitive):\n");
            printf("  INIT - Cria um novo sistema de arquivos EXT3\n");
            printf("  ADDFILE - Adiciona um arquivo em um dado sistema de arquivos\n");
            printf("  ADDDIR - Adiciona um diretorio no sistema de arquivos\n");
            printf("  EXIT - Termina o programa\n\n");
            
            continue;
         }

         if      (r == ERR_FILE_OPEN ) printf("Erro ao abrir o filesystem\n\n");
         else if (r == ERR_INODE_SIZE) printf("Tamanho do arquivo exede os limites dos inodes\n\n");
         else if (r == ERR_NO_INODE  ) printf("Nao existe mais inodes para armazenar o arquivo\n\n");
         else if (r == ERR_FILE_EXIST) printf("Um arquivo/pasta com o mesmo nome ja existe\n\n");
         else if (r == ERR_INV_PATH  ) printf("Caminho para o arquivo invalido\n\n");
         else if (r == ERR_FILENAME  ) printf("Nome do arquivo muito longo - ultrapassa 9 caracteres\n\n");
         else if (r == ERR_DIR_FULL  ) printf("Diretorio nao consegue armazenar mais arquivos\n\n");
         else if (r == ERR_OUT_MEMORY) printf("Nao foi possivel concluir a operacao por falta de memoria\n\n");
         else if (r == ERR_OUT_BLOCKS) printf("Nao existem blocos suficientes para armazenar arquivo\n\n");
         else if (r != 0) printf("Ocorreu um erro indeterminado\n\n");
      }
   }
   return 0;
}

void stringToUpper(char* string)
{
   if (string == NULL) return;
   while (*string) { *string = (char)toupper(*string); string++; }
}

char* getNextArg(char* arguments, char** nextArg)
{
	if (arguments != NULL) *nextArg = arguments;
   if (*nextArg == NULL) return NULL;
	char *c = *nextArg;
	char *r = NULL;
	while (*c != 0)
	{
		//Check if argument is between ""
		if (*c == '\"')
		{
			*c = 0;
			r = ++c;
			while (*c != '\"' && *c != 0) c++;
         if   (*c == 0) *nextArg = NULL;
         else {*c = 0;  *nextArg = ++c;}
			return r;
		}

      //Search next word separated by spaces
		if (*c != ' ')
		{
			r = c++;
			while (*c != ' ' && *c != 0) c++;
         if   (*c == 0) *nextArg = NULL;
         else {*c = 0;  *nextArg = ++c;}
			return r;
		}
		*c = 0;
		*nextArg = ++c;
	}
   *nextArg = NULL;
	return NULL;
}
