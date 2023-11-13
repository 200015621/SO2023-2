#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/resource.h>

// Incluindo o header "vector.h" para facilitar a manipulação de filas
#include "vector.h"

#define PROCESS_ARRAY_SIZE 10
#define PROCESS_NAME_SIZE 50

// Definindo uma estrutura para armazenar informações sobre processos
struct process_info
{
    pid_t pid;                        // PID do processo
    time_t start_time;
    int priority;                     // Prioridade do processo
    char name[PROCESS_NAME_SIZE + 1]; // Nome do processo (limitado a PROCESS_NAME_SIZE caracteres)
};

// Variáveis globais
FILE *fptr;                      // Arquivo com as informacoes de processos
struct process_info *proc_atual; // Ponteiro para o processo atual
Vector process_array;            // Vetor para armazenar informações dos processos
Vector tickets;                  // Vetor para armazenar "tickets" usados no escalonamento

int alarm_times = 0;


// Função para criar um processo e adicionar ao vetor process_array
void create_process(struct process_info * p_info){
    pid_t pid = fork();
    if (pid == 0)
    {
        execl(p_info->name, "", NULL);
        exit(0);
    }
    else
    {
        kill(pid, SIGSTOP); // Envia um sinal para interromper a si mesmo
        p_info->pid = pid;
        time(&(p_info->start_time));
        printf("[Processo %d] Nome: %s, Prioridade %d\n", pid, p_info->name, p_info->priority);
        vector_push_back(&process_array, p_info); // Coloca o processo filho na fila de execução
    }   
}

// Função para remover um processo do vetor process_array
void remove_process(struct process_info *p_info)
{
    int i;
    for (i = 0; i < process_array.size; i++)
    {
        if (p_info->pid == ((struct process_info *)vector_get(&process_array, i))->pid)
        {
            vector_erase(&process_array, i);
            break;
        }
    }
}

// Função para escolher um processo com base em "tickets" de acordo com a prioridade
struct process_info *escolhe_processo()
{
    int i, j;
    struct process_info *p_info;

    vector_clear(&tickets);
    for (i = 0; i < process_array.size; i++)
    {
        p_info = vector_get(&process_array, i);
        for (j = 0; j <= (p_info->priority); j++) // Atribui uma quantidade de "tickets" com base na prioridade
        {
            vector_push_back(&tickets, p_info);
        }
    }
    int n = rand() % tickets.size;  // Escolhe um "ticket" aleatório
    return vector_get(&tickets, n); // Retorna o processo correspondente ao "ticket"
}

// Função para interromper o processo atual usando um sinal
void quantum_escalonador()
{
    kill(proc_atual->pid, SIGSTOP);      // Envia um sinal para parar o processo atual
    alarm(6);
}

void read_create_process(){

    struct process_info p_info;
    if( fscanf(fptr, "%50[^ ]%d\n", p_info.name, &p_info.priority) != EOF ){
        create_process(&p_info);
        alarm(2);
    }
    else {
        signal(SIGALRM, quantum_escalonador);
        alarm(6 - 2*alarm_times);
    }

    if (alarm_times++ == 2)
    {
        alarm_times = 0;
        kill(proc_atual->pid, SIGSTOP);      // Envia um sinal para parar o processo atual
    }
    
}


// Função para executar o escalonador
void executa_escalonador()
{
    int status;
    pid_t wait_status, wait_finish, wait_stop;
    struct rusage child_usage;

    while(process_array.size)
    {
        proc_atual = escolhe_processo(); // Escolhe um processo
        kill(proc_atual->pid, SIGCONT);  // Envia um sinal para continuar o processo escolhido
        printf("[Processo %d] executando\n", proc_atual->pid);

        wait_finish = wait4(proc_atual->pid, &status, WUNTRACED, &child_usage);
        if (WIFEXITED(status))
        {
            time_t finish = time(NULL);
            // printf("%ld, %ld ", proc_atual->start_time, time(NULL));
            printf("[Processo %d] Makespan: %.1lfs, Tempo de CPU: %ld.%06lds\n", proc_atual->pid, difftime(finish, proc_atual->start_time) , child_usage.ru_utime.tv_sec, child_usage.ru_utime.tv_usec);
            remove_process(proc_atual);    // remove processo se terminou
        }
    }
}

int main(int argc, char *argv[])
{
    const char *filename = "process_file";
    fptr = fopen(filename, "r");
    srand(time(NULL));
    vector_setup(&process_array, PROCESS_ARRAY_SIZE, sizeof(struct process_info));
    vector_setup(&tickets, 3 * (PROCESS_ARRAY_SIZE), sizeof(struct process_info *)); // Prepara os "tickets" com um tamanho adequado

    signal(SIGALRM, read_create_process);

    read_create_process();

    executa_escalonador();

    fclose(fptr);
    vector_destroy(&process_array);
    vector_destroy(&tickets);

    return 0;
}
