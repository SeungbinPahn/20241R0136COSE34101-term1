#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define TIME_QUANTUM 3

typedef struct {
    int pid;         // 프로세스 ID
    int arrival;     // 도착 시간
    int burst;       // CPU 버스트 시간
    int priority;    // 우선 순위
    int waiting;     // 대기 시간
    int turnaround;  // 턴어라운드 시간
    int remaining;   // 남은 버스트 시간 (Round Robin에 사용)
} Process;

// 전역 변수
Process *processes_fcfs;
Process *processes_sjf;
Process *processes_psjf;
Process *processes_priority;
Process *processes_ppriority;
Process *processes_rr;
int n;
pthread_mutex_t lock;

// 프로세스 비교 함수 (FCFS에 사용)
int compare_fcfs(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    return p1->arrival - p2->arrival;
}

// 비교 함수 (SJF용)
int compare_sjf(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    if (p1->burst == p2->burst) {
        return p1->arrival - p2->arrival;
    }
    return p1->burst - p2->burst;
}

// 비교 함수 (Priority용)
int compare_priority(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    if (p1->priority == p2->priority) {
        return p1->arrival - p2->arrival;
    }
    return p1->priority - p2->priority;
}

// 프로세스 생성 함수
void create_processes(Process *processes, int n) {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        processes[i].pid = i + 1;
        processes[i].arrival = rand() % 10;
        processes[i].burst = rand() % 10 + 1;
        processes[i].priority = rand() % 10 + 1;
        processes[i].waiting = 0;
        processes[i].turnaround = 0;
        processes[i].remaining = processes[i].burst;
    }
}

// 스케줄링 함수 (FCFS)
void schedule_fcfs(Process *processes, int n) {
    qsort(processes, n, sizeof(Process), compare_fcfs);
    int current_time = 0;
    for (int i = 0; i < n; i++) {
        if (current_time < processes[i].arrival) {
            current_time = processes[i].arrival;
        }
        processes[i].waiting = current_time - processes[i].arrival;
        current_time += processes[i].burst;
        processes[i].turnaround = processes[i].waiting + processes[i].burst;
    }
}

// 스케줄링 함수 (SJF)
void schedule_sjf(Process *processes, int n) {
    qsort(processes, n, sizeof(Process), compare_sjf);
    int current_time = 0;
    for (int i = 0; i < n; i++) {
        if (current_time < processes[i].arrival) {
            current_time = processes[i].arrival;
        }
        processes[i].waiting = current_time - processes[i].arrival;
        current_time += processes[i].burst;
        processes[i].turnaround = processes[i].waiting + processes[i].burst;
    }
}

// 스케줄링 함수 (Preemptive SJF)
void schedule_psjf(Process *processes, int n) {
    int current_time = 0;
    int completed = 0;
    while (completed < n) {
        int shortest_index = -1;
        int shortest_burst = __INT_MAX__;
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival <= current_time && processes[i].remaining > 0 && processes[i].remaining < shortest_burst) {
                shortest_burst = processes[i].remaining;
                shortest_index = i;
            }
        }
        if (shortest_index != -1) {
            processes[shortest_index].remaining--;
            current_time++;
            if (processes[shortest_index].remaining == 0) {
                processes[shortest_index].waiting = current_time - processes[shortest_index].arrival - processes[shortest_index].burst;
                processes[shortest_index].turnaround = current_time - processes[shortest_index].arrival;
                completed++;
            }
        } else {
            current_time++;
        }
    }
}

// 스케줄링 함수 (Priority)
void schedule_priority(Process *processes, int n) {
    qsort(processes, n, sizeof(Process), compare_priority);
    int current_time = 0;
    for (int i = 0; i < n; i++) {
        if (current_time < processes[i].arrival) {
            current_time = processes[i].arrival;
        }
        processes[i].waiting = current_time - processes[i].arrival;
        current_time += processes[i].burst;
        processes[i].turnaround = processes[i].waiting + processes[i].burst;
    }
}

