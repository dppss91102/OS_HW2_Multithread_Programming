#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include "time.h"

/*mutex and condition for pthread_cond_wait*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/*data shared by the threads*/
typedef struct thread_data {
    int lenA;
    int lenB;
    int *A;
    int *B;
    int *sorted;
};

/*used to check if the threads are done*/
int done = 0;

/*convert the line to integers and store into int array*/
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
	//return number of integers in the array
	return i;
}

/*merge two sub-arrays into one sorted array*/
void Merge(int *array, int start, int mid, int end){
    int lenA = mid - start + 1;
    int lenB = end - mid;

    //allocate memory for two sub-arrays
    int *A = (int*)malloc(sizeof(int)*lenA);
    int *B = (int*)malloc(sizeof(int)*lenB);

    //initialize the sub-arrays with values
    for(int i = 0; i < lenA; i++)
        A[i] = array[start + i];
    for(int i = 0; i < lenB; i++)
        B[i] = array[mid + 1 + i];

    int i = 0;
    int j = 0;
    int index = start;
    //while: check if either is done; store the smaller one into array
    while(i < lenA && j < lenB)
        if(A[i] < B[j])
            array[index++] = A[i++];
        else
            array[index++] = B[j++];

    //store the remaining numbers into array
    while(i < lenA)
        array[index++] = A[i++];
    while(j < lenB)
        array[index++] = B[j++];
}

/*split the array into sub-arrays using different index*/
void Sort(int *array, int start, int end){
    if (start < end){
        int mid = (end + start) / 2;
        Sort(array, start, mid);
        Sort(array, mid + 1, end);
        Merge(array, start, mid, end);
    }
}

/*thread A and B for sorting two equal-sized sub-arrays*/
void merge_sort_A(void *a){
    struct thread_data *tData = (struct thread_data *)a;

    Sort(tData->A, 0, tData->lenA - 1);

    //signal thread M
    pthread_mutex_lock(&mutex);
    done++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}
void merge_sort_B(void *a){
    struct thread_data *tData = (struct thread_data *)a;

    Sort(tData->B, 0, tData->lenB - 1);

    //signal thread M
    pthread_mutex_lock(&mutex);
    done++;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}

/*thread M for merging*/
void merge_sort_M(void *a){
    struct thread_data *tData = (struct thread_data *)a;

    //check signal, if not both A and B are done, put back to wait
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
}

int main(int argc, char *argv[]) {

	FILE *finptr = fopen(argv[1], "r");
	FILE *foutptr = fopen(argv[2], "w");
	char *line = NULL;
	size_t len = 0;
	if (finptr == NULL){
		printf("read fail\n");
		exit(EXIT_FAILURE);
	}

	clock_t start, end;

    int array[10000];

	while (getline(&line, &len, finptr) != -1){
        start = clock();//starting time
        struct thread_data tData;
        pthread_t idA, idB, idM;

        //store the integers to be sorted into array
		int i = a2i(line, array);

		//allocate memory for and initialize two sub-arrays
		tData.lenA = i/2;
		tData.lenB = i-i/2;
        tData.A = (int*)malloc(sizeof(int)*tData.lenA);
        tData.B = (int*)malloc(sizeof(int)*tData.lenB);
        for(int i = 0; i < tData.lenA; i++)
            tData.A[i] = array[i];
        for(int i = 0; i < tData.lenB; i++)
            tData.B[i] = array[tData.lenA + i];

        //allocate memory for the final sorted array
        tData.sorted = (int*)malloc(sizeof(int)*i);

        //create the threads
        pthread_create(&idA, NULL, (void *)merge_sort_A, &tData);
        pthread_create(&idB, NULL, (void *)merge_sort_B, &tData);
        pthread_create(&idM, NULL, (void *)merge_sort_M, &tData);

        //wait until thread M terminates, which means all 3 threads are done
        pthread_join(idM, NULL);

        //print the result into output file
        for (int j = 0; j < i; j++) {
            fprintf(foutptr, "%d ", tData.sorted[j]);
        }
        fprintf(foutptr, "\n");

        //print the running time to screen
        end = clock();//completed time
        printf("%f ms\n", (double)(end - start));

        //back to initial state
        done = 0;
        free(tData.A);
        free(tData.B);
        free(tData.sorted);
	}

	//clean up and exit
	if (line)
		free(line);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	return EXIT_SUCCESS;
}
