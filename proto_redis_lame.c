#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SMALL_SIZE 100

const char *getsnl(char *buffer, int size){
	char *s = fgets(buffer, size, stdin);

	int pos = strlen(s) - 1;

	int i;
	if (pos >= 0)
		for(i = pos; i >=0; i--){
			if (s[i] == '\r' || s[i] == '\n')
				s[i] = '\0';
			else
				break;
		}

	return s;
}

const char *readparam(){
	char buffer[SMALL_SIZE];

	printf("Go ahead with size '$':\n");

	const char *count = getsnl(buffer, SMALL_SIZE);

	if (strlen(count) == 0){
		printf("Wrong responce\n");
		return NULL;
	}

	if (count[0] != '$'){
		printf("Expecting $\n");
		return NULL;
	}

	int size = atoi(& count[1]);

	if (size == 0){
		printf("Expecting size more than zero\n");
		return NULL;
	}

	// make room for \r\n\0
	size = size + 2 + 1;

	char *data = malloc(size);

	if (data == NULL){
		printf("Out of memory\n");
		return NULL;
	}

	printf("Go ahead with data:\n");

	return getsnl(data, size);
}

void serve(){
	char buffer[SMALL_SIZE];

	printf("Go ahead with param specification '*':\n");

	const char *count = getsnl(buffer, SMALL_SIZE);

	if (strlen(count) != 2){
		printf("Wrong responce\n");
		return;
	}

	if (count[0] != '*'){
		printf("Expecting *\n");
		return;
	}

	int numparams = atoi(& count[1]);

	if (numparams < 2 || numparams > 3){
		printf("Expecting 2 or 3 parameters\n");
		return;
	}

	const char *params[3];

	int i;
	for (i = 0; i < numparams; i++)
		params[i] = readparam();

	for (i = 0; i < numparams; i++)
		printf("%d | %s\n", i, params[i]);
}

int main(){
	for(;;){
		serve();
	}

	return 0;
}
