#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "quicksort.h"
#include <iostream>

//void* labeling(void*);
void quicksort_main(int,int,int,int,int,int);
void* quicksort(void*);
int initialize_arrays(int, int);
void labeling(int, int, int, int);
void prefix_sum(int start, int end, int total_start, int size, int barr_id);
void partition(int start, int end, int total_start, int total_end, int pivot);
int cmpfunc(const void*, const void*);
//int prefix_sum(int,int);
//void partition(int,int,int,int);
//int cmpfunc(const void*, const void*);

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

int cmpfunc (const void * a, const void * b) //what is it returning?
{
   return ( *(int*)a - *(int*)b ); //What is a and b?
}

int main(int argc, char** argv) {
    /*
    arr = (int*)calloc(100, sizeof(int));
    */
    srand(10);
    //fast_srand(10);

    int num_elements = 10;
    int num_threads = 1;
    initialize_arrays(num_elements, num_threads);

    
    printf("Pre-sorting\n");
    for (int i = 0; i < num_elements; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n------------------------\n");

    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    //if (num_threads == 1) {
      //  serialquicksort(0, num_elements);
    //} else {
        quicksort_main(0, num_elements, 0, num_threads, num_elements, num_threads);
    //}

    gettimeofday(&end, NULL); 

    double time_taken; 
  
    time_taken = (end.tv_sec - start.tv_sec) * 1e6; 
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6; 
  
    std::cout << "Time taken by program is : " << std::fixed << time_taken; 
    std::cout << " sec" << std::endl; 

    
    printf("Post-sorting\n");
    for (int i = 0; i < num_elements; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    /*
    printf("lessthan\n");
    for (int i = 0; i < num_elements; i++) {
        printf("%d ", lessthan[i]);
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

void quicksort_main(int start, int end, int proc_start, int proc_end, int size, int proc_size) {
    if (pthread_barrier_init(&barr[proc_start], NULL, proc_size)) {
        printf("Could not make barrier\n");
    }

    for (int i = proc_start; i < proc_end; i++) {
        info[i].pivot = arr[start];
        info[i].num_threads = proc_size;
        info[i].barr_id = proc_start;
        info[i].total_start = start;
        info[i].total_end = end;
        info[i].size = size;
        info[i].proc_start = proc_start;
        info[i].proc_end = proc_end;

        pthread_create(&tid[i], NULL, quicksort, &info[i]);
    }

    for (int i = proc_start; i < proc_end; i++) {
		pthread_join(tid[i],NULL);
    }
}

void* quicksort(void* t_info) {
    // extract information
    thread_info* arr_info = (thread_info*) t_info;
    int thread_id = arr_info->thread_id;
    int pivot = arr_info->pivot;
    int barr_id = arr_info->barr_id;
    int total_start = arr_info->total_start;
    int total_end = arr_info->total_end;
    int num_threads = arr_info->num_threads;
    int size = arr_info->size;
    int proc_start = arr_info->proc_start;
    int proc_end = arr_info->proc_end;
 
    if (thread_id == proc_start) {
        printf("Pivot: %d\n", pivot);
    }

    // calculate the start and end for this thread
    int group_size = size / num_threads;

	int end = total_start;
    int start = 0;
    // int end = 0;
    for (int i = proc_start; i < proc_end; i++) {
        
        start = end;
        end += group_size;

        if (end > total_end) {
            end = total_end;
        }

        if (i == proc_end - 1) {
            // TODO: may need to do some other wacky array splitting stuff so that
            // it's not too unbalanced
            end = total_end;
        }

        if (i == thread_id) {
            // printf("Start: %d, end: %d\n", start, end);
            break;
        }
    }

    if (num_threads == 1) {
        qsort(&arr[start], size, sizeof(int), cmpfunc);
        pthread_barrier_destroy(&barr[barr_id]);
        return 0;
    }

    labeling(start, end, pivot, barr_id);

    prefix_sum(start, end, total_start, size, barr_id);

    partition(start, end, total_start, total_end, pivot);
    pthread_barrier_wait(&barr[barr_id]);
    memcpy(&arr[start], &aux_arr[start], (end - start) * sizeof(int));
    pthread_barrier_wait(&barr[barr_id]);
    
    if (thread_id == proc_start) {
        pthread_barrier_destroy(&barr[barr_id]);
    }

    // printf("Thread_id: %d, Start: %d, End: %d, Group_size: %d, Proc_start: %d, Proc_end: %d\n", thread_id, start, end, group_size, proc_start, proc_end);

    // begin recursion calculations

}

void labeling(int start, int end, int pivot, int barr_id) {
    // generate lessthan and morethan arrays before prefix sum
    for (int i = start; i < end; i++) {
        if (arr[i] <= pivot) {
            lessthan[i] = 1;
            morethan[i] = 0;
            
        } else {
            lessthan[i] = 0;
            morethan[i] = 1;
        }
    }
    
    pthread_barrier_wait(&barr[barr_id]);
    return;
}

void prefix_sum(int start, int end, int total_start, int size, int barr_id) {
    // ------------- PREFIX SUM ---------------------------
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
        memcpy(&lessthan[start], &aux_lessthan[start], (end - start) * sizeof(int));
        memcpy(&morethan[start], &aux_morethan[start], (end - start) * sizeof(int));

        pthread_barrier_wait(&barr[barr_id]);
    }
    return;
}

void partition(int start, int end, int total_start, int total_end, int pivot) {
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
}





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
        thread_info_constructor((thread_info*)&info[i],i,0,0,0,0,-1,0,0,0,0,0);
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