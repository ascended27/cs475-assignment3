#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

node_ptr makeSingleList()
{
    node_ptr start = makeNode(NULL, NULL, NULL, NULL, -1,  -1, NULL, NULL);
    node_ptr end = makeNode(NULL, NULL, NULL, NULL, -1,  -1, NULL, NULL);

    start->next = end;
    end->previous = start;

    return start;
}

//traverse the seg lists from left to right
//used for debugging
void traverseListLeft(node_ptr dummy)
{
    while(dummy)
    {
		if(dummy->size == -1)
		{
			printf("\n-------------------\n");
			printf("DUMMY\n");
			printf("-------------------\n");
		}
		else
		{
			printf("\n-------------------\n");
			if(dummy->path)
				printf("path: %s\n", dummy->path);
			else
				printf("path: no path\n");

			if(dummy->host)
				printf("host: %s\n", dummy->host);
			else
				printf("host: no host\n");

		//	printf("size: %s\n", dummy->data);
			printf("size: %d\n", dummy->size);
			printf("uses: %d\n", dummy->countUses);
			printf("-------------------\n");
		}

        dummy = dummy->previous;
    }

    printf("\n");
}

//traverse the seg lists from left to right
//used for debugging
node_ptr traverseListRight(node_ptr dummy)
{
	printf("CALLING TRAVERSAL\n");
    node_ptr pointer;

    while(dummy)
    {
		if(dummy->size == -1)
		{
			printf("\n-------------------\n");
			printf("DUMMY\n");
			printf("-------------------\n");
		}
		else
		{
			printf("\n-------------------\n");
			if(dummy->path)
				printf("path: %s\n", dummy->path);
			else
				printf("path: no path\n");

			if(dummy->host)
				printf("host: %s\n", dummy->host);
			else
				printf("host: no host\n");

		//	printf("size: %s\n", dummy->data);
			printf("size: %d\n", dummy->size);
			printf("uses: %d\n", dummy->countUses);
			printf("-------------------\n");
		}

        if(!dummy->next)
            pointer = dummy;
        dummy = dummy->next;
    }

    printf("\n");

    return pointer;
}

//function checks to see of the pointers are equal
int nodesEqual(node_ptr node1, node_ptr node2)
{
    if(node1 == NULL || node2 == NULL)
        return 0;

    return strcmp(node1->port, node2->port) == 0 && strcmp(node1->path, node2->path) == 0 && strcmp(node1->host, node2->host) == 0;
}

node_ptr selectNodeByPath(char* port, char* host, char* path, node_ptr list)
{
    node_ptr current = list->next;

    while(current && !(strcmp(current->port, port)==0 && strcmp(current->host, host)==0 && strcmp(current->path, path)==0))
    {
        current = current->next;
    }

	if(current == NULL)
	{
		//printf("My value before: %d\n", current);
		return 0;
	}

    return current;
}

//Function to remove a node from a set by searching by path, will reconnect list under the hood
node_ptr removeByPath(char* port, char* host, char* path, node_ptr list)
{
	node_ptr node = selectNodeByPath(port, host, path, list);
	//printf("My value now: %s", node->path);

	return removeNode(node, list);
}

//Function to remove a node from a set, will reconnect list under the hood
node_ptr removeNode(node_ptr node, node_ptr list)
{
    //if any argument is null, do nothing
    if(!node || !list)
	{
		//printf("NODE NULL");
		//fflush(stdout);
        return NULL;
	}

    //Get previous and current first node in list. Previous is the dummy node at start.
    node_ptr prev = list;
    node_ptr current = prev->next;

    //if set is empty, just end execution of the function
    if(isEmpty(list) || (node->size == -1))
    {
        return NULL;
    }

    //traverse list until match found.
    while(current && current->next && !nodesEqual(current, node))
    {
        //printf("Moving to appropriate node\n");
        prev = current;
        current = current->next;
    }

    //reconnect
    prev->next = current->next;

    current->next->previous = prev;

    current->next = NULL;
    current->previous = NULL;

    return current;
}

//inserts the nNode into the given seg list(dummy)
//the nNode is inserted in address-order(lowest to highest)
int insertNode(node_ptr nNode, node_ptr dummy)
{
	//if parameters are null, exit
    if(!nNode || !dummy)
        return 0;

	if(nNode->size > MAX_OBJECT_SIZE)
		return 0;

    //get dummy node and node after dummy node
    node_ptr current = dummy->next;
    node_ptr prev = dummy;

    //traverse the list and stop once the current string is "greater" than the new string
    while(current && (nNode->path > current->path))
    {
        //printf("MOVING:\ncurrentAddress: %d, nNodeAddress: %d\n", current->address, nNode->address);

        if(current->size == -1)
            break;

        //move to next node both current and prev pointers
        prev = current;
        current = current->next;
    }

    nNode->next = current;
    current->previous = nNode;

    prev->next = nNode;
    nNode->previous = prev;

    return 1;
}

//helper function used to instantiates the nodes' fields
node_ptr makeNode(char* port, char* host, char* path, char* data, int size, int countUses, node_ptr prev, node_ptr next)
{
    node_ptr node = malloc(sizeof(node_rec));

    if(!node)
    {
        printf("Allocation of node failed. Exiting program...\n");
        exit(1);
    }

	int i;
    for(i = 0; i < MAX_OBJECT_SIZE; i++)
	{
		node->data[i] = '\0';
	}

    //if(!node->data)
    //{
      //  printf("Allocation of node failed. Exiting program...\n");
        //exit(1);
    //}


	if(path != NULL)
	{
    	strcpy(node->path, path);
	}

	if(host != NULL)
	{
		//printf("HOST NOT NULL");
    	strcpy(node->host, host);
	}

	if(port != NULL)
	{
		//printf("HOST NOT NULL");
    	strcpy(node->port, port);
	}

    node->size = size;
    node->countUses = countUses;

	if(data != NULL)
	{
    	strcpy(node->data, data);
	}

    node->previous = prev;
    node->next = next;

    return node;
}

//Deletes all contents from the list passed into it
void deleteList(node_ptr start)
{
	//temporary nodes to traverse the list and kill each node
	node_ptr current = start;
	node_ptr toKill = start;

	//traverse the list
	while(current)
	{
		//move current
		current = current->next;
		//Helper function to kill a node.		
		freeNode(toKill);
		//move previous to current
		toKill = current;
	}
}

//Function to free the given node's memory
node_ptr freeNode(node_ptr node)
{
	//check if node isn't already NULL
	if(node)
	{
		//make next and previous NULL to unlink it from any potential lists
		node->next = NULL;
		node->previous = NULL;

		//free space occupied by node
		free(node);
	}
	
	//Will return null if successful.
	return node;
}

char* serveData(char* port, char* host, char* path, node_ptr list)
{
	//fetch node containing object from given URL
	node_ptr node = selectNodeByPath(port, host, path, list);

	//check if the node is null
	if(node)
	{
		//if node exists, return the pointer to the data
		return node->data;
	}
	else
		return NULL;
}

//Function that will search for node with given path and increment its countUses by one.
void incrementCount(char* port, char* host, char* path, node_ptr list)
{
    node_ptr node = selectNodeByPath(port, host, path, list);
	//printf("path: %s", node->path);
	if(node)
    	node->countUses++;
}

void incrementCountForNode(node_ptr node)
{
	node->countUses++;
}

int isEmpty(node_ptr list)
{
    return (list->size == -1 && list->next->size == -1);
}
