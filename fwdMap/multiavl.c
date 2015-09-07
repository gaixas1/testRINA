#include "multiavl.h"
#include "multiavl_internals.h"



t_mavl mavl_new_node () {
	t_mavl  ret =  (t_mavl) malloc(sizeof(struct multiavl_store_s));

	ret->value = NULL;
	ret->down = NULL;
	ret->hasContent = false;
	return ret;
}

void mavl_release_node (
		t_mavl node) {
	free(node);
}


t_mavl mavl_insert (
		t_mavl base,
		void * value,
		unsigned char * keys,
		unsigned int keys_size){
	t_mavl ret;
	if(keys_size > 0) {
		ret= mavl_insert_r(base, value, keys, keys_size);
	} else {
		base->hasContent = true;
		base->value = value;
		ret= base;
	}
	return ret;
}

void mavl_remove (
		t_mavl base,
		unsigned char * keys,
		unsigned int keys_size) {
	if(keys_size > 0) { mavl_remove_r(base, keys, keys_size); }
	else { base->hasContent = false; }
}

t_mavl  mavl_search (
		t_mavl base,
		unsigned char * keys,
		unsigned int keys_size) {
	int i ;
	for(i = 0; i< keys_size; i++) {
		if(!base->down) { return NULL; }
		avl_n t = avl_search(base->down, keys[i]);
		if(!t || t->value == NULL) { return NULL; }
		base = (t_mavl) t->value;
	}
	return base;
}

t_mavl  mavl_search_prev (
		t_mavl base,
		unsigned char * keys,
		unsigned int keys_size) {
	int i ;
	for(i = 0; i< keys_size; i++) {
		if(!base->down) { return NULL; }
		avl_n t = avl_search(base->down, keys[i]);
		if(!t || t->value == NULL) { return base; }
		base = (t_mavl) t->value;
	}
	return base;
}
