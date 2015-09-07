#ifndef H_MULTIAVLINTERNALS
#define H_MULTIAVLINTERNALS


#include "multiavl.h"


t_mavl mavl_insert_r (
		t_mavl base,
		void * value,
		unsigned char * keys,
		unsigned int keys_size);

void mavl_remove_r (
		t_mavl base,
		unsigned char * keys,
		unsigned int keys_size);


#endif

