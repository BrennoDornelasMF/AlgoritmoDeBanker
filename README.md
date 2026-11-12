# Algoritmo do banqueiro

Implementação em C da atividade de Infraestrutura de Software. O programa processa solicitações e liberações de recursos e concede uma solicitação somente quando o estado resultante é seguro.

A quantidade de tipos de recurso vem dos argumentos da execução. A quantidade de clientes é determinada pelas linhas de `customer.txt`. Os comandos são executados na ordem de `commands.txt`, em uma única thread.

## Arquivos do repositório

| Arquivo | Responsabilidade |
| --- | --- |
| `banker.c` | Leitura, validação, algoritmo de segurança e processamento das operações. |
| `Makefile` | Compilação do executável e limpeza. |
| `evidencias.log` | Registro dos comandos e resultados das validações. |
| `README.md` | Instruções de uso e descrição do programa. |

Os arquivos `customer.txt` e `commands.txt` são fornecidos pelo professor. Coloque-os no diretório da implementação antes de executar o programa.

## Requisitos

- Sistema POSIX, como Linux, Unix ou macOS. Os testes documentados foram executados em Linux x86_64.
- Compilador com suporte a C11, como GCC ou Clang.
- `make`.

A função `getline` é habilitada por `#define _POSIX_C_SOURCE 200809L`, definido antes dos cabeçalhos de C.

## Compilação e execução

Na pasta que contém `banker.c`, `Makefile` e os dois arquivos de entrada:

```sh
make clean
make
./banker 10 5 7
cat result.txt
```

`make` gera o executável `banker`. Cada argumento representa a quantidade inicial de um tipo de recurso. Nesse exemplo, existem três tipos, com 10, 5 e 7 unidades.

Para remover o executável:

```sh
make clean
```

As entradas, `result.txt` e o log são preservados pelo alvo `clean`.

## Entradas

### `customer.txt`

Cada linha contém a demanda máxima de um cliente, com os valores separados por vírgulas. Exemplo com cinco clientes e três recursos:

```text
7,5,3
3,2,2
9,0,2
2,2,2
4,3,3
```

Os índices começam em zero. Todas as linhas devem ter exatamente tantos valores quanto os argumentos da execução.

### `commands.txt`

| Formato | Operação |
| --- | --- |
| `RQ cliente q0 q1 ...` | Solicita recursos para o cliente. |
| `RL cliente q0 q1 ...` | Libera recursos que o cliente possui. |
| `*` | Registra o estado naquele ponto da execução. |

`RQ` e `RL` recebem um índice de cliente e uma quantidade por recurso, separados por espaços ou tabulações. A consulta `*` não recebe parâmetros e não encerra a leitura.

## Funcionamento

| Estrutura | Conteúdo |
| --- | --- |
| `available` | Recursos disponíveis de cada tipo. |
| `maximum` | Demanda máxima declarada por cliente. |
| `allocation` | Recursos atualmente alocados a cada cliente. |
| `need` | Necessidade restante: `maximum - allocation`. |

Inicialmente, `allocation` é zero e `need` é uma cópia de `maximum`. As matrizes são alocadas dinamicamente.

Uma solicitação precisa respeitar `need` e `available`. Depois dessas verificações, o programa simula a alocação e procura uma ordem na qual todos os clientes possam concluir. Se essa ordem existir, confirma a concessão. Caso contrário, restaura `available`, `allocation` e `need` e registra a recusa.

Uma liberação é validada por inteiro antes de alterar o estado. O cliente não pode devolver mais do que possui. A verificação de segurança simula conclusões; as liberações reais acontecem pelos comandos `RL`.

O teste de segurança tem custo de `O(n² × m)` no pior caso, com `n` clientes e `m` tipos de recurso.

## Saída e tratamento de erros

`result.txt` registra concessões, liberações, recusas e consultas. As mensagens de operação seguem o formato em inglês do enunciado.

- **Código de saída 0:** os comandos foram processados. Uma operação recusada é um resultado normal e os comandos seguintes continuam.
- **Código de saída 1:** ocorreu erro de argumento, arquivo, formatação, memória ou escrita. A mensagem é enviada para `stderr`.

São rejeitados arquivos ausentes ou sem permissão de leitura, arquivos vazios, linhas vazias, separadores incorretos, clientes inexistentes, valores negativos, números acima de `INT_MAX` e quantidades de valores incompatíveis. Uma demanda máxima individual maior que os recursos totais também torna a entrada inválida.

Espaços nas extremidades, finais de linha CRLF e última linha sem quebra de linha são aceitos. Todos os comandos são validados antes de abrir `result.txt`; uma entrada inválida preserva um resultado de execução anterior, caso ele exista.



