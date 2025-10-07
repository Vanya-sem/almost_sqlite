#include "types.h"

using namespace std;

typedef struct data_list_node data_node;

struct data_list_node {
	char id;
	int size;
	struct data_list_node* next;
};

data_node* init(char id, int size) {
	data_node* root = (data_node*)malloc(sizeof(data_node*));
	*root = {id, size, NULL };
	return root;
}

data_node* append(data_node* root, char id, int size) {
	if (root == NULL) root = init(id, size);
	else {
		data_node* current = root;
		while (current->next != NULL) {
			current = current->next;
		}

		data_node* new_node = (data_node*)malloc(sizeof(data_node*));
		*new_node = { id, size, NULL };
		current->next = new_node;
		return new_node;
	}
}

void destroy(data_node* root) {
	if (root != NULL) {
		data_node* next = root->next;
		while (next != NULL) {
			free(root);
			root = next;
			next = root->next;
		}
		free(root);
	}
}

int get_type_size(All_types colomn, int size=-1) {
	switch (colomn) {
	case 0: return 1;
	case 1: return 1;
	case 2: return 2;
	case 3: return 4;
	case 4: return 8;
	case 5: return 8;
	case 6: return 8;
	case 7: return 8;
	case 8: return 4;
	case 9: return 3;
	case 10: return 5;
	case 11: if (size <= 254 && size != -1) return size + 2; else return 256;
	case 12: if (size <= 254 && size != -1) return size + 2; else return 256;
	case 13: return 256;
	}
}