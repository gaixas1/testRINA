#ifndef H_AVLINTERNALS
#define H_AVLINTERNALS

#include "avl.h"

avl_n avl_new_node (
		const avl_kt key,
		void * value);

void avl_release_node (
		avl_n node);

void alv_recalc_h(
		avl_n node);

avl_n avl_balance(
		avl_n p);

char avl_balance_factor(
		avl_n l,
		avl_n r);

avl_n avl_find_min(
		avl_n node);

avl_n avl_remove_min(
		avl_n node);


avl_n avl_rotate_right(
		avl_n p);

avl_n avl_rotate_left(
		avl_n p);

#endif
