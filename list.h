/* Recommended max cache and object sizes */
#include <stdio.h>
#include <stdlib.h>
#include "csapp.h"

#define MAX_OBJECT_SIZE 102400
#define MAX_CACHE_SIZE 1049000

//node struct definition
typedef struct n
{
    char path[MAXLINE];
    int size;
    int countUses;
    char data[MAX_OBJECT_SIZE];
	char host[MAXLINE];
	char port[MAXLINE];
    struct n *next;
    struct n *previous;
} node_rec, *node_ptr;

//List stuff
node_ptr makeNode(char* port, char* host, char* path, char* data, int size, int countUses, node_ptr prev, node_ptr next);
void deleteList(node_ptr start);
node_ptr freeNode(node_ptr node);
node_ptr removeNode(node_ptr node, node_ptr list);
node_ptr selectNodeByPath(char* port, char* host, char* path, node_ptr list);
int insertNode(node_ptr nNode, node_ptr dummy);
int isEmpty(node_ptr list);
node_ptr makeSingleList();

//Special removes
node_ptr removeByPath(char* port, char* host, char* path, node_ptr list);

node_ptr removeByLRU(node_ptr list);
node_ptr selectNodeWithLeastCount(node_ptr list);

//Specific manipulations for this node type
void incrementCount(char* port, char* host, char* path, node_ptr list);
void incrementCountForNode(node_ptr node);

//List traversal
void traverseListLeft(node_ptr list);
node_ptr traverseListRight(node_ptr list);

