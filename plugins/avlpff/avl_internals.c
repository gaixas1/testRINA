/*
 * AVL Implementation, internal functions
 *
 *	Sergio Leon <slgaixas@upc.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/slab.h>
#include "avl_internals.h"

avl_n avl_new_node (
		const avl_kt key,
		void * value){
	avl_n  ret =  (avl_n) kmalloc(sizeof(struct avl_node_s), GFP_KERNEL);

	ret->key = key;
	ret->value = value;
	ret->h = 0;
	ret->left = NULL;
	ret->right = NULL;
	return ret;

}

void avl_release_node (
		avl_n node) {
	kfree(node);
}

void alv_recalc_h(
		avl_n node) {
	node->h = 0;
	if(node->left) { node->h = node->left->h+1; }
	if(node->right && node->right->h >= node->h) { node->h = node->right->h+1; }
}

char avl_balance_factor(
		avl_n l,
		avl_n r) {
	char lh = l? l->h : 0;
	char rh = r? r->h : 0;
	return lh-rh;
}

avl_n avl_find_min(
		avl_n node) {
	if(node->left) { return avl_find_min(node->left); }
	return node;
}

avl_n avl_remove_min(
		avl_n node) {
	if(!node->left) { return node->right; }
	node->left = avl_remove_min(node->left);
	return avl_balance(node);
}

avl_n avl_rotate_right(
		avl_n p) {

    avl_n q = p -> left;
    p -> left = q -> right;
    q -> right = p;

    alv_recalc_h(p);
    alv_recalc_h(q);

    return q;
}

avl_n avl_rotate_left(
		avl_n p) {

    avl_n q = p -> right;
    p -> right = q -> left;
    q -> left = p;

    alv_recalc_h(p);
    alv_recalc_h(q);

    return q;
}

avl_n avl_balance(
		avl_n p) {

	alv_recalc_h(p);
	char bf = avl_balance_factor(p->left, p->right);

    if ( bf >= 2 ) {
    	char lbf = avl_balance_factor(p->left->left, p->left->right);
    	if(lbf < 0) { p -> left = avl_rotate_left(p -> left); }
        return avl_rotate_right(p);
    }
    if ( bf <= -2 ) {
    	char rbf = avl_balance_factor(p->right->left, p->right->right);
    	if(rbf > 0) { p -> right = avl_rotate_right(p -> right); }
        return avl_rotate_left(p);
    }

    return p;
}