// 스케줄링 함수 (Preemptive Priority)
void schedule_ppriority(Process *processes, int n) {
    int current_time = 0;
    int completed = 0;
    while (completed < n) {
        int highest_priority_index = -1;
        int highest_priority = __INT_MAX__;
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival <= current_time && processes[i].remaining > 0 && processes[i].priority < highest_priority) {
                highest_priority = processes[i].priority;
                highest_priority_index = i;
            }
        }
        if (highest_priority_index != -1) {
            processes[highest_priority_index].remaining--;
            current_time++;
            if (processes[highest_priority_index].remaining == 0) {
                processes[highest_priority_index].waiting = current_time - processes[highest_priority_index].arrival - processes[highest_priority_index].burst;
                processes[highest_priority_index].turnaround = current_time - processes[highest_priority_index].arrival;
                completed++;
            }
        } else {
            current_time++;
        }
    }
}

// 스케줄링 함수 (Round Robin)
void schedule_rr(Process *processes, int n) {
    int current_time = 0;
    int completed = 0;
    while (completed < n) {
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival <= current_time && processes[i].remaining > 0) {
                if (processes[i].remaining > TIME_QUANTUM) {
                    current_time += TIME_QUANTUM;
                    processes[i].remaining -= TIME_QUANTUM;
                } else {
                    current_time += processes[i].remaining;
                    processes[i].waiting = current_time - processes[i].arrival - processes[i].burst;
                    processes[i].turnaround = current_time - processes[i].arrival;
                    processes[i].remaining = 0;
                    completed++;
                }
            }
        }
    }
}

// 스레드 함수 (프로세스 실행)
void* execute_process(void* arg) {
    Process *p = (Process *)arg;

    // 대기 시간 동안 대기
    sleep(p->waiting);

    printf("Process %d is running for %d seconds\n", p->pid, p->burst);
    sleep(p->burst);
    printf("Process %d finished\n", p->pid);

    pthread_exit(NULL);
}

// 평가 함수 (평균 대기 시간, 평균 턴어라운드 시간 계산)
void evaluate(Process *processes, int n) {
    double total_waiting = 0, total_turnaround = 0;
    for (int i = 0; i < n; i++) {
        total_waiting += processes[i].waiting;
        total_turnaround += processes[i].turnaround;
    }
    printf("Average Waiting Time: %.2f\n", total_waiting / n);
    printf("Average Turnaround Time: %.2f\n", total_turnaround / n);
}

