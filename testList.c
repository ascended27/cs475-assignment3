#include <string.h>

#include "list.h"

int main(int argc, char**argv) 
{
	node_ptr list = makeSingleList();
	traverseListRight(list);

	node_ptr n1 = makeNode("80", "localhost", "/csapp.h", "some code", 10, 1, NULL, NULL);
	node_ptr n2 = makeNode("80", "localhost", "/tiny.c", "some code", 10, 1, NULL, NULL);

	insertNode(n1, list);
	insertNode(n2, list);

	//incrementCount("http://localhost:2000/", list);

	node_ptr n = removeByPath("80","localhost", "/tiny.c", list);

	if(n)
		freeNode(n);

	node_ptr n3 = removeByPath("80","localhost", "/csapp.h", list);

	if(n3)
		freeNode(n3);

	node_ptr n4 = removeByPath("80", "localhost", "/some.h", list);

	if(n4)
		freeNode(n4);

	
	traverseListRight(list);

	deleteList(list);
}
