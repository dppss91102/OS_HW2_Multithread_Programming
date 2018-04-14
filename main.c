#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
typedef struct thread_data {
    int lenA;
    int lenB;
    int *A;
    int *B;
    int *sorted;
};
int done = 0;

int a2i (char *s, int *array){
	int i = 0;
	while (*s != '\n'){
	    int sign = 1;
	    if (*s == '-'){
	        sign = -1;
	        s++;
	    }

	    int num = 0;
	    while (*s <= '9' && *s >= '0'){
	        num = ((*s) - '0') + num*10;
	        s++;
	    }

	    array[i++] = num*sign;
	    s++;
	}
	return i;
}

void Merge(int *array, int start, int mid, int end){
    int lenA = mid - start + 1;
    int lenB = end - mid;
    int *A = (int*)malloc(sizeof(int)*lenA);
    int *B = (int*)malloc(sizeof(int)*lenB);

    for(int i = 0; i < lenA; i++)
        A[i] = array[start + i];
    for(int i = 0; i < lenB; i++)
        B[i] = array[mid + 1 + i];

    int i = 0;
    int j = 0;
    int index = start;
    while(i < lenA && j < lenB)
        if(A[i] < B[j])
            array[index++] = A[i++];
        else
            array[index++] = B[j++];
    while(i < lenA)
        array[index++] = A[i++];

    while(j < lenB)
        array[index++] = B[j++];
}

void Sort(int *array, int start, int end){
    if (start < end){
        int mid = (end + start) / 2;
        Sort(array, start, mid);
        Sort(array, mid + 1, end);
        Merge(array, start, mid, end);
    }
}

void merge_sort_A(void *a){
    struct thread_data *tData = (struct thread_data *)a;

    Sort(tData->A, 0, tData->lenA - 1);
    pthread_mutex_lock(&mutex);
    done++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}
void merge_sort_B(void *a){
    struct thread_data *tData = (struct thread_data *)a;

    Sort(tData->B, 0, tData->lenB - 1);
    pthread_mutex_lock(&mutex);
    done++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}
void merge_sort_M(void *a){
    struct thread_data *tData = (struct thread_data *)a;

    pthread_mutex_lock(&mutex);

    while (done < 2)
        pthread_cond_wait(&cond, &mutex);

    pthread_mutex_unlock(&mutex);

    int i = 0;
    int j = 0;
    int index = 0;
    while(i < tData->lenA && j < tData->lenB)
        if(tData->A[i] < tData->B[j])
            tData->sorted[index++] = tData->A[i++];
        else
            tData->sorted[index++] = tData->B[j++];
    while(i < tData->lenA)
        tData->sorted[index++] = tData->A[i++];
    while(j < tData->lenB)
        tData->sorted[index++] = tData->B[j++];

    for (int j = 0; j < index; j++) {
        printf("%d ", tData->sorted[j]);
    }
    printf("\n");

    done++;
}

int main(int argc, char *argv[]) {

	FILE *fptr = fopen(argv[1], "r");
	char *line = NULL;
	size_t len = 0;
	if (fptr == NULL){
		printf("read fail\n");
		exit(EXIT_FAILURE);
	}

    int array[10000];
	while (getline(&line, &len, fptr) != -1){
        struct thread_data tData;
        pthread_t idA, idB, idM;
		int i = a2i(line, array);
		tData.lenA = i/2;
		tData.lenB = i-i/2;
        tData.A = (int*)malloc(sizeof(int)*tData.lenA);
        tData.B = (int*)malloc(sizeof(int)*tData.lenB);
        for(int i = 0; i < tData.lenA; i++)
            tData.A[i] = array[i];
        for(int i = 0; i < tData.lenB; i++)
            tData.B[i] = array[tData.lenA + i];

        tData.sorted = (int*)malloc(sizeof(int)*i);
        pthread_create(&idA, NULL, (void *)merge_sort_A, &tData);
        pthread_create(&idB, NULL, (void *)merge_sort_B, &tData);
        pthread_create(&idM, NULL, (void *)merge_sort_M, &tData);

        pthread_join(idM, NULL);

        done = 0;
        free(tData.A);
        free(tData.B);
        free(tData.sorted);
	}
	if (line)
		free(line);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	return EXIT_SUCCESS;
}
