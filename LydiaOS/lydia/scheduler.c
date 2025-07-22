#include "bootpack.h"

/* 简化的优先级调度器实现 */

void scheduler_init(void)
{
    /* 调度器初始化 - 在task_init中调用 */
    int i;
    
    /* 初始化信号量 */
    for (i = 0; i < MAX_SEMAPHORES; i++) {
        taskctl->semaphores[i].flags = 0;
        taskctl->semaphores[i].value = 0;
        taskctl->semaphores[i].waiting_count = 0;
    }
    
    /* 初始化消息队列 */
    for (i = 0; i < MAX_MESSAGE_QUEUES; i++) {
        taskctl->message_queues[i].flags = 0;
        taskctl->message_queues[i].message_count = 0;
        taskctl->message_queues[i].head = 0;
        taskctl->message_queues[i].tail = 0;
        taskctl->message_queues[i].waiting_count = 0;
    }
    
    /* 初始化消息池 */
    for (i = 0; i < MAX_MESSAGES; i++) {
        taskctl->messages[i].next = 0;
    }
    
    taskctl->next_task_id = 1;
    taskctl->next_message_id = 0;
}

struct TASK *scheduler_select_next(void)
{
    /* 平衡的调度器 - 保持优先级功能但减少复杂度 */
    struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
    struct TASK *best_task = 0;
    int i;
    
    /* 首先在当前级别查找可运行任务 */
    if (tl->running > 0) {
        for (i = 0; i < tl->running; i++) {
            int idx = (tl->now + i) % tl->running;
            struct TASK *task = tl->tasks[idx];
            if (task->state == TASK_STATE_READY) {
                tl->now = idx;
                return task;
            }
        }
    }
    
    /* 如果当前级别没有可运行任务，检查其他级别 */
    for (i = 0; i < MAX_TASKLEVELS; i++) {
        if (i == taskctl->now_lv) continue;
        tl = &taskctl->level[i];
        if (tl->running > 0) {
            struct TASK *task = tl->tasks[0];
            if (task->state == TASK_STATE_READY) {
                taskctl->now_lv = i;
                tl->now = 0;
                return task;
            }
        }
    }
    
    return 0;
}

void scheduler_update_priorities(void)
{
    /* 极简优先级更新 - 几乎不做任何操作 */
    static int update_counter = 0;
    update_counter++;
    
    /* 每10000次才做一次轻微调整 */
    if (update_counter >= 10000) {
        update_counter = 0;
        /* 几乎不做任何优先级调整，保持性能 */
    }
}

/* 简化的信号量实现 */
struct SEMAPHORE *semaphore_alloc(char *name, int initial_value)
{
    int i, j;
    
    for (i = 0; i < MAX_SEMAPHORES; i++) {
        if (taskctl->semaphores[i].flags == 0) {
            taskctl->semaphores[i].flags = 1;
            taskctl->semaphores[i].value = initial_value;
            taskctl->semaphores[i].waiting_count = 0;
            
            /* 复制名称 */
            for (j = 0; j < 15 && name[j] != 0; j++) {
                taskctl->semaphores[i].name[j] = name[j];
            }
            taskctl->semaphores[i].name[j] = 0;
            
            return &taskctl->semaphores[i];
        }
    }
    return 0; /* 分配失败 */
}

void semaphore_free(struct SEMAPHORE *sem)
{
    if (sem != 0) {
        sem->flags = 0;
        sem->waiting_count = 0;
    }
}

int semaphore_wait(struct SEMAPHORE *sem)
{
    int eflags = io_load_eflags();
    
    io_cli();
    
    if (sem->value > 0) {
        sem->value--;
        io_store_eflags(eflags);
        return 0; /* 成功获取 */
    } else {
        /* 简化：直接返回失败，不实现等待 */
        io_store_eflags(eflags);
        return -1; /* 失败 */
    }
}

int semaphore_signal(struct SEMAPHORE *sem)
{
    int eflags = io_load_eflags();
    
    io_cli();
    sem->value++;
    io_store_eflags(eflags);
    return 0;
}

/* 简化的消息队列实现 */
struct MESSAGE_QUEUE *msgqueue_alloc(char *name, int max_messages)
{
    int i, j;
    
    for (i = 0; i < MAX_MESSAGE_QUEUES; i++) {
        if (taskctl->message_queues[i].flags == 0) {
            taskctl->message_queues[i].flags = 1;
            taskctl->message_queues[i].max_messages = max_messages;
            taskctl->message_queues[i].message_count = 0;
            taskctl->message_queues[i].head = 0;
            taskctl->message_queues[i].tail = 0;
            taskctl->message_queues[i].waiting_count = 0;
            
            /* 复制名称 */
            for (j = 0; j < 15 && name[j] != 0; j++) {
                taskctl->message_queues[i].name[j] = name[j];
            }
            taskctl->message_queues[i].name[j] = 0;
            
            return &taskctl->message_queues[i];
        }
    }
    return 0;
}

void msgqueue_free(struct MESSAGE_QUEUE *queue)
{
    if (queue != 0) {
        queue->flags = 0;
        queue->waiting_count = 0;
    }
}

int msgqueue_send(struct MESSAGE_QUEUE *queue, int type, char *data, int size)
{
    /* 简化实现：暂时只返回成功 */
    return 0;
}

int msgqueue_receive(struct MESSAGE_QUEUE *queue, int *type, char *data, int *size)
{
    /* 简化实现：暂时只返回失败 */
    return -1;
}
