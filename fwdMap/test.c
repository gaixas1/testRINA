#include "multiavl.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void print_line_r(avl_n base) {
	if(!base) { return; }

	print_line_r(base->left);
	printf("%hhu ", base->key);
	print_line_r(base->right);
}

void printKeys(unsigned char* keys, unsigned char pos) {
	int i;
	for(i = 0; i <= pos; i++) {
		printf("%hhu.", keys[i]);
	}
	printf("*\n");
}

void printRec(avl_n base, unsigned char* keys, unsigned char pos) {
	if(!base) { return; }
	if(!base->value) { return; }
	t_mavl current = (t_mavl) base->value;

	printRec(base->left, keys, pos);

	keys[pos] = base->key;
	if(current->hasContent) {
		printKeys(keys, pos);
	}

	if(current->down) { printRec(current->down, keys, pos+1); }

	printRec(base->right, keys, pos);
}


int main(int argc, char *argv[]) {
	srand(time(NULL));

	t_mavl base = mavl_new_node ();
	int i, j, l;
	unsigned char keys[10];

	char c;
	unsigned char d;

	while(1) {
		scanf("%c%d", &c, &j);
		l = 0;
		for(i = 0; i<j; i++) {
			scanf("%hhu", &d);
			keys[i] = d;
		}
		if(c == 'a') { mavl_insert(base, NULL, keys, j); }
		else if(c == 'r') { mavl_remove(base, keys, j); }
		else if (c=='p') { printf("\nPrint table\n"); printRec(base->down, keys, 0); }
	 	else if (c=='e') { break; }
	}
	printf("\nPrint table\n");
	printRec(base->down, keys, 0);
	/*
	char c;
	unsigned char d;
	while(1) {
		scanf("%c%hhu", &c, &d);

		if(c == 'r') {
			printf("remove %hhu\n", c);
			base = avl_remove(base, d);

			print_line_r(base);
			printf("\n");
		} else if(c == 'a') {
			printf("add %hhu\n", d);
			base = avl_insert(base, d, NULL);

			print_line_r(base);
			printf("\n");
		}
	}

	avl_release_node(base);
	*/
}
