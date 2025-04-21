#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>

#define MAX_PESSOAS 15
#define TROCA_SEXO_TIMEOUT 3
#define CHECK_INTERVAL_US 100000
#define CAPACIDADE_BANHEIRO 4

typedef enum
{
    MALE,
    FEMALE,
    LIVRE
} Sexo;
const char *sexo_str[] = {"M", "F", "LIVRE"};

typedef enum
{
    NA_FILA,
    NO_BANHEIRO,
    SAINDO,
    FORA
} EstadoPessoa;

typedef struct
{
    int id;
    Sexo sexo;
    EstadoPessoa estado;
    pthread_cond_t cond;
} Pessoa;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int banheiro_ocupado = 0;
Sexo sexo_atual = LIVRE;
int tempo_troca_ativo = 0;
int bloqueio_entrada = 0;
time_t tempo_inicio_troca;
Pessoa pessoas[MAX_PESSOAS];
int banheiro[CAPACIDADE_BANHEIRO] = {-1, -1, -1};
sem_t sem_estados;

int fila_male[MAX_PESSOAS], fila_female[MAX_PESSOAS];
int inicio_male = 0, fim_male = 0;
int inicio_female = 0, fim_female = 0;

void enfileirar(Sexo sexo, int id)
{
    if (sexo == MALE)
        fila_male[fim_male++] = id;
    else
        fila_female[fim_female++] = id;
}

void inicializar_aleatorio() {
    srand(time(NULL));
}

int numero_aleatorio_1_3() {
    return (rand() % 3) + 1;
}

int tamanho_fila(Sexo sexo)
{
    return sexo == MALE ? fim_male - inicio_male : fim_female - inicio_female;
}

void imprimeVisualizacao()
{
    int i, qtosNaFila = 0, qtosSaindo = 0;
    int pessoasFila[MAX_PESSOAS], pessoasSaindo[MAX_PESSOAS];
    char buffer[200]; // Para centralizar título

    // Contabiliza pessoas na fila e saindo (somente se id > 0)
    for (i = 0; i < MAX_PESSOAS; i++)
    {
        if (pessoas[i].id > 0)
        {
            if (pessoas[i].estado == NA_FILA)
            {
                pessoasFila[qtosNaFila++] = i;
            }
            else if (pessoas[i].estado == SAINDO)
            {
                pessoasSaindo[qtosSaindo++] = i;
            }
        }
    }

    /**************************************************************************
     **************************** TÍTULO CENTRALIZADO *************************
     **************************************************************************/

    sprintf(buffer, "BANHEIRO UNISSEX [%s] (%d/%d)", sexo_str[sexo_atual], banheiro_ocupado, CAPACIDADE_BANHEIRO);
    int total_largura = 6 * qtosNaFila + 2 + 6 * CAPACIDADE_BANHEIRO + 2 + 6 * qtosSaindo;
    int padding = (total_largura - strlen(buffer)) / 2;
    for (i = 0; i < padding; i++)
        printf(" ");
    printf("%s\n", buffer);

    /**************************************************************************
     ************************* PRIMEIRA LINHA (TOPO) **************************
     **************************************************************************/

    for (i = qtosNaFila - 1; i >= 0; i--)
    {
        printf("  O   ");
    }
    if (qtosNaFila == 0)
    {
        printf("      ");
    }

    printf(" /");

    for (i = 0; i < CAPACIDADE_BANHEIRO; i++)
    {
        if (banheiro[i] != -1)
        {
            if (i != CAPACIDADE_BANHEIRO - 1)
            printf("|=====");
            else 
            printf("|=====|");
        }
        else
        {
            if (i != CAPACIDADE_BANHEIRO - 1)
            printf("|     ");
            else
            printf("|     |");
        }
    }

    printf("\\     ");

    for (i = 0; i < qtosSaindo; i++)
    {
        printf("  O   ");
    }
    printf("\n");

    /**************************************************************************
     ************************* SEGUNDA LINHA (MEIO) **************************
     **************************************************************************/

        // Primeira parte (fila) sem deslocamento
    for (i = qtosNaFila - 1; i >= 0; i--)
    {
        printf(" %02d%c  ", pessoas[pessoasFila[i]].id,
               sexo_str[pessoas[pessoasFila[i]].sexo][0]);
    }
    if (qtosNaFila == 0)
    {
        printf("      ");
    }

    // Agora adicionamos o deslocamento apenas no trecho do banheiro em diante
    printf("  |");

    for (i = 0; i < CAPACIDADE_BANHEIRO; i++)
    {
        if (banheiro[i] != -1)
        {
            printf(" %02d%c |", banheiro[i],
                   sexo_str[pessoas[banheiro[i] - 1].sexo][0]);
        }
        else
        {
            printf("     |");
        }
    }

    printf("      "); // Espaço extra antes dos que estão saindo

    for (i = 0; i < qtosSaindo; i++)
    {
        printf(" %02d%c  ", pessoas[pessoasSaindo[i]].id,
               sexo_str[pessoas[pessoasSaindo[i]].sexo][0]);
    }
    printf("\n");

    /**************************************************************************
     ************************* TERCEIRA LINHA (BASE) **************************
     **************************************************************************/

    for (i = qtosNaFila - 1; i >= 0; i--)
    {
        printf("------");
    }
    if (qtosNaFila == 0)
    {
        printf("      ");
    }

    printf(" /");

    for (i = 0; i < CAPACIDADE_BANHEIRO; i++)
    {
        printf("------");
    }

    printf("\\     ");

    for (i = 0; i < qtosSaindo; i++)
    {
        printf("------");
    }
    printf("\n\n");
}

