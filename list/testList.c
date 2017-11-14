#include <string.h>

#include "list.h"

int main(int argc, char**argv) 
{
	node_ptr list = makeSingleList();
	traverseListRight(list);

	node_ptr n1 = makeNode("http://localhost:1/", "ABCDEF00", 10, 0, NULL, NULL);
	node_ptr n2 = makeNode("http://localhost:2001/", NULL, 10, 0, NULL, NULL);

	insertNode(n1, list);
	insertNode(n2, list);

	//incrementCount("http://localhost:2000/", list);

	node_ptr n = removeByPath("http://localhost:/", list);

	//if(n)
	//	freeNode(n);
	
	traverseListRight(list);

	deleteList(list);
}