// 메인 함수
int main() {
    printf("Enter number of processes: ");
    scanf("%d", &n);

    processes_fcfs = (Process *)malloc(n * sizeof(Process));
    processes_sjf = (Process *)malloc(n * sizeof(Process));
    processes_psjf = (Process *)malloc(n * sizeof(Process));
    processes_priority = (Process *)malloc(n * sizeof(Process));
    processes_ppriority = (Process *)malloc(n * sizeof(Process));
    processes_rr = (Process *)malloc(n * sizeof(Process));
    pthread_t *threads = (pthread_t *)malloc(n * sizeof(pthread_t));

    create_processes(processes_fcfs, n);

    // SJF, Preemptive SJF, Priority, Preemptive Priority, Round Robin 스케줄링을 위해 프로세스 복사
    for (int i = 0; i < n; i++) {
        processes_sjf[i] = processes_fcfs[i];
        processes_psjf[i] = processes_fcfs[i];
        processes_priority[i] = processes_fcfs[i];
        processes_ppriority[i] = processes_fcfs[i];
        processes_rr[i] = processes_fcfs[i];
    }

    printf("Processes:\n");
    printf("PID\tArrival\tBurst\tPriority\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\n", processes_fcfs[i].pid, processes_fcfs[i].arrival, processes_fcfs[i].burst, processes_fcfs[i].priority);
    }

    // FCFS 스케줄링
    schedule_fcfs(processes_fcfs, n);
    printf("\nScheduled Processes (FCFS):\n");
    printf("PID\tArrival\tBurst\tWaiting\tTurnaround\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\n", processes_fcfs[i].pid, processes_fcfs[i].arrival, processes_fcfs[i].burst, processes_fcfs[i].waiting, processes_fcfs[i].turnaround);
    }

    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, execute_process, (void *)&processes_fcfs[i]);
        pthread_join(threads[i], NULL);  // 각 스레드가 끝날 때까지 대기
    }

    evaluate(processes_fcfs, n);

    // SJF 스케줄링
    schedule_sjf(processes_sjf, n);
    printf("\nScheduled Processes (SJF):\n");
    printf("PID\tArrival\tBurst\tWaiting\tTurnaround\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\n", processes_sjf[i].pid, processes_sjf[i].arrival, processes_sjf[i].burst, processes_sjf[i].waiting, processes_sjf[i].turnaround);
    }

    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, execute_process, (void *)&processes_sjf[i]);
        pthread_join(threads[i], NULL);  // 각 스레드가 끝날 때까지 대기
    }

    evaluate(processes_sjf, n);

    // Preemptive SJF 스케줄링
    schedule_psjf(processes_psjf, n);
    printf("\nScheduled Processes (Preemptive SJF):\n");
    printf("PID\tArrival\tBurst\tWaiting\tTurnaround\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\n", processes_psjf[i].pid, processes_psjf[i].arrival, processes_psjf[i].burst, processes_psjf[i].waiting, processes_psjf[i].turnaround);
    }

    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, execute_process, (void *)&processes_psjf[i]);
        pthread_join(threads[i], NULL);  // 각 스레드가 끝날 때까지 대기
    }

    evaluate(processes_psjf, n);

    // Priority 스케줄링
    schedule_priority(processes_priority, n);
    printf("\nScheduled Processes (Priority):\n");
    printf("PID\tArrival\tBurst\tPriority\tWaiting\tTurnaround\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n", processes_priority[i].pid, processes_priority[i].arrival, processes_priority[i].burst, processes_priority[i].priority, processes_priority[i].waiting, processes_priority[i].turnaround);
    }

    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, execute_process, (void *)&processes_priority[i]);
        pthread_join(threads[i], NULL);  // 각 스레드가 끝날 때까지 대기
    }

    evaluate(processes_priority, n);

    // Preemptive Priority 스케줄링
    schedule_ppriority(processes_ppriority, n);
    printf("\nScheduled Processes (Preemptive Priority):\n");
    printf("PID\tArrival\tBurst\tPriority\tWaiting\tTurnaround\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n", processes_ppriority[i].pid, processes_ppriority[i].arrival, processes_ppriority[i].burst, processes_ppriority[i].priority, processes_ppriority[i].waiting, processes_ppriority[i].turnaround);
    }

    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, execute_process, (void *)&processes_ppriority[i]);
        pthread_join(threads[i], NULL);  // 각 스레드가 끝날 때까지 대기
    }

    evaluate(processes_ppriority, n);

    // Round Robin 스케줄링
    schedule_rr(processes_rr, n);
    printf("\nScheduled Processes (Round Robin):\n");
    printf("PID\tArrival\tBurst\tPriority\tWaiting\tTurnaround\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n", processes_rr[i].pid, processes_rr[i].arrival, processes_rr[i].burst, processes_rr[i].priority, processes_rr[i].waiting, processes_rr[i].turnaround);
    }

    for (int i = 0; i < n; i++) {
        pthread_create(&threads[i], NULL, execute_process, (void *)&processes_rr[i]);
        pthread_join(threads[i], NULL);  // 각 스레드가 끝날 때까지 대기
    }

    evaluate(processes_rr, n);

    free(processes_fcfs);
    free(processes_sjf);
    free(processes_psjf);
    free(processes_priority);
    free(processes_ppriority);
    free(processes_rr);
    free(threads);
    return 0;
}