void iniciar_troca_forcada()
{
    if (!tempo_troca_ativo && banheiro_ocupado > 0)
    {
        tempo_troca_ativo = 1;
        tempo_inicio_troca = time(NULL);
        printf(">> INICIANDO TIMER PARA TROCA FORÇADA (Timeout: %ds) <<\n", TROCA_SEXO_TIMEOUT);
        sem_wait(&sem_estados);
        imprimeVisualizacao();
        sem_post(&sem_estados);
    }
}

void acordar_proximos(Sexo sexo_fila)
{
    int liberados = 0;
    int *fila = (sexo_fila == MALE) ? fila_male : fila_female;
    int *inicio = (sexo_fila == MALE) ? &inicio_male : &inicio_female;
    int *fim = (sexo_fila == MALE) ? &fim_male : &fim_female;

    int i = *inicio;
    while (i < *fim && liberados < CAPACIDADE_BANHEIRO - banheiro_ocupado)
    {
        int prox_id = fila[i];
        if (pessoas[prox_id - 1].estado == NA_FILA)
        {
            pthread_cond_signal(&pessoas[prox_id - 1].cond);
            liberados++;
        }
        i++;
    }
}

void processar_troca()
{
    time_t agora = time(NULL);

    // Verifica se precisa iniciar troca forçada
    if (!tempo_troca_ativo)
    {
        if (sexo_atual == MALE && tamanho_fila(FEMALE) > 0)
        {
            iniciar_troca_forcada();
        }
        else if (sexo_atual == FEMALE && tamanho_fila(MALE) > 0)
        {
            iniciar_troca_forcada();
        }
    }

    // Processa troca em andamento
    else 
    {
        // Se timeout expirou, bloqueia novas entradas
        if (!bloqueio_entrada && difftime(agora, tempo_inicio_troca) >= TROCA_SEXO_TIMEOUT)
        {
            bloqueio_entrada = 1;
            printf(">> TEMPO EXPIRADO! Banheiro agora é exclusivo para %s <<\n",
                   sexo_atual == MALE ? "FEMALES" : "MALES");
            sem_wait(&sem_estados);
            imprimeVisualizacao();
            sem_post(&sem_estados);

            // Acorda todos para saírem
            for (int i = 0; i < MAX_PESSOAS; i++)
            {
                if (pessoas[i].estado == NO_BANHEIRO)
                {
                    pthread_cond_signal(&pessoas[i].cond);
                }
            }
        }

        // Se banheiro esvaziou, conclui a troca
        if (bloqueio_entrada && banheiro_ocupado == 0)
        {
            Sexo novo_sexo = (sexo_atual == MALE) ? FEMALE : MALE;
            sexo_atual = novo_sexo;
            tempo_troca_ativo = 0;
            bloqueio_entrada = 0;

            printf(">> TROCA CONCLUÍDA! Banheiro agora é %s <<\n", sexo_str[novo_sexo]);
            sem_wait(&sem_estados);
            imprimeVisualizacao();
            sem_post(&sem_estados);

            acordar_proximos(novo_sexo);
        }
    }
}

