#ifndef H_MULTIAVL
#define H_MULTIAVL

#include "avl.h"

struct multiavl_store_s {
	bool hasContent;
	avl_n down;
	void  * value;
};

typedef struct multiavl_store_s * t_mavl;

t_mavl mavl_new_node ();

void mavl_release_node (
		t_mavl node);

t_mavl mavl_insert (
		t_mavl base,
		void * value,
		unsigned char * keys,
		unsigned int keys_size);

void mavl_remove (
		t_mavl base,
		unsigned char * keys,
		unsigned int keys_size);

t_mavl  mavl_search (
		t_mavl base,
		unsigned char * keys,
		unsigned int keys_size);

t_mavl  mavl_search_prev (
		t_mavl base,
		unsigned char * keys,
		unsigned int keys_size);

#endif

