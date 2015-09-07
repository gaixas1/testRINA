/*#ifndef H_FWDMAP
#define H_FWDMAP


#ifndef NULL
#define NULL   ((void *) 0)
#endif


struct tr_node_s {
	unsigned char key;
	unsigned char h;

	void * value;

	struct tr_node_s * left;
	struct tr_node_s * right;
	struct tr_node_s * down;
};

typedef struct tr_node_s * tr_node_t;

tr_node_t new_tr_node (
		const unsigned char key,
		void * value);

void release_tr_node (tr_node_t node);

tr_node_t insert_tr_node (
		tr_node_t base,
		void * value,
		unsigned char * keys,
		unsigned int keys_size);

tr_node_t remove_tr_node (
		tr_node_t base,
		unsigned char * keys,
		unsigned int keys_size);

tr_node_t  search_tr_node (
		tr_node_t base,
		unsigned char * keys,
		unsigned int keys_size);

tr_node_t  search_max_tr_node (
		tr_node_t base,
		unsigned char * keys,
		unsigned int keys_size);

tr_node_t  search(
		tr_node_t  base,
		const unsigned char key);

tr_node_t  search_or_insert(
		tr_node_t base,
		const unsigned char key,
		void * value);

tr_node_t  remove_node(
		tr_node_t base,
		const unsigned char key);

tr_node_t  balance(
		tr_node_t  node);

tr_node_t rotate_leftleft(
		tr_node_t node );

tr_node_t rotate_leftright(
		tr_node_t node );

tr_node_t rotate_rightleft(
		tr_node_t node );

tr_node_t rotate_rightright(
		tr_node_t node );

char balance_factor(
		tr_node_t  node);

char compute_h(
		tr_node_t  node);

#endif
*/
