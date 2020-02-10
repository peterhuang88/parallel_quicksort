#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "quicksort.h"
#include <iostream>

#define NUM_THREADS 10

inline void fast_srand(int);
inline int fastrand();
void* sample_func(void *i);
int initialize_arrays(int, int);
void* labeling(void*);
void quicksort(int,int,int,int,int,int);
int prefix_sum(int,int);
void partition(int,int,int,int);
int cmpfunc(const void*, const void*);

int partition2(int,int);
void swap(int*, int*);
void serialquicksort(int, int);

// pthread_t tid[NUM_THREADS];
pthread_t* tid;    // thread pool
thread_info* info;  // thread info array
pthread_barrier_t* barr;
pthread_mutex_t* mu;

int* arr;          // holds the array we need to sort
int* aux_arr;      // auxiliary array for swapping stuff
int* lessthan;     // less than array for prefix sum
int* morethan;     // more than array for prefix sum
int* aux_lessthan;
int* aux_morethan;
int* counter_arr; 

// int count = -1;
// pthread_mutex_t mu;
static unsigned int myg_seed;

int cmpfunc (const void * a, const void * b) //what is it returning?
{
   return ( *(int*)a - *(int*)b ); //What is a and b?
}
inline void fast_srand(int seed) {
    myg_seed = seed;
}
inline int fastrand() {
    myg_seed = (214013*myg_seed+2531011);
    return (myg_seed>>16)&0x7FFF;
}
int main(int argc, char** argv) {
    /*
    arr = (int*)calloc(100, sizeof(int));
    */
    srand(10);
    //fast_srand(10);

    int num_elements = 10000000;
    int num_threads = 16;
    initialize_arrays(num_elements, num_threads);

    /*
    printf("Pre-sorting\n");
    for (int i = 0; i < num_elements; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n------------------------\n");*/
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    if (num_threads == 1) {
        serialquicksort(0, num_elements);
    } else {
        quicksort(0, num_elements, 0, num_threads, num_elements, num_threads);
    }

    gettimeofday(&end, NULL); 

    double time_taken; 
  
    time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
  
    std::cout << "Time taken by program is : " << std::fixed << time_taken; 
    std::cout << " sec" << std::endl; 

    /*
    printf("Post-sorting\n");
    for (int i = 0; i < num_elements; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");*/
    

    int not_sorted = 0;

    for (int i = 1; i < num_elements; i++) {
        if (arr[i] < arr[i-1]) {
            not_sorted = 1;
        }
    }

    if (not_sorted) {
        printf("not sorted!\n");
    } else {
        printf("sorted!\n");
    }

    free(tid);
    free(info);
    free(barr);
    free(mu);
    free(arr);
    free(aux_arr);
    free(lessthan);
    free(morethan);
    free(aux_morethan);
    free(aux_lessthan);
    free(counter_arr);

    return EXIT_SUCCESS;
}

