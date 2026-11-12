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

static void read_customers(Bank *bank)
{
    const char *name = "customer.txt";
    FILE *file = open_file(name, "r");
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    while ((length = getline(&line, &capacity, file)) != -1) {
        check_line(line, length, name, bank->n + 1);
        if (bank->n >= SIZE_MAX / bank->m)
            fail("quantidade de clientes muito grande");
        size_t cells = (bank->n + 1) * bank->m;
        bank->maximum = resize(bank->maximum, cells, sizeof(int));
        const char *cursor = line;

        for (size_t j = 0; j < bank->m; ++j) {
            int *cell = &bank->maximum[bank->n * bank->m + j];
            if (!read_number(&cursor, cell))
                input_error(name, bank->n + 1, "esperado inteiro entre 0 e INT_MAX");
            cursor = skip_space(cursor);
            if (j + 1 < bank->m) {
                if (*cursor != ',')
                    input_error(name, bank->n + 1, "quantidade de recursos ou separador incorreto");
                ++cursor;
            } else if (*cursor != '\0') {
                input_error(name, bank->n + 1, "valores extras ou formato incorreto");
            }
        }
        ++bank->n;
    }

    free(line);
    close_input(file, name);
    if (bank->n == 0)
        fail("customer.txt vazio");
    size_t cells = bank->n * bank->m;
    bank->allocation = zeroed(cells, sizeof(int));
    bank->need = zeroed(cells, sizeof(int));
    memcpy(bank->need, bank->maximum, cells * sizeof(int));
    bank->work = zeroed(bank->m, sizeof(int));
    bank->finish = zeroed(bank->n, sizeof(bool));
}