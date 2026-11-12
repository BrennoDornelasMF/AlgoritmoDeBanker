#define _POSIX_C_SOURCE 200809L
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
    int *maximum;
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

static Command *read_commands(const Bank *bank, size_t *count)
{
    const char *name = "commands.txt";
    FILE *file = open_file(name, "r");
    Command *commands = NULL;
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;

    while ((length = getline(&line, &capacity, file)) != -1) {
        size_t number = *count + 1;
        check_line(line, length, name, number);
        const char *cursor = skip_space(line);
        Command command = {0};

        if (*cursor == '*' && *skip_space(cursor + 1) == '\0') {
            command.operation = '*';
        } else {
            if (strncmp(cursor, "RQ", 2) == 0 && isspace((unsigned char)cursor[2]))
                command.operation = 'Q';
            else if (strncmp(cursor, "RL", 2) == 0 && isspace((unsigned char)cursor[2]))
                command.operation = 'L';
            else
                input_error(name, number, "comando esperado: RQ, RL ou *");

            cursor += 2;
            int customer;
            if (!read_number(&cursor, &customer) || (size_t)customer >= bank->n)
                input_error(name, number, "indice de cliente invalido");
            command.customer = (size_t)customer;
            command.resources = zeroed(bank->m, sizeof(int));

            for (size_t j = 0; j < bank->m; ++j) {
                if (!isspace((unsigned char)*cursor) ||
                    !read_number(&cursor, &command.resources[j]))
                    input_error(name, number, "esperado um inteiro nao negativo por recurso");
            }
            if (*skip_space(cursor) != '\0')
                input_error(name, number, "valores extras ou formato incorreto");
        }

        commands = resize(commands, number, sizeof(Command));
        commands[*count] = command;
        *count = number;
    }

    free(line);
    close_input(file, name);
    if (*count == 0)
        fail("commands.txt vazio");
    return commands;
}

static bool is_safe(Bank *bank)
{
    memcpy(bank->work, bank->available, bank->m * sizeof(int));
    memset(bank->finish, 0, bank->n * sizeof(bool));
    size_t completed = 0;

    while (completed < bank->n) {
        bool progress = false;
        for (size_t i = 0; i < bank->n; ++i) {
            if (bank->finish[i])
                continue;
            bool can_finish = true;
            for (size_t j = 0; j < bank->m; ++j) {
                if (bank->need[i * bank->m + j] > bank->work[j]) {
                    can_finish = false;
                    break;
                }
            }
            if (!can_finish)
                continue;
            for (size_t j = 0; j < bank->m; ++j)
                bank->work[j] += bank->allocation[i * bank->m + j];
            bank->finish[i] = true;
            ++completed;
            progress = true;
        }
        if (!progress)
            return false;
    }
    return true;
}

static void print_vector(FILE *output, const int *vector, size_t length)
{
    for (size_t j = 0; j < length; ++j)
        fprintf(output, "%s%d", j == 0 ? "" : " ", vector[j]);
}

static void print_state(FILE *output, const Bank *bank)
{
    fputs("MAXIMUM | ALLOCATION | NEED\n", output);
    for (size_t i = 0; i < bank->n; ++i) {
        print_vector(output, bank->maximum + i * bank->m, bank->m);
        fputs(" | ", output);
        print_vector(output, bank->allocation + i * bank->m, bank->m);
        fputs(" | ", output);
        print_vector(output, bank->need + i * bank->m, bank->m);
        fputc('\n', output);
    }
    fputs("AVAILABLE ", output);
    print_vector(output, bank->available, bank->m);
    fputc('\n', output);
}

static int deny_request(FILE *output, const Bank *bank, size_t customer,
                        const int request[], const char *reason)
{
    fprintf(output, "The customer %zu request ", customer);
    print_vector(output, request, bank->m);
    fprintf(output, " was denied because %s\n", reason);
    return -1;
}

static int request_resources(Bank *bank, size_t customer,
                             const int request[], FILE *output)
{
    size_t row = customer * bank->m;
    for (size_t j = 0; j < bank->m; ++j) {
        if (request[j] > bank->need[row + j])
            return deny_request(output, bank, customer, request,
                                "exceed its maximum allocation");
    }
    for (size_t j = 0; j < bank->m; ++j) {
        if (request[j] > bank->available[j]) {
            fputs("The resources ", output);
            print_vector(output, bank->available, bank->m);
            fprintf(output, " are not enough to customer %zu request ", customer);
            print_vector(output, request, bank->m);
            fputc('\n', output);
            return -1;
        }
    }

    for (size_t j = 0; j < bank->m; ++j) {
        bank->available[j] -= request[j];
        bank->allocation[row + j] += request[j];
        bank->need[row + j] -= request[j];
    }
    if (!is_safe(bank)) {
        for (size_t j = 0; j < bank->m; ++j) {
            bank->available[j] += request[j];
            bank->allocation[row + j] -= request[j];
            bank->need[row + j] += request[j];
        }
        return deny_request(output, bank, customer, request,
                            "result in an unsafe state");
    }

    fprintf(output, "Allocate to customer %zu the resources ", customer);
    print_vector(output, request, bank->m);
    fputc('\n', output);
    return 0;
}

static int release_resources(Bank *bank, size_t customer,
                             const int release[], FILE *output)
{
    size_t row = customer * bank->m;
    for (size_t j = 0; j < bank->m; ++j) {
        if (release[j] > bank->allocation[row + j]) {
            fprintf(output, "The customer %zu release ", customer);
            print_vector(output, release, bank->m);
            fputs(" was denied because exceed its current allocation\n", output);
            return -1;
        }
    }
    for (size_t j = 0; j < bank->m; ++j) {
        bank->available[j] += release[j];
        bank->allocation[row + j] -= release[j];
        bank->need[row + j] += release[j];
    }
    fprintf(output, "Release from customer %zu the resources ", customer);
    print_vector(output, release, bank->m);
    fputc('\n', output);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        fail("uso: ./banker quantidade_R0 [quantidade_R1 ...]");
    Bank bank = {0};
    bank.m = (size_t)(argc - 1);
    bank.available = zeroed(bank.m, sizeof(int));
    for (size_t j = 0; j < bank.m; ++j) {
        const char *cursor = argv[j + 1];
        if (!read_number(&cursor, &bank.available[j]) || *skip_space(cursor) != '\0')
            fail("cada argumento deve ser um inteiro entre 0 e INT_MAX");
    }

    read_customers(&bank);
    size_t count = 0;
    Command *commands = read_commands(&bank, &count);
    if (!is_safe(&bank))
        fail("estado inicial inseguro: uma demanda maxima excede os recursos totais");

    FILE *output = open_file("result.txt", "w");
    for (size_t i = 0; i < count; ++i) {
        const Command *command = &commands[i];
        if (command->operation == '*')
            print_state(output, &bank);
        else if (command->operation == 'Q')
            request_resources(&bank, command->customer, command->resources, output);
        else
            release_resources(&bank, command->customer, command->resources, output);
    }
    if (ferror(output))
        fail("falha ao escrever result.txt");
    if (fclose(output) != 0)
        fail("falha ao fechar result.txt");

    for (size_t i = 0; i < count; ++i)
        free(commands[i].resources);
    free(commands);
    free(bank.available);
    free(bank.maximum);
    free(bank.allocation);
    free(bank.need);
    free(bank.work);
    free(bank.finish);
    return EXIT_SUCCESS;
}
