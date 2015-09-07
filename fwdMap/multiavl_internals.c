#include "multiavl_internals.h"

t_mavl mavl_insert_r (
		t_mavl base,
		void * value,
		unsigned char * keys,
		unsigned int keys_size){

	avl_n current_node = avl_search(base->down, keys[0]);
	if(!current_node) {
		base->down = avl_insert(base->down, keys[0], NULL, &current_node);
	}
	if(!current_node->value) { current_node->value = mavl_new_node(); }
	t_mavl current = (t_mavl) current_node->value;

	if(keys_size == 1) {
		current->hasContent = true;
		current->value = value;
		return current;
	}
	return mavl_insert_r(current, value, keys+1, keys_size-1);

}

void mavl_remove_r (
		t_mavl base,
		unsigned char * keys,
		unsigned int keys_size) {

	avl_n current_node = avl_search(base->down, keys[0]);
	if(!current_node) return;

	t_mavl current = (t_mavl) current_node->value;
	if(!current) return;


	if(keys_size == 1) {
		if(!current->down) {
			base->down = avl_remove(base->down, keys[0]);
			mavl_release_node(current);
		}
		else { current->hasContent = false; }
	} else if(current->down) {
		mavl_remove_r(current, keys+1, keys_size-1);
		if(!current->hasContent && !current->down) {
			base->down = avl_remove(base->down, keys[0]);
			mavl_release_node(current);
		}
	}

}
