//
// Created by Alex Brodsky on 2023-05-07.
//

#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include "process.h"
#include "prio_q.h"
#include "barrier.c"
#include "MessagePassingFacility.c"
#define MAX_PROCS 100
#define MAX_THREADS 100

enum {
    PROC_NEW = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_FINISHED,
    PROC_BLOCKED_SEND,
    PROC_BLOCKED_RECV
};

static char *states[] = {"new", "ready", "running", "blocked", "finished", "blocked (send)", "blocked (recv)"};
static int quantum;
static prio_q_t *finished;//finished queue to keep finished processes.

//barrier and message passing facility, for clock synchronization and message passing.
static barrier_t *barr;
static MessageFacility_t *messageFacility;

/* Initialize the simulation
 * @params:
 *   quantum: the CPU quantum to use in the situation
 * @returns:
 *   returns 1
 */
extern void process_init(int cpu_quantum,int num_threads) {
    /* Set up the finish queue and store the quantum
     * Assume the queue will be allocated
     */
    quantum = cpu_quantum;
    finished = prio_q_new();
    //initialize barrier and message passing facility, for clock synchronization and message passing.
    barr = barrier_new(num_threads);
    facilityInit(&messageFacility);

}

/* Create a new node context
 * @params:
 *   None
 * @returns:
 *   pointer to new node context.
 */
extern processor_t * process_new() {
    /* Allocate struct and set up the queues
     * Assume the queues will be allocated
     * Process ID sequence begins at 1
     */
    processor_t * cpu = calloc(1, sizeof(processor_t));
    assert(cpu);
    cpu->blocked = prio_q_new();
    cpu->ready = prio_q_new();
    cpu->next_proc_id = 1;
    return cpu;
}

/* Admit a process into the simulation
 * @params:
 *   proc: pointer to the program context of the process to be admitted
 *   cpu : node context
 * @returns:
 *   returns 1
 */
static void print_process(processor_t *cpu, context *proc) {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    /* Need to protect output with a global lock
     * Assume this is the only place where output occurs during multithreaded execution
     */
    int result = pthread_mutex_lock(&lock);
    assert(result == 0);
    printf("[%2.2d] %5.5d: process %d %s\n", proc->thread, cpu->clock_time,
           proc->id, states[proc->state]);
    result = pthread_mutex_unlock(&lock);
    assert(result == 0);
}

/* Add process to finished queue when they are done
 * @params:
 *   proc: pointer to the program context of the finished process
 *   cpu : node context
 * @returns:
 *   returns 1
 */
static void process_finished(processor_t *cpu, context *proc) {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    /* Need to protect shared queue global lock
     * threads are ordered by time, thread id, proc id.
     */
    proc->finished = cpu->clock_time;
    int result = pthread_mutex_lock(&lock);
    assert(result == 0);
    int order = cpu->clock_time * MAX_PROCS * MAX_THREADS + proc->thread * MAX_PROCS + proc->id;
    prio_q_add(finished, proc, order);
    result = pthread_mutex_unlock(&lock);
    assert(result == 0);
}

/* Compute priority of process, depending on whether SJF or priority based scheduling is used
 * @params:
 *   proc: process' context
 * @returns:
 *   priority of process
 */
static int actual_priority(context *proc) {
    if (proc->priority < 0) {
        /* SJF means duration of current DOOP is the priority
         */
        return proc->duration;
    }
    return proc->priority;
}

/* Insert process into appropriate queue based on the primitive it is performing
 * @params:
 *   proc: process' context
 *   cpu : node context
 *   next_op: if true, current primitive is done, so move IP to next primitive.
 * @returns:
 *   none
 */
static void insert_in_queue(processor_t *cpu, context *proc, int next_op) {
    /* If current primitive is done, move to next
     */
    if (next_op) {
        context_next_op(proc);
        proc->duration = context_cur_duration(proc);
    }

    int op = context_cur_op(proc);

    /* 3 cases:
     * 1. If DOOP/SEND/RECV, process goes into ready queue
     * 2. If BLOCK, process goes into blocked queue
     * 3. If HALT, process is not queued
     */
    if (op == OP_DOOP || op == OP_SEND || op == OP_RECV) {
        proc->state = PROC_READY;
        prio_q_add(cpu->ready, proc, actual_priority(proc));
        proc->wait_count++;
        proc->enqueue_time = cpu->clock_time;
    } else if (op == OP_BLOCK) {
        /* Use the duration field of the process to store their wake-up time.
         */
        proc->state = PROC_BLOCKED;
        proc->duration += cpu->clock_time;
        prio_q_add(cpu->blocked, proc, proc->duration);
    } else {
        proc->state = PROC_FINISHED;
        process_finished(cpu, proc);
    }
    print_process(cpu, proc);
}

/* Admit a process into the simulation
 * @params:
 *   proc: pointer to the program context of the process to be admitted
 *   cpu : node context
 * @returns:
 *   returns 1
 */
