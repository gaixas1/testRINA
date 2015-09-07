#include "avl.h"
#include "avl_internals.h"

avl_n avl_search(
		avl_n base,
		const unsigned char key) {

	while(base) {
		if(key == base->key) { return base; }
		if(key < base->key) { base = base->left; }
		else if(key > base->key) { base = base->right; }
	}
	return NULL;
}

avl_n avl_insert(
		avl_n base,
		const unsigned char key,
		void * value,
		avl_n* stored) {

	if(!base) {
		*stored = avl_new_node(key, value);
		return *stored;
	}

	if(base->key == key) {
		base->value = value;
		return base;
	}

    if ( key < base-> key )
        base-> left = avl_insert(base-> left, key, value, stored);
    else if ( key > base-> key )
        base-> right = avl_insert(base-> right, key, value, stored);

    return avl_balance(base);
}

avl_n avl_remove(
		avl_n base,
		const unsigned char key) {

    if ( !base ) return NULL;

    if ( key < base -> key ) { base -> left = avl_remove(base -> left, key); }
    else if ( key > base -> key ) { base -> right = avl_remove(base -> right, key); }
    else {
        avl_n l = base -> left;
        avl_n r = base -> right;
        avl_release_node(base);

        if ( r == NULL ) { return l; }
        if ( l == NULL ) { return r; }

        avl_n m = avl_find_min(r);
        m -> right = avl_remove_min(r);
        m -> left = l;

        return avl_balance(m);
    }

    return avl_balance(base);
}

void avl_release_tree(avl_n base) {
	if(!base) { return; }
	if(base->left) { avl_release_tree(base->left); }
	if(base->right) { avl_release_tree(base->right);}
	avl_release_node(base);
}
