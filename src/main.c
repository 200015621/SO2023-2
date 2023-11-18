#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/resource.h>

#define MAX_PROCESS_ARRAY_SIZE 25
#define MAX_PROCESS_NAME_SIZE 250
#define ARQUIVO "process_file.c"


// Estrutura para armazenar informações sobre o processo
struct process_info
{
    pid_t pid;                        // PID do processo
    int priority;                     // Prioridade do processo
    time_t start_time;                // Tempo de Inicio
};

// Variáveis globais
FILE *fptr;                      // Arquivo com as informacoes dos processos
struct process_info *proc_atual; // Ponteiro para o processo atual

size_t process_array_size = 0;
struct process_info process_array[MAX_PROCESS_ARRAY_SIZE];  // Vetor para armazenar informações dos processos

int alarm_times = 0;            // Numero de vezes que houve o signal de alarm 
time_t start_turnaround;        // Tempo de inicio do programa


// Função para criar um processo e adicionar ao vetor process_array
void create_process(char * exec_name, struct process_info * p_info){
    pid_t pid = fork();
    if (pid == 0)
    {
        execl(exec_name, "", NULL);
        exit(0);
    }
    else
    {
        kill(pid, SIGSTOP); // Envia um sinal para interromper o processo filho
        p_info->pid = pid;

        if(process_array_size < MAX_PROCESS_ARRAY_SIZE)
            process_array[process_array_size++] = *p_info;
        else
            printf("Limite de processos atigindo\n");
        
        // printf("[Processo %d] Prioridade %d, Executavel: %s\n", pid, p_info->priority, exec_name);
    }   
}

// Função para remover um processo do vetor process_array
void remove_process(struct process_info *p_info)
{
    int i;
    for (i = 0; i < process_array_size; i++)
    {
        struct process_info p_i = process_array[i];
        if (p_info->pid == p_i.pid)
        {
            size_t elem_size = sizeof(struct process_info);
            process_array_size--;
            memcpy(&process_array[i], &process_array[i+1], (process_array_size-i) * elem_size);
            break;
        }
    }

}

// Função para escolher um processo com base em "tickets" de acordo com a prioridade
struct process_info *escolhe_processo()
{
    int i, j;
    struct process_info * p_info;

    size_t tickets_array_size = 0;
    struct process_info * tickets[4*process_array_size];  // Vetor para armazenar "tickets" usados no escalonamento

    for (i = 0; i < process_array_size; i++)
    {
        p_info = &process_array[i];
        for (j = 0; j <= (p_info->priority); j++) // Atribui uma quantidade de "tickets" com base na prioridade
        {
            tickets[tickets_array_size++] = p_info;
        }
    }
    int n = rand() % (int)tickets_array_size;  // Escolhe um "ticket" aleatório

    return tickets[n]; // Retorna o processo correspondente ao "ticket"
}

// Função para interromper o processo atual usando um sinal a cada 6s
void quantum_escalonador()
{
    kill(proc_atual->pid, SIGSTOP);      // Envia um sinal para parar o processo atual
    alarm(6);
}

// Função para ler arquivo a cada 2s e criar um processo, e dar o quantum em 3*2s 
void read_create_process(){

    struct process_info p_info;
    char exec_name[MAX_PROCESS_NAME_SIZE+1] = {0};
    if( fscanf(fptr, "%250[^ ]%d\n", exec_name, &p_info.priority) != EOF ){
        time(&(p_info.start_time));
        create_process(exec_name, &p_info);
        alarm(2);
    }
    else {
        signal(SIGALRM, quantum_escalonador); // quando terminar o arquivo, trocar o alarm para funcao de 6 segundos
        alarm(6 - 2*alarm_times);            
    }

    if (alarm_times++ == 2)     // verifica se passou 6 segundos
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

    while(process_array_size)
    {
        proc_atual = escolhe_processo(); // Escolhe um processo
        kill(proc_atual->pid, SIGCONT);  // Envia um sinal para continuar o processo escolhido
        
        // printf("[Processo %d] executando\n", proc_atual->pid);

        wait_finish = wait4(proc_atual->pid, &status, WUNTRACED, &child_usage);
        if (WIFEXITED(status))
        {
            double makespan = difftime(time(NULL), proc_atual->start_time);
            printf("[Processo %d] Makespan: %.1lfs, Tempo de Execucao: %ld.%06lds\n", proc_atual->pid, makespan, child_usage.ru_utime.tv_sec, child_usage.ru_utime.tv_usec);
            remove_process(proc_atual);    // remove o processo se terminou
        }
    }
}

int main(int argc, char *argv[])
{
    fptr = fopen(ARQUIVO, "r");
    srand(time(NULL));
    signal(SIGALRM, read_create_process);

    start_turnaround = time(NULL);
    read_create_process();

    executa_escalonador();
    
    alarm(0);

    // printf("Tempo Total: %.1lf\n", difftime(time(NULL), start_turnaround));

    fclose(fptr);

    return 0;
}