void *verificador_de_troca(void *arg)
{
    while (1)
    {
        pthread_mutex_lock(&mtx);
        processar_troca();
        pthread_mutex_unlock(&mtx);
        usleep(CHECK_INTERVAL_US);
    }
    return NULL;
}

void usar_banheiro_now(Sexo sexo, int id)
{
    inicializar_aleatorio();
    int valor = numero_aleatorio_1_3();
    sleep(valor);
}

void sair_banheiro(Sexo sexo, int id)
{
    banheiro_ocupado--;
    for (int i = 0; i < CAPACIDADE_BANHEIRO; i++)
        if (banheiro[i] == id)
        {
            banheiro[i] = -1;
            break;
        }

    sem_wait(&sem_estados);
    pessoas[id - 1].estado = SAINDO;
    imprimeVisualizacao();
    sem_post(&sem_estados);

    acordar_proximos(sexo);
}

void *pessoa(void *arg)
{
    Sexo sexo = ((int *)arg)[0];
    int id = ((int *)arg)[1];
    free(arg);

    sem_wait(&sem_estados);
    pessoas[id - 1].id = id;
    pessoas[id - 1].sexo = sexo;
    pessoas[id - 1].estado = NA_FILA;
    pthread_cond_init(&pessoas[id - 1].cond, NULL);
    imprimeVisualizacao();
    sem_post(&sem_estados);

    pthread_mutex_lock(&mtx);
    enfileirar(sexo, id);

    while (1)
    {
        // Pode entrar se:
        // 1. Banheiro está vazio OU
        // 2. Mesmo sexo, e:
        //    a. Não está em processo de troca OU
        //    b. Está em processo de troca mas o timeout ainda não expirou
        if (banheiro_ocupado < CAPACIDADE_BANHEIRO &&
            (sexo_atual == LIVRE ||
             (sexo_atual == sexo && (!tempo_troca_ativo || !bloqueio_entrada))))
        {

            // Se banheiro estava vazio, define o sexo atual
            if (sexo_atual == LIVRE)
            {
                sexo_atual = sexo;
            }

            // Entra no banheiro
            banheiro_ocupado++;
            for (int i = 0; i < CAPACIDADE_BANHEIRO; i++)
                if (banheiro[i] == -1)
                {
                    banheiro[i] = id;
                    break;
                }

            // Atualiza estado
            sem_wait(&sem_estados);
            pessoas[id - 1].estado = NO_BANHEIRO;
            imprimeVisualizacao();
            sem_post(&sem_estados);

            pthread_mutex_unlock(&mtx);
            usar_banheiro_now(sexo, id);
            pthread_mutex_lock(&mtx);

            sair_banheiro(sexo, id);

            sem_wait(&sem_estados);
            pessoas[id - 1].estado = FORA;
            imprimeVisualizacao();
            sem_post(&sem_estados);

            pthread_mutex_unlock(&mtx);
            return NULL;
        }
        else
        {
            pthread_cond_wait(&pessoas[id - 1].cond, &mtx);
        }
    }
}

int main()
{
    pthread_t threads[MAX_PESSOAS], troca_thread;
    sem_init(&sem_estados, 0, 1);
    for (int i = 0; i < CAPACIDADE_BANHEIRO; i++)
        banheiro[i] = -1;

    pthread_create(&troca_thread, NULL, verificador_de_troca, NULL);
    srand(time(NULL));

    for (int i = 0; i < MAX_PESSOAS; i++)
    {
        pessoas[i].id = i + 1;
        pessoas[i].estado = FORA;
        Sexo sexo = rand() % 2;
        int *args = malloc(2 * sizeof(int));
        args[0] = sexo;
        args[1] = i + 1;
        pthread_create(&threads[i], NULL, pessoa, args);
        usleep(300000 + rand() % 300000);
    }

    for (int i = 0; i < MAX_PESSOAS; i++)
        pthread_join(threads[i], NULL);

    return 0;
}
