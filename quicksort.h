#include <stdio.h>
#include <stdlib.h>

struct thread_info {
    // int* arr;
    int thread_id;
    int start;
    int end;
    int pivot;
    int num_threads;
    int barr_id;
    int total_start;
    int total_end;
    int size;
    int proc_start;
    int proc_end;
    //int size;
};

// void thread_info_constructor(thread_info struc, int s, int e);
void thread_info_constructor(thread_info* struc, int id, int s, int e, int piv, int nt, int barr_id,int total_start, int total_end, int size,int proc_start, int proc_end) {
    // struc->arr = the_arr;
    //printf("doing something\n");
    // printf("%d\n", id);
    struc->thread_id = id;
    struc->start = s;
    struc->end = e;
    struc->pivot = piv;
    struc->num_threads = nt;
    struc->barr_id = barr_id;
    struc->total_start = total_start;
    struc->total_end = total_end;
    struc->size = size;
    struc->proc_start = proc_start;
    struc->proc_end = proc_end;

}