#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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

static void fail(const char *menssage){
    fprintf(stderr, "Erro: %s. \n", menssage);
    exit(EXIT_FAILURE);
}

static void input_error(const char *file, size_t line, const char *message){
    fprintf(stderr, "Erro em %s, linha %zu: %s.\n", file, line, message);
    exit(EXIT_FAILURE);
}

static void *resize(void *pointer, size_t count, size_t width)
{
    if (width == 0 || count == 0 || count > SIZE_MAX / width)
        fail("tamanho de memoria invalido");
    void *result = realloc(pointer, count * width);
    if (result == NULL)
        fail("memoria insuficiente");
    return result;
}

static void *zeroed(size_t count, size_t width)
{
    void *result = resize(NULL, count, width);
    memset(result, 0, count * width);
    return result;
}

static FILE *open_file(const char *name, const char *mode)
{
    FILE *file = fopen(name, mode);
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir %s: %s.\n", name, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return file;
}

static const char *skip_space(const char *text)
{
    while (isspace((unsigned char)*text))
        ++text;
    return text;
}

static bool read_number(const char **cursor, int *result)
{
    const char *start = skip_space(*cursor);
    if (!isdigit((unsigned char)*start))
        return false;
    char *end;
    errno = 0;
    long value = strtol(start, &end, 10);
    if (errno == ERANGE || value > INT_MAX)
        return false;
    *result = (int)value;
    *cursor = end;
    return true;
}

static void check_line(const char *text, ssize_t length,
                       const char *name, size_t line)
{
    if (memchr(text, '\0', (size_t)length) != NULL)
        input_error(name, line, "caractere NUL dentro da linha");
}

static void close_input(FILE *file, const char *name)
{
    if (ferror(file)) {
        fprintf(stderr, "Erro ao ler %s.\n", name);
        exit(EXIT_FAILURE);
    }
    if (fclose(file) != 0) {
        fprintf(stderr, "Erro ao fechar %s.\n", name);
        exit(EXIT_FAILURE);
    }
}