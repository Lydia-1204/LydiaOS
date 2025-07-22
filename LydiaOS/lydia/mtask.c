#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_now(void)
{
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}

void task_add(struct TASK *task)
{
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2;
	task->state = TASK_STATE_READY;
	return;
}

void task_remove(struct TASK *task)
{
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];

	for (i = 0; i < tl->running; i++) {
		if (tl->tasks[i] == task) {
			break;
		}
	}

	tl->running--;
	if (i < tl->now) {
		tl->now--;
	}
	if (tl->now >= tl->running) {
		tl->now = 0;
	}
	task->flags = 1;
	task->state = TASK_STATE_SLEEPING;

	for (; i < tl->running; i++) {
		tl->tasks[i] = tl->tasks[i + 1];
	}

	return;
}

void task_switchsub(void)
{
	int i;
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		if (taskctl->level[i].running > 0) {
			break; 
		}
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}

void task_idle(void)
{
	for (;;) {
		io_hlt();
	}
}

struct TASK *task_init(struct MEMMAN *memman)
{
	int i;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;

	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].state = TASK_STATE_UNUSED;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
		taskctl->tasks0[i].task_id = 0;
		taskctl->tasks0[i].priority = PRIORITY_NORMAL;
		taskctl->tasks0[i].base_priority = PRIORITY_NORMAL;
		taskctl->tasks0[i].time_slice = 10;  /* 减小到10ms，优化鼠标响应 */
		taskctl->tasks0[i].remaining_time = 10;
		taskctl->tasks0[i].waiting_semaphore = 0;
		taskctl->tasks0[i].waiting_msgqueue = 0;
		taskctl->tasks0[i].run_time = 0;
		taskctl->tasks0[i].sleep_time = 0;
		taskctl->tasks0[i].created_time = timerctl.count;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT);
	}
	for (i = 0; i < MAX_TASKLEVELS; i++) {
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}

	/* 初始化调度器 */
	scheduler_init();

	task = task_alloc();
	task->flags = 2;	
	task->priority = PRIORITY_NORMAL; 
	task->base_priority = PRIORITY_NORMAL;
	task->level = 0;
	task->state = TASK_STATE_RUNNING;
	task_add(task);
	task_switchsub();	
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->time_slice);

	idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int) &task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	idle->priority = PRIORITY_IDLE;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	idle->priority = PRIORITY_IDLE;
	idle->base_priority = PRIORITY_IDLE;
	task_run(idle, MAX_TASKLEVELS - 1, PRIORITY_IDLE);

	return task;
}

struct TASK *task_alloc(void)
{
	int i;
	struct TASK *task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1;
			task->state = TASK_STATE_READY;
			task->task_id = taskctl->next_task_id++;
			task->priority = PRIORITY_NORMAL;
			task->base_priority = PRIORITY_NORMAL;
			task->time_slice = 10;  /* 减小到10ms */
			task->remaining_time = 10;
			task->created_time = timerctl.count;
			task->run_time = 0;
			task->sleep_time = 0;
			task->waiting_semaphore = 0;
			task->waiting_msgqueue = 0;
			task->wait_result = 0;
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0;
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.iomap = 0x40000000;
			task->tss.ss0 = 0;
			return task;
		}
	}
	return 0; 
}

void task_run(struct TASK *task, int level, int priority)
{
	if (level < 0) {
		level = task->level; 
	}
	if (priority > 0) {
		task->priority = priority;
		task->base_priority = priority;
	}

	if (task->flags == 2 && task->level != level) { 
		task_remove(task);
	}
	if (task->flags != 2) {
		task->level = level;
		task_add(task);
	}

	taskctl->lv_change = 1; 
	return;
}

void task_sleep(struct TASK *task)
{
	struct TASK *now_task;
	if (task->flags == 2) {
		now_task = task_now();
		task->state = TASK_STATE_SLEEPING;
		task_remove(task); 
		if (task == now_task) {
			task_switchsub();
			now_task = task_now(); 
			farjmp(0, now_task->sel);
		}
	}
	return;
}

void task_set_priority(struct TASK *task, int priority)
{
	if (priority >= PRIORITY_HIGH && priority <= PRIORITY_IDLE) {
		task->priority = priority;
		task->base_priority = priority;
		
		/* 如果是当前运行的任务，可能需要重新调度 */
		if (task == task_now()) {
			taskctl->lv_change = 1;
		}
	}
}

void task_wake_up(struct TASK *task)
{
	if (task->state == TASK_STATE_SLEEPING) {
		task->state = TASK_STATE_READY;
		if (task->flags == 1) {
			task_run(task, -1, 0);
		}
	}
}

void task_block(struct TASK *task)
{
	task->state = TASK_STATE_BLOCKED;
	if (task->flags == 2) {
		task_remove(task);
		if (task == task_now()) {
			task_switchsub();
			struct TASK *next_task = task_now();
			farjmp(0, next_task->sel);
		}
	}
}

void task_unblock(struct TASK *task)
{
	if (task->state == TASK_STATE_BLOCKED) {
		task->state = TASK_STATE_READY;
		if (task->flags == 1) {
			task_run(task, -1, 0);
		}
	}
}

int task_get_state(struct TASK *task)
{
	return task->state;
}

void task_set_state(struct TASK *task, int state)
{
	task->state = state;
}

void task_switch(void)
{
	struct TASK *new_task, *now_task;
	static int priority_update_counter = 0;
	static int scheduler_counter = 0;
	
	/* 更新当前任务的运行时间统计 - 减少频率 */
	static int stat_counter = 0;
	stat_counter++;
	if (stat_counter >= 50) {  /* 每50次才更新一次统计 */
		now_task = task_now();
		if (now_task->state == TASK_STATE_RUNNING) {
			now_task->run_time++;
		}
		stat_counter = 0;
	}
	
	/* 减少当前任务的剩余时间片 */
	now_task = task_now();  /* 获取当前任务 */
	if (now_task->remaining_time > 0) {
		now_task->remaining_time--;
	}
	
	/* 暂时禁用优先级更新，减少CPU开销 */
	priority_update_counter++;
	if (priority_update_counter >= 10000) {  /* 极少更新 */
		/* scheduler_update_priorities(); // 暂时禁用 */
		priority_update_counter = 0;
	}
	
	/* 极少调用调度器 - 每50次task_switch才选择新任务 */
	scheduler_counter++;
	if (scheduler_counter >= 50) {
		/* 使用优先级调度选择下一个任务 */
		new_task = scheduler_select_next();
		scheduler_counter = 0;
	} else {
		new_task = 0;  /* 使用简单轮转，最小化调度开销 */
	}
	
	if (new_task == 0) {
		/* 如果没有找到合适的任务，使用原来的轮转调度作为后备 */
		struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
		tl->now++;
		if (tl->now == tl->running) {
			tl->now = 0;
		}
		if (taskctl->lv_change != 0) {
			task_switchsub();
			tl = &taskctl->level[taskctl->now_lv];
		}
		new_task = tl->tasks[tl->now];
	}
	
	/* 重置新任务的时间片 */
	if (new_task != now_task) {
		new_task->remaining_time = new_task->time_slice;
		new_task->state = TASK_STATE_RUNNING;
		if (now_task->state == TASK_STATE_RUNNING) {
			now_task->state = TASK_STATE_READY;
		}
	}
	
	/* 设置定时器为新任务的时间片 */
	timer_settime(task_timer, new_task->time_slice);
	
	if (new_task != now_task) {
		farjmp(0, new_task->sel);
	}
	return;
}
