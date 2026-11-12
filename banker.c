#include <stdio.h>
#include <stdbool.h>

typedef struct{
    size_t n, m;
    int *available;
    int *maximun;
    int *allocation;
    int *need;
    int *work;
    bool *finish;
} Bank;

typedef struct {
    char operation;
    size_t customer;
    int *resources;
} Command;