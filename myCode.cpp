#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define TIME_QUANTUM 3  // Round Robin의 타임 퀀텀
#define MAX_PROCESSES 100

// 프로세스 구조체 정의
typedef struct {
    int pid;         // 프로세스 ID
    int arrival;     // 도착 시간
    int burst;       // CPU 버스트 시간
    int priority;    // 우선 순위
    int waiting;     // 대기 시간
    int turnaround;  // 턴어라운드 시간
    int remaining;   // 남은 버스트 시간
} Process;

// 간트차트 항목 구조체 정의
typedef struct {
    int pid;         // 프로세스 ID
    int start;       // 시작 시간
    int end;         // 종료 시간
} GanttItem;

// 전역 변수
Process *processes_fcfs;
Process *processes_sjf;
Process *processes_psjf;
Process *processes_priority;
Process *processes_ppriority;
Process *processes_rr;
GanttItem gantt_chart[MAX_PROCESSES];
int gantt_index;
int n;

// 프로세스 비교 함수 (FCFS에 사용)
int compare_fcfs(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    return p1->arrival - p2->arrival;
}

// 프로세스 비교 함수 (SJF에 사용)
int compare_sjf(const void *a, const void *b) {
    Process *p1 = (Process *)a;
    Process *p2 = (Process *)b;
    if (p1->burst == p2->burst) {
        return p1->arrival - p2->arrival;
    }
    return p1->burst - p2->burst;
}

// 프로세스 비교 함수 (Priority에 사용)
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

// 간트차트 출력 함수
void print_gantt_chart() {
    printf("Gantt Chart:\n");

    int total_time = gantt_chart[gantt_index-1].end;
    
    printf ("    ");
    for (int i = 1; i <= total_time; i++) {
        printf(" %2d ", i);
    }
    printf("\n");

    for (int i = 0; i < gantt_index; i++) {
        printf(" P%d ", gantt_chart[i].pid);
        for (int j = 1; j < total_time; j++) {
            if (gantt_chart[i].start <= j && j < gantt_chart[i].end) printf("====");
            else printf("    ");
        }
        printf("\n");
    }
    printf("\n");
}

// 스케줄링 함수 (FCFS)
void schedule_fcfs(Process *processes, int n) {
    gantt_index = 0;
    qsort(processes, n, sizeof(Process), compare_fcfs);
    int current_time = 0;
    for (int i = 0; i < n; i++) {
        if (current_time < processes[i].arrival) {
            current_time = processes[i].arrival;
        }
        processes[i].waiting = current_time - processes[i].arrival;
        printf("Process %d is running for %d seconds\n", processes[i].pid, processes[i].burst);
        gantt_chart[gantt_index].pid = processes[i].pid;
        gantt_chart[gantt_index].start = current_time;
        sleep(processes[i].burst);  // 실행 시간 시뮬레이션
        current_time += processes[i].burst;
        gantt_chart[gantt_index].end = current_time;
        gantt_index++;
        processes[i].turnaround = processes[i].waiting + processes[i].burst;
    }
    print_gantt_chart();
}

// SJF 스케줄링 함수
void schedule_sjf(Process *processes, int n) {
    gantt_index = 0;
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

        if (shortest_index == -1) {
            current_time++;
            continue;
        }

        Process *current_process = &processes[shortest_index];

        if (current_time < current_process->arrival) {
            current_time = current_process->arrival;
        }

        current_process->waiting = current_time - current_process->arrival;
        current_process->remaining = 0;
        printf("Process %d is running for %d seconds\n", current_process->pid, current_process->burst);
        gantt_chart[gantt_index].pid = current_process->pid;
        gantt_chart[gantt_index].start = current_time;
        sleep(current_process->burst);
        current_time += current_process->burst;
        gantt_chart[gantt_index].end = current_time;
        gantt_index++;
        current_process->turnaround = current_process->waiting + current_process->burst;
        completed++;
    }

    print_gantt_chart();
}

// 스케줄링 함수 (Preemptive SJF)
void schedule_psjf(Process *processes, int n) {
    gantt_index = 0;
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
            printf("Process %d is running for 1 second\n", processes[shortest_index].pid);
            if (gantt_index > 0 && gantt_chart[gantt_index - 1].pid == processes[shortest_index].pid) {
                sleep(1);
                current_time++;
                gantt_chart[gantt_index - 1].end = current_time;
                processes[shortest_index].remaining--;
            }
            else {
                gantt_chart[gantt_index].pid = processes[shortest_index].pid;
                gantt_chart[gantt_index].start = current_time;
                sleep(1);  // 실행 시간 시뮬레이션
                current_time++;
                processes[shortest_index].remaining--;
                gantt_chart[gantt_index].end = current_time;
                gantt_index++;
            }
            if (processes[shortest_index].remaining == 0) {
                processes[shortest_index].waiting = current_time - processes[shortest_index].arrival - processes[shortest_index].burst;
                processes[shortest_index].turnaround = current_time - processes[shortest_index].arrival;
                completed++;
            }
        } else {
            current_time++;
        }
    }
    print_gantt_chart();
}