extern int process_admit(processor_t *cpu, context *proc) {
    /* Use node's PID counter to assign each process a unique process id.
     */
    proc->id = cpu->next_proc_id;
    cpu->next_proc_id++;
    proc->state = PROC_NEW;
    print_process(cpu, proc);
    insert_in_queue(cpu, proc, 1);
    return 1;
}

/* Perform the simulation
 * @params:
 *   cpu : node context
 * @returns:
 *   returns 1
 */
extern int process_simulate(processor_t *cpu) {
    context *cur = NULL;
    int cpu_quantum;

    /* We can only stop when all processes are in the finished state
     * no processes are readdy, running, or blocked
     */
    while(!prio_q_empty(cpu->ready) || !prio_q_empty(cpu->blocked) || cur != NULL || !prio_q_empty(&messageFacility->sendQ) || !prio_q_empty(&messageFacility->recvQ) ||!prio_q_empty(&messageFacility->completed)) {
        int preempt = 0;

        /* Step 1: Unblock processes
         * If any of the unblocked processes have completed blocking from send/recv/normal-blocking have higher priority than current running process
         *   we will need to preempt the current running process
         */
        assert(&messageFacility->completed);
        while(!prio_q_empty(&messageFacility->completed)){//if finished send and recv
            //remove from completed
            context *proc = prio_q_remove(&messageFacility->completed);
            //reset enqueue time as not needed for this queue
            proc->enqueue_time=cpu->clock_time;

            //insert in appropriate simulation queue and preempt if needed
            insert_in_queue(cpu, proc, 1);
            preempt |= cur != NULL && proc->state == PROC_READY &&
                       actual_priority(cur) > actual_priority(proc);
        }
        //for normal unblocking
        while (!prio_q_empty(cpu->blocked)) {
            /* We can stop ff process at head of queue should not be unblocked
             */
            context *proc = prio_q_peek(cpu->blocked);
            if (proc->duration > cpu->clock_time) {
                break;
            }

            /* Move from blocked and reinsert into appropriate queue
             */
            prio_q_remove(cpu->blocked);
            insert_in_queue(cpu, proc, 1);

            /* preemption is necessary if a process is running, and it has lower priority than
             * a newly unblocked ready process.
             */
            preempt |= cur != NULL && proc->state == PROC_READY &&
                    actual_priority(cur) > actual_priority(proc);
        }

        /* Step 2: Update current running process
         */
        if (cur != NULL) {
            cur->duration--;
            cpu_quantum--;

            /* Process stops running if it is preempted, has used up their quantum, or has completed its DOOP
            */
            if ((cur->duration == 0 || cpu_quantum == 0 ||  preempt) ||  cur->code[cur->ip].op==OP_SEND || cur->code[cur->ip].op==OP_RECV ) {
                //if its send then invoke send function
                if(cur->code[cur->ip].op==OP_SEND){

                    //change state and print
                    cur->state=PROC_BLOCKED_SEND;
                    print_process(cpu,cur);

                    //invoke
                    send(messageFacility,cur,cur->code[cur->ip].addressNodeId,cur->code[cur->ip].addressProcessId);
                    cur = NULL; //for next process to run
                }else if(cur->code[cur->ip].op==OP_RECV){ //if recv then invoke recv function

                    //change state and print
                    cur->state=PROC_BLOCKED_RECV;
                    print_process(cpu,cur);

                    //invoke
                    recv(messageFacility,cur,cur->code[cur->ip].addressNodeId,cur->code[cur->ip].addressProcessId);
                    cur = NULL; //for next process to run
                }
                 else {//insert again as process stops running if it is preempted, has used up their quantum, or has completed its DOOP, and its not send/recv
                    insert_in_queue(cpu, cur, cur->duration == 0);
                    cur = NULL;
                }

            }
        }

        /* Step 3: Select next ready process to run if none are running
         * Be sure to keep track of how long it waited in the ready queue
         */
        if (cur == NULL && !prio_q_empty(cpu->ready)) {
            cur = prio_q_remove(cpu->ready);
                cur->wait_time += cpu->clock_time - cur->enqueue_time;
            cpu_quantum = quantum;
            cur->state = PROC_RUNNING;
            print_process(cpu, cur);
        }
        //call barrier to synchronize clocks
        barrier_wait(barr);
        cpu->clock_time++;
    }
    barrier_done(barr);//done with barrier so decrement max_threads count inside barrier.

    /* next clock tick
     */
    return 1;
}

/* Output process summary post execution
 * @params:
 *   fout : output file
 * @returns:
 *   none
 */
extern void process_summary(FILE *fout) {
    /* Finished processes are in order in the queue
     */
    while (!prio_q_empty(finished)) {
        context *proc = prio_q_remove(finished);
        context_stats(proc, fout);
    }
}
