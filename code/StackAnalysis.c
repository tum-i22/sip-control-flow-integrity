// Requires openSSL development package. Install with apt-get install libssl-dev
#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <string.h>
#define DEBUG 0

typedef struct node {
	char *value;
	struct node *next;
} node_t;

node_t *stack = NULL;
int stack_len = 0;

int built_matrix = 0;
char **mapping;
char **adj_mat;
int vertices_count;

void registerFunction(char functionName[]);
void deregisterFunction(char functionName[]);

void registerFunction(char functionName[]) {
	node_t *next = stack;
	while(next != NULL) {
		// if(next->value == functionName) {
		// 	return;
		// }
		next = next->next;
	}

	stack_len++;
	node_t *new_node = (node_t*)malloc(sizeof(node_t));
	new_node->value = functionName;
	new_node->next = stack;
	stack = new_node;
	if(DEBUG) {
		printf("Stack: ");
		next = stack;
		while(next != NULL) {
			printf("%s ; ", next->value);
			next = next->next;
		}
		printf("\n");
	}
}

void deregisterFunction(char functionName[]) {
	if (stack == NULL) {
		fprintf(stderr, "Error: No function on shadow stack that can be poped!\n");
		return;
	}
	if(stack->value != functionName) {
		return;
	}

	stack_len--;
	node_t *next_node = stack->next;
	free(stack);
	stack = next_node;
	if(DEBUG) {
		node_t *next = stack;
		printf("Stack: ");
		while(next != NULL) {
			printf("%s ; ", next->value);
			next = next->next;
		}
		printf("\n");
	}
}

/*int verifyChecksum(){
	if(DEBUG) printf("Verifying checksum...\n");
	FILE *fp = fopen("graph.txt", "rb");
	if (fp == NULL){
		fprintf(stderr, "Could not open 'graph.txt'\n");
		exit(1);
	}
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha;
	SHA256_Init(&sha);

	char buf[1024];
	int r;

	while((r = fread(buf, 1, sizeof(buf), fp)) > 0){
		SHA256_Update(&sha, buf, r);
	}
	SHA256_Final(hash, &sha);
	if(DEBUG) printf("Building checksum...\n");
	char actualHash[65];
	for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(actualHash + (i * 2), "%02x", hash[i]);
    }
    actualHash[64] = 0;

	char *expectedHash = "123";
	if(DEBUG) printf("Returning...\n");
	return (strcmp(expectedHash, actualHash) == 0);
}*/

/**
 * Binary search: O(log(n)), only works for sorted list!
 */
int binarySearch(char ***list, char *str, int len) {
	if(DEBUG) printf("BinarySearch\n");
	int start = 0, end = len;
	int pos;
	while(end >= start) {
		pos = start + ((end-start) / 2);
		if(strcmp((*list)[pos], str) == 0) {
			if(DEBUG) printf("Found\n");
			return pos;
		} else if(start == end) {
			if(DEBUG) printf("Found nothing\n");
			return -1;
		} else if(strcmp((*list)[pos], str) < 0) {
			start = pos+1;
		} else {
			end = pos;
		}
	}
	if(DEBUG) printf("Found nothing\n");
	return -1;
}

int stringcmp(const void *a, const void *b) {
    const char **ia = (const char **)a;
    const char **ib = (const char **)b;
    return strcmp(*ia, *ib);
}