// 우선순위 스케줄링 함수
void schedule_priority(Process *processes, int n) {
    gantt_index = 0;
    int current_time = 0;
    int completed = 0;

    while (completed < n) {
        int highest_index = -1;
        int highest_priority = __INT_MAX__;
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival <= current_time && processes[i].remaining > 0 && processes[i].priority < highest_priority) {
                highest_priority = processes[i].priority;
                highest_index = i;
            }
        }

        if (highest_index == -1) {
            current_time++;
            continue;
        }

        Process *current_process = &processes[highest_index];

        if (current_time < current_process->arrival) {
            current_time = current_process->arrival;
        }

        current_process->waiting = current_time - current_process->arrival;
        printf("Process %d is running for %d seconds\n", current_process->pid, current_process->burst);
        gantt_chart[gantt_index].pid = current_process->pid;
        gantt_chart[gantt_index].start = current_time;
        sleep(current_process->burst);
        current_time += current_process->burst;
        gantt_chart[gantt_index].end = current_time;
        gantt_index++;
        current_process->turnaround = current_process->waiting + current_process->burst;
        current_process->remaining = 0;
        completed++;
    }

    // 간트 차트 출력 함수 호출
    print_gantt_chart();
}

// 스케줄링 함수 (Preemptive Priority)
void schedule_ppriority(Process *processes, int n) {
    gantt_index = 0;
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
            printf("Process %d is running for 1 second\n", processes[highest_priority_index].pid);
            if (gantt_index > 0 && gantt_chart[gantt_index - 1].pid == processes[highest_priority_index].pid) {
                sleep(1);
                current_time++;
                gantt_chart[gantt_index - 1].end = current_time;
                processes[highest_priority_index].remaining--;
            }
            else {
                gantt_chart[gantt_index].pid = processes[highest_priority_index].pid;
                gantt_chart[gantt_index].start = current_time;
                sleep(1);  // 실행 시간 시뮬레이션
                current_time++;
                processes[highest_priority_index].remaining--;
                gantt_chart[gantt_index].end = current_time;
                gantt_index++;
            }
            if (processes[highest_priority_index].remaining == 0) {
                processes[highest_priority_index].waiting = current_time - processes[highest_priority_index].arrival - processes[highest_priority_index].burst;
                processes[highest_priority_index].turnaround = current_time - processes[highest_priority_index].arrival;
                completed++;
            }
        } else {
            current_time++;
        }
    }
    print_gantt_chart();
}

// 스케줄링 함수 (Round Robin)
void schedule_rr(Process *processes, int n) {
    gantt_index = 0;
    int current_time = 0;
    int completed = 0;
    while (completed < n) {
        int find = 0;
        for (int i = 0; i < n; i++) {
            if (processes[i].arrival <= current_time && processes[i].remaining > 0) {
                int exec_time = (processes[i].remaining > TIME_QUANTUM) ? TIME_QUANTUM : processes[i].remaining;
                printf("Process %d is running for %d seconds\n", processes[i].pid, exec_time);
                gantt_chart[gantt_index].pid = processes[i].pid;
                gantt_chart[gantt_index].start = current_time;
                sleep(exec_time);  // 실행 시간 시뮬레이션
                current_time += exec_time;
                processes[i].remaining -= exec_time;
                gantt_chart[gantt_index].end = current_time;
                gantt_index++;
                if (processes[i].remaining == 0) {
                    processes[i].waiting = current_time - processes[i].arrival - processes[i].burst;
                    processes[i].turnaround = current_time - processes[i].arrival;
                    completed++;
                }
                find = 1;
            }
        }
        if (find == 0) current_time++;
    }
    print_gantt_chart();
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
    printf("\nFCFS Scheduling:\n");
    schedule_fcfs(processes_fcfs, n);
    evaluate(processes_fcfs, n);

    // SJF 스케줄링
    printf("\nSJF Scheduling:\n");
    schedule_sjf(processes_sjf, n);
    evaluate(processes_sjf, n);

    // Preemptive SJF 스케줄링
    printf("\nPreemptive SJF Scheduling:\n");
    schedule_psjf(processes_psjf, n);
    evaluate(processes_psjf, n);

    // Priority 스케줄링
    printf("\nPriority Scheduling:\n");
    schedule_priority(processes_priority, n);
    evaluate(processes_priority, n);

    // Preemptive Priority 스케줄링
    printf("\nPreemptive Priority Scheduling:\n");
    schedule_ppriority(processes_ppriority, n);
    evaluate(processes_ppriority, n);

    // Round Robin 스케줄링
    printf("\nRound Robin Scheduling:\n");
    schedule_rr(processes_rr, n);
    evaluate(processes_rr, n);

    free(processes_fcfs);
    free(processes_sjf);
    free(processes_psjf);
    free(processes_priority);
    free(processes_ppriority);
    free(processes_rr);

    return 0;
}
