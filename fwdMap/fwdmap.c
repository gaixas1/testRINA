/*

#include "fwdmap.h"
#include <stdlib.h>
#include <stdio.h>


tr_node_t  new_tr_node (
		const unsigned char key,
		void * value) {

	tr_node_t  ret =  (tr_node_t) malloc(sizeof(struct tr_node_s));

	ret->key = key;
	ret->value = value;
	ret->h = 0;
	ret->left = NULL;
	ret->right = NULL;
	ret->down = NULL;

	return ret;
}

void release_tr_node (tr_node_t node) {zz
	free(node);
}

tr_node_t  search_tr_node (
		tr_node_t base,
		unsigned char * keys,
		unsigned int keys_size){

	if(keys_size <= 0) { return NULL; }

	tr_node_t node = search(base, keys[0]);
	int i ;

	for(i = 1; i< keys_size && node != NULL; i++) {
		node =  search(base, keys[i]);
	}
	return node;
}

tr_node_t  search_max_tr_node (
		tr_node_t base,
		unsigned char * keys,
		unsigned int keys_size){

	if(keys_size <= 0) { return NULL; }

	tr_node_t node = search(base, keys[0]);
	tr_node_t pnode = NULL;
	int i ;

	for(i = 1; i< keys_size && node != NULL; i++) {
		pnode = node;
		node =  search(base, keys[i]);
	}
	if(node) { return node; }
	return pnode;
}

tr_node_t  insert_tr_node (
		tr_node_t base,
		void * value,
		unsigned char * keys,
		unsigned int keys_size){

	if(keys_size <= 0) { return NULL; }

	int i ;

	tr_node_t node = search_or_insert(base, keys[0], (keys_size == 1)? value : NULL);
	tr_node_t tnode;

	for(i = 1; i< keys_size; i++) {
		tnode = node;
		node = search_or_insert(node, keys[i], (keys_size == i+1)? value : NULL);
	}
	return node;
}


tr_node_t remove_tr_node (
		tr_node_t base,
		unsigned char * keys,
		unsigned int keys_size) {
	if(keys_size <= 0) { return NULL; }

	if(keys_size == 1) { return remove_node(base, keys[0]); }

	tr_node_t ret = remove_tr_node(base, keys+1, keys_size-1);

	if(!base->down) { remove_node(base, keys[0]); }

	return ret;
}



tr_node_t  search(tr_node_t  base, const unsigned char key) {
	tr_node_t  n = base->down;
	while(n != NULL) {
		if(key == n->key) { return n; }
		if(key < n->key) { n = n->left; }
		else {n = n->right; }
	}


	return NULL;
}
char balance_factor(tr_node_t  node) {
	char r = 0;
	if(node->left) { r  += node->left->h;  }
	if(node->right) { r -= node->right->h; }
	return r;
}

char compute_h(tr_node_t  node) {
	char h = 0;
	if(node->left) { h  = node->left->h + 1;  }
	if(node->right && node->right->h >= h) { h = node->right->h + 1; }
	return h;
}

tr_node_t rotate_leftleft( tr_node_t node ) {
 	tr_node_t a = node;
	tr_node_t b = a->left;

	a->left = b->right;
	b->right = a;

	a->h = compute_h(a);
	b->h = compute_h(b);

	return( b );
}

tr_node_t rotate_leftright( tr_node_t node ) {
	tr_node_t a = node;
	tr_node_t b = a->left;
	tr_node_t c = b->right;

	a->left = c->right;
	b->right = c->left;
	c->left = b;
	c->right = a;

	a->h = compute_h(a);
	b->h = compute_h(b);
	c->h = compute_h(c);
	return( c );
}

tr_node_t rotate_rightleft( tr_node_t node ) {
	tr_node_t a = node;
	tr_node_t b = a->right;
	tr_node_t c = b->left;

	a->right = c->left;
	b->left = c->right;
	c->right = b;
	c->left = a;

	a->h = compute_h(a);
	b->h = compute_h(b);
	c->h = compute_h(c);

	return( c );
}

tr_node_t rotate_rightright( tr_node_t node ) {
	tr_node_t a = node;
	tr_node_t b = a->right;

	a->right = b->left;
	b->left = a;

	a->h = compute_h(a);
	b->h = compute_h(b);

	return( b );
}


tr_node_t  balance(tr_node_t  node) {
	tr_node_t ret = node;

	char bf = balance_factor( node );

	if(bf > 1 || bf < 1) {
		if( node->left )
			node->left  = balance( node->left  );
		if( node->right )
			node->right = balance( node->right );

		bf = balance_factor( node );

		if( bf >= 2 ) {
			if( balance_factor( node->left ) <= -1 )
				ret = rotate_leftright( node );
			else
				ret = rotate_leftleft( node );
		} else if( bf <= -2 ) {
			if( balance_factor( node->right ) >= 1 )
				ret = rotate_rightleft( node );
			else
				ret = rotate_rightright( node );

		}
	}

	return( ret );
}

tr_node_t  search_or_insert(
		tr_node_t base,
		const unsigned char key,
		void * value) {

//	printf("%d.", key);

	if(base->down == NULL) {
		base->down = new_tr_node(key, value);
		return base->down;
	}

	tr_node_t n = base->down;
	tr_node_t m = n;
	while(n != NULL) {
		m = n;
		if(key == n->key) { return n; }
		if(key < n->key) { n = n->left; }
		else {n = n->right; }
	}

	n = new_tr_node(key, value);

	if(key < m->key) { m->left = n; }
	else {m->right = n; }

	base->down = balance(base->down);

	return n;
}


tr_node_t  remove_node(
		tr_node_t base,
		const unsigned char key) {
	if(base->down == NULL) { return NULL; }

	if(base->down->key == key) {

	}


	char found_at = 0;

	tr_node_t n = base->down;
	tr_node_t m = n;
	while(n != NULL) {
		m = n;
		if(key == n->key) { return n; }
		if(key < n->key) {
			n = n->left;
			found_at = -1;
		}
		else {
			n = n->right;
			found_at = +1;
		}
	}

	n = new_tr_node(key, value);

	if(key < m->key) { m->left = n; }
	else {m->right = n; }

	base->down = balance(base->down);
	return NULL;
}
*/
