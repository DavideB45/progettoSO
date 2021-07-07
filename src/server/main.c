#include <stdio.h>
#include "../utils/utils.c"
#include <stdlib.h>
#include <errno.h>

// int intCompare(void* A, void* B){
// 	return *((int*) A) - *((int*) B);
// }


int main(void){
	printf("ciao\n");
	for (size_t i = 0; i < 10; i++){
		printf("%d\t", i);
		free(NULL);
	}
	printf("\n");

	int arr[10] = {12, 23, 3, 19, 19, 10, 0, 6, 54, 8};
	qsort(arr, 10, sizeof(int), intCompare);
	for (size_t i = 0; i < 10; i++){
		printf("%d ", arr[i]);
	}printf("\n");
	
	errno = EFAULT;
	perror("test");

	return 0;
}