void quicksort(int start, int end, int proc_start, int proc_end, int size, int proc_size) {
    // calculate size of each group
    if (proc_size == 1) {
        // printf("i should qsort from %d to %d\n", start, end);
        // printf("Start: %d, end: %d, size: %d, proc_start: %d, proc_end: %d, proc_size: %d\n", start,end,size,proc_start, proc_end, proc_size);
        qsort(&arr[start], size, sizeof(int), cmpfunc);
        return;
    }
    struct timeval timstart, timend;
    double time_taken;
    /*
    printf("Before threading\n");
    for (int i = start; i < end; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");*/

    // calculate size for each group
	int group_size = size / proc_size;
	int excess = size % proc_size;

	// determine pivot
    // TODO: change to random thing later
	// int pivot = arr[start];
    //int pivot = arr[start + rand() % ((end + 1) - start)];
    // printf("Start: %d, end: %d, size: %d, proc_start: %d, proc_end: %d, proc_size: %d\n", start,end,size,proc_start, proc_end, proc_size);
    
    
    int pivot = arr[start + (((end) - start)/2)];
    
    // int pivot = arr[start + fastrand() % ((end + 1) - start)];

    //printf("pivot ind: %d, pivot: %d\n", start + ((end - start)/2), pivot);
    gettimeofday(&timstart, NULL);

    if (pthread_barrier_init(&barr[proc_start], NULL, proc_size)) {
        printf("Could not make barrier\n");
    }

    

    counter_arr[proc_start] = 0;
	// take care of excess elements
	int last_ind = start;
    for (int i = proc_start; i < proc_end; i++) {
		// set start and end for each pair of start and ends, and pivot
		info[i].start = last_ind;
		last_ind += group_size;

        // check to see if we overshoot the end of my array
        if (last_ind > end) {
            last_ind = end;
        }
        // check to see if we're on the last thread allocated
        // if so, give the rest of the array to this thread
        if (i == proc_end - 1) {
            // TODO: may need to do some other wacky array splitting stuff so that
            // it's not too unbalanced
            last_ind = end;
        } 
		info[i].end = last_ind;
		info[i].pivot = pivot;
        info[i].num_threads = proc_size;
        info[i].barr_id = proc_start;
        info[i].total_start = start;
        info[i].total_end = end;
        info[i].size = size;

		pthread_create(&tid[i], NULL, &labeling, &info[i]);
    }

	for (int i = proc_start; i < proc_end; i++) {
		pthread_join(tid[i],NULL);
    }

    pthread_barrier_destroy(&barr[proc_start]);
    /*
    gettimeofday(&timend, NULL); 
  
    time_taken = (timend.tv_sec - timstart.tv_sec) * 1e6; 
    time_taken = (time_taken + (timend.tv_usec - timstart.tv_usec)) * 1e-6; 
  
    std::cout << "Time taken by main is : " << std::fixed << time_taken; 
    std::cout << " sec" << std::endl;*/
	// do magic with prefix_sum
    //int counter = prefix_sum(start, end);
    /*
    int counter = 0;
    for (int i = start; i < end; i++) {
        if (arr[i] <= pivot) {
            counter++;
        }
    }
    */
    
    

    // TODO: DEBUG
    /*
    printf("Less than after join\n");
    for (int i = start; i < end; i++) {
        printf("%d ", lessthan[i]);
    }
    printf("\n");
    printf("More than after join\n");
    for (int i = start; i < end; i++) {
        printf("%d ", morethan[i]);
    }
    printf("\n");  */
    
    
    //printf("%d\n", counter_arr[proc_start]);
    // int counter = counter_arr[proc_start];
    int counter = lessthan[end-1];
    /*printf("Counter: %d\n", counter);

    printf("Arr after junk\n");
    for (int i = start; i < end; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");*/
    
    // partition into aux_arr
    //partition(start, end, counter, pivot);

    // TODO: DEBUG
    /*
    for (int i = start; i < end; i++) {
        printf("%d ", aux_arr[i]);
    }
    printf("\n");
    */
    //int new_size = counter - start;
    // COPY ELEMENTS AFTER PARTITION
    /*
    for (int i = start; i < end; i++) {
        arr[i] = aux_arr[i];
    }
    */
    /*
    printf("After partition\n");
    for (int i = start; i < end; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");*/
    //printf("mut:%d\n",counter_arr[proc_start]);
    //printf("lessthan:%d\n",lessthan[end-1]);

    
    

    int proc_mid = proc_start + (proc_size / 2);
    //float percent = ((float) counter / (float) size);
    //int proc_mid = proc_start + (int)(percent * proc_size);
    /*
    if (proc_size == 2) {
        proc_mid = proc_start + 1;
    }*/
    //printf("Procstart: %d, proc_mid: %d, proc_end: %d\n", proc_start, proc_mid, proc_end);
    
    //printf("-----------------------RECURSION------------------------------------\n");
    // printf("proc_mid: %d, counter: %d\n", proc_mid,counter);
    //printf("-----------------------QUICKSORT 1------------------------------------\n");
    quicksort(start, start + counter, proc_start, proc_mid, counter, proc_mid - proc_start);
    //printf("-----------------------QUICKSORT 2------------------------------------\n");
    quicksort(counter, end, proc_mid, proc_end, end - counter, proc_end - proc_mid);
}

