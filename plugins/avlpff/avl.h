#ifndef H_AVL
#define H_AVL

#include <stdlib.h>
#include <stdio.h>

typedef int avl_kt;

#ifndef NULL
#define NULL   ((void *) 0)
#endif

#ifndef bool
#define bool   char
#endif

#ifndef true
#define true   1
#endif

#ifndef false
#define false   0
#endif

struct avl_node_s {
	avl_kt key;
	unsigned char h;

	struct avl_node_s * left;
	struct avl_node_s * right;

	void * value;
};

typedef struct avl_node_s * avl_n;


//Returns searched node
avl_n avl_search(
		avl_n base,
		const avl_kt key);

//Returns root node
avl_n avl_insert(
		avl_n base,
		const avl_kt key,
		void * value,
		avl_n* stored);

//Returns root node
avl_n avl_remove(
		avl_n base,
		const avl_kt key);

void avl_release_tree(avl_n base);

#endif