/*
* Reads known edges from file 'X.txt' line by line.
* Returns pointer to array of known edges and number of edges read.
*/
void readEdges(char ***mapping, char ***adj_mat, int *vertices_count){
	if(DEBUG) printf("Reading edges...\n");

	char * toks;
	int next = 0;

	char graph_text[] = "123";
	*vertices_count = 123;
	int line_count = 123;

	char *cur_line = graph_text;

	// alloc func buffer
	char ** buffer = (char **)malloc(2 * line_count * sizeof(char *));
	if (buffer == NULL){
		fprintf(stderr, "Failed to alloc buffer.\n");
		exit(1);
	}
	if(DEBUG) printf("Allocated buffer\n");

	// Alloc mapping
	*mapping = (char **)malloc(*vertices_count * sizeof(char *));
	if (*mapping == NULL){
		fprintf(stderr, "Failed to alloc mapping.\n");
		exit(1);
	}
	if(DEBUG) printf("Allocated mapping\n");

	// Alloc adjacency matrix
	*adj_mat = (char **)malloc(*vertices_count * sizeof(char *));
	if (*adj_mat == NULL){
		fprintf(stderr, "Failed to alloc adjacency matrix.\n");
		exit(1);
	}
	for(int i = 0 ; i < *vertices_count ; i++){
		(*adj_mat)[i] = (char *)malloc(*vertices_count * sizeof(char));
		if ((*adj_mat)[i] == NULL){
			fprintf(stderr, "Failed to alloc adj_mat[%d].\n", i);
			exit(1);
		}
	}
	if(DEBUG) printf("Allocated adj_mat\n");

	int count = 0;
	while(cur_line){
		char *next_line = strchr(cur_line, '\n');

		if (next_line) *next_line = '\0';
		else break;

		toks = strtok(cur_line, " ");

		while(toks != NULL){
			buffer[count++] = toks;

			int found = binarySearch(mapping, toks, next-1);
			if (found == -1){
				int len = strlen(toks);
				(*mapping)[next] = (char *) malloc(len * sizeof(char));
				strncpy((*mapping)[next++], toks, len);
				qsort(*mapping, next, sizeof(char *), stringcmp);
			}
			toks = strtok(NULL, " ");
			//if(toks != NULL)
			//	toks[strcspn(toks, "\n")] = 0;
		}//while(toks != NULL);
	}

	if(DEBUG) {
		for (int i = 0 ; i < *vertices_count ; i++) {
			printf("List[%d]=%s\n", i, (*mapping)[i]);
		}
	}

	for (int i = 0; i < 2*line_count; i+=2){
		int row = binarySearch(mapping, buffer[i], *vertices_count);
		int col = binarySearch(mapping, buffer[i+1], *vertices_count);
		if (row == -1 || col == -1){
			if(DEBUG) printf("Unexpected row/col for %s -> %s\n", buffer[i], buffer[i+1]);
		}
		(*adj_mat)[row][col] = 1;
	}

	free(buffer);
	return;
}

void response(){
	printf("Response mechanism.\n");
	exit(1);
}

void verify(char ***mapping, char ***adj_mat, int vertices_count) {
	node_t *curr = stack, *next;
	char *curr_name;
	char *next_name;

	if(curr == NULL || curr->next == NULL) {
		return;
	}
	char *func = stack->value;
	next = curr->next;

	do {
		curr_name = curr->value;
		next_name = next->value;

		if(DEBUG) printf("Checking edge: %s -> %s\n", next_name, curr_name);

		int col = binarySearch(mapping, curr_name, vertices_count);
		int row = binarySearch(mapping, next_name, vertices_count);

		if( row == -1) {
			printf("Could not find function %s. => Invalid call to sensitive function!\n", curr_name);
			response();
			return;
		}
		if ( col == -1){
			printf("Could not find function %s. => Invalid call to sensitive function!\n", next_name);
			response();
			return;
		}
		if((*adj_mat)[row][col] != 1){
			printf("Error row %d, col %d. => Invalid call to sensitive function!\n", row, col);
			response();
			return;
		}
		curr = next;
		next = curr->next;
	} while(next != NULL);

	if(DEBUG) printf("Valid access to sensitive function '%s'! :-)\n", func);
}

void verifyStack() {
	if(!built_matrix){
		/*if (!verifyChecksum()){
			printf("Wrong hash\n");
			response();
			return;
		}*/
		readEdges(&mapping, &adj_mat, &vertices_count);
		built_matrix = 1;
		if(DEBUG) {
			for(int j = 0 ; j < vertices_count ; j ++){
				for(int k = 0 ; k < vertices_count; k++){
					printf("%d ", adj_mat[j][k]);
				}
				printf("\n");
			}
		}
	}

	verify(&mapping, &adj_mat, vertices_count);
	
}