void* labeling(void* t_info) {
    
    struct timeval timstart, timend;
    double time_taken; 

    


    thread_info* arr_info = (thread_info*) t_info;
    int start = arr_info->start;
    int end = arr_info->end;
    int pivot = arr_info->pivot;
    int id = arr_info->thread_id;
    int num_threads = arr_info->num_threads;
    int barr_id = arr_info->barr_id;
    int total_start = arr_info->total_start;
    int total_end = arr_info->total_end;
    int size = arr_info->size;
    // printf("Num threads: %d\n", num_threads);
    // printf("Thread %d has start: %d and end: %d and pivot: %d\n", id, start, end, pivot);

    // initialize barrier
    /*
    if (pthread_barrier_init(&barr[barr_id], NULL, num_threads)) {
        printf("Could not make barrier\n");
    }*/
    

    for (int i = start; i < end; i++) {
        //printf("%d ", arr[i]);
        if (arr[i] <= pivot) {
            //pthread_mutex_lock(&mu[barr_id]);
            //counter_arr[barr_id]++;
            //pthread_mutex_unlock(&mu[barr_id]);
            lessthan[i] = 1;
            morethan[i] = 0;
            
        } else {
            // pthread_mutex_lock(&mu[barr_id]);
            lessthan[i] = 0;
            morethan[i] = 1;
            // pthread_mutex_unlock(&mu[barr_id]);
        }
    }
    
    
    
    /*
    printf("TID: %d, Less than\n", id);
    for (int i = start; i < end; i++) {
        printf("%d ", lessthan[i]);
    }
    printf("\n");*/
    
    // printf("\n");
    // wait on completion of labeling
    
    
    
    pthread_barrier_wait(&barr[barr_id]);

    //gettimeofday(&timstart, NULL);

    // prefix sum based on https://en.wikipedia.org/wiki/Prefix_sum algo 1
    int bound = ceil(log2(size));
    // printf("Bound: %d\n", bound);
    int pow1;
    for (int i = 0; i < bound; i++) {
        pow1 = pow(2, i);
        // int k = 0;
        for (int j = start; j < end; j++) {
            // if (j < pow1) {
            if (j - pow1 < total_start) {
                aux_lessthan[j] = lessthan[j];
                aux_morethan[j] = morethan[j];
            } else {
                aux_lessthan[j] = lessthan[j] + lessthan[j-pow1];
                aux_morethan[j] = morethan[j] + morethan[j-pow1];
            }
            
        }
        pthread_barrier_wait(&barr[barr_id]);
        /*
        for (int j = start; j < end; j++) {
            lessthan[j] = aux_lessthan[j];
            morethan[j] = aux_morethan[j];
        }*/
        memcpy(&lessthan[start], &aux_lessthan[start], (end - start) * sizeof(int));
        memcpy(&morethan[start], &aux_morethan[start], (end - start) * sizeof(int));

        pthread_barrier_wait(&barr[barr_id]);
    }
    
    /*gettimeofday(&timend, NULL); 
    time_taken = (timend.tv_sec - timstart.tv_sec) * 1e6; 
    time_taken = (time_taken + (timend.tv_usec - timstart.tv_usec)) * 1e-6; 
  
    std::cout << "Time taken by labeling is : " << std::fixed << time_taken; 
    std::cout << " sec" << std::endl;*/
    
    
    
    // partition the arrays
    for (int i = start; i < end; i++) {
        if (arr[i] <= pivot) {
            aux_arr[total_start + lessthan[i]-1] = arr[i];
        } else {
            //printf("partition ind: %d\n", midpoint -1 + morethan[i]);
            //fflush(stdout);
            //aux_arr[total_start + counter_arr[barr_id] - 1 + morethan[i]] = arr[i];
            aux_arr[total_start + lessthan[total_end-1] - 1 + morethan[i]] = arr[i];
        }
    }
    

    

    pthread_barrier_wait(&barr[barr_id]);
    /*
    for (int i = start; i < end; i++) {
        arr[i] = aux_arr[i];
    }*/
    memcpy(&arr[start], &aux_arr[start], (end - start) * sizeof(int));

    
}

