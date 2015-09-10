/*
 * AVL Implementation
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

#ifndef H_AVL
#define H_AVL

#include <stdlib.h>
#include <stdio.h>

typedef int avl_kt;

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
