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