int prefix_sum(int start, int end) {
    int counter = 0;
    if (lessthan[start] == 1) {
        counter++;
    }
    for (int i = start+1; i < end; i++) {
        if (lessthan[i] == 1) {
            counter++;
        }
        lessthan[i] = lessthan[i-1] + lessthan[i];
        morethan[i] = morethan[i-1] + morethan[i];
    }
    return counter;
}

void partition(int start, int end, int midpoint,int pivot) {
    for (int i = start; i < end; i++) {
        if (arr[i] <= pivot) {
            aux_arr[start + lessthan[i]-1] = arr[i];
        } else {
            //printf("partition ind: %d\n", midpoint -1 + morethan[i]);
            //fflush(stdout);
            aux_arr[start + midpoint - 1 + morethan[i]] = arr[i];
        }
    }
}
/*
void* sample_func(void* p) {
    pthread_mutex_lock(&mu);
    count++;
    printf("Current job count = %d\n", count);
    printf("The value in the array at index count is %d\n", arr[count]);
    pthread_mutex_unlock(&mu);
}*/

int initialize_arrays(int num_elements, int num_threads) {
    tid = (pthread_t*) malloc(num_threads * sizeof(*tid));
    barr = (pthread_barrier_t*) malloc(num_threads * sizeof(*barr));
    // info = (thread_info*)calloc(num_threads, sizeof(thread_info));
    info = (thread_info*) malloc(num_threads * sizeof(thread_info));
    mu = (pthread_mutex_t*) malloc(num_threads * sizeof(*mu));
    arr = (int*)calloc(num_elements, sizeof(int));
    aux_arr = (int*)calloc(num_elements, sizeof(int));
    lessthan = (int*)calloc(num_elements, sizeof(int));
    morethan = (int*)calloc(num_elements, sizeof(int));
    aux_lessthan = (int*)calloc(num_elements, sizeof(int));
    aux_morethan = (int*)calloc(num_elements, sizeof(int));
    counter_arr = (int*) calloc(num_elements, sizeof(int));
    for (int i = 0; i < num_threads; i++) {
        // printf("%d\n", i);
        thread_info_constructor((thread_info*)&info[i],i,0,0,0,0,-1,0,0,0);
    }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_mutex_init(&mu[i], NULL) != 0) {
            printf("mutex init failed\n");
            exit(-1);
        }
    }

    for (int i = 0; i < num_elements; i++) {
        arr[i] = rand() % num_elements;
    }

    for (int i = 0; i < num_elements; i++) {
        aux_arr[i] = 0;
    }

    return 1;
}

int partition2 (int low, int high)  
{  
    int pivot = arr[high];
    int i = (low - 1); 
  
    for (int j = low; j <= high - 1; j++) {  
        if (arr[j] < pivot) {  
            i++;
            swap(&arr[i], &arr[j]);  
        }  
    }  
    swap(&arr[i + 1], &arr[high]);  
    return (i + 1);  
}  

void swap(int* a, int* b) {  
    int t = *a;  
    *a = *b;  
    *b = t;  
}  

void serialquicksort(int low, int high) {  
    if (low < high) {  
        int ind = partition2(low, high);  

        serialquicksort(low, ind - 1);  
        serialquicksort(ind + 1, high);  
    }  
}  
