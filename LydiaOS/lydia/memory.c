/* Enhanced Memory Management System for LydiaOS */
#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

/* 全局内存管理器 */
struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

/* 内存测试函数 - 保持不变 */
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { 
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; 
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; 
		store_cr0(cr0);
	}

	return i;
}

/* 初始化内存管理器 */
void memman_init(struct MEMMAN *man)
{
	/* 初始化传统管理 */
	man->frees = 0;			
	man->maxfrees = 0;		
	man->lostsize = 0;	
	man->losts = 0;
	
	/* 初始化页面管理 */
	man->page_table = 0;
	man->total_pages = 0;
	man->free_pages = 0;
	man->free_list = 0;
	
	/* 初始化伙伴系统 */
	int i;
	for (i = 0; i <= BUDDY_MAX_ORDER; i++) {
		man->buddy_free[i] = 0;
	}
	for (i = 0; i < (MAX_PAGES / 32) + 1; i++) {
		man->buddy_bitmap[i] = 0;
	}
	
	/* 初始化虚拟内存 */
	man->page_dir = 0;
	man->virt_next = VIRT_BASE;
	
	/* 默认使用最佳适配算法 */
	man->strategy = ALLOC_BEST_FIT;
	
	return;
}

/* 设置分配策略 */
void memman_set_strategy(struct MEMMAN *man, alloc_strategy_t strategy)
{
	man->strategy = strategy;
}

/* 获取伙伴系统所需的阶数 */
int buddy_get_order(unsigned int size)
{
	int order = 0;
	unsigned int pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	
	while ((1U << order) < pages) {
		order++;
	}
	return order;
}

/* 伙伴系统分配 */
unsigned int buddy_alloc(struct MEMMAN *man, int order)
{
	if (order > BUDDY_MAX_ORDER) {
		return 0;
	}
	
	/* 查找合适大小的块 */
	int current_order = order;
	while (current_order <= BUDDY_MAX_ORDER) {
		if (man->buddy_free[current_order] != 0) {
			/* 找到空闲块 */
			struct BUDDY_BLOCK *block = man->buddy_free[current_order];
			man->buddy_free[current_order] = block->next;
			
			/* 如果块太大，需要分割 */
			while (current_order > order) {
				current_order--;
				/* 创建伙伴块并加入到相应的空闲列表 */
				struct BUDDY_BLOCK *buddy = (struct BUDDY_BLOCK *)(block->addr + (1 << current_order) * PAGE_SIZE);
				buddy->addr = block->addr + (1 << current_order) * PAGE_SIZE;
				buddy->order = current_order;
				buddy->next = man->buddy_free[current_order];
				man->buddy_free[current_order] = buddy;
			}
			
			return block->addr;
		}
		current_order++;
	}
	
	return 0; /* 分配失败 */
}

/* 伙伴系统释放 */
void buddy_free(struct MEMMAN *man, unsigned int addr, int order)
{
	/* 将块加入到空闲列表 */
	struct BUDDY_BLOCK *block = (struct BUDDY_BLOCK *)addr;
	block->addr = addr;
	block->order = order;
	block->next = man->buddy_free[order];
	man->buddy_free[order] = block;
}

/* 最佳适配分配算法 */
unsigned int bestfit_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	int best_idx = -1;
	unsigned int best_size = 0xFFFFFFFF;
	
	/* 查找最佳匹配的块 */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size && man->free[i].size < best_size) {
			best_idx = i;
			best_size = man->free[i].size;
		}
	}
	
	if (best_idx == -1) {
		return 0; /* 没有合适的块 */
	}
	
	/* 分配内存 */
	a = man->free[best_idx].addr;
	man->free[best_idx].addr += size;
	man->free[best_idx].size -= size;
	
	if (man->free[best_idx].size == 0) {
		/* 块已完全使用，从空闲列表中移除 */
		man->frees--;
		for (i = best_idx; i < man->frees; i++) {
			man->free[i] = man->free[i + 1]; 
		}
	}
	
	return a;
}

/* 计算总可用内存 */
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

/* 智能内存分配 - 根据策略选择算法 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	switch (man->strategy) {
		case ALLOC_BEST_FIT:
			return bestfit_alloc(man, size);
			
		case ALLOC_BUDDY:
		{
			int order = buddy_get_order(size);
			return buddy_alloc(man, order);
		}
		
		case ALLOC_FIRST_FIT:
		default:
		{
			/* 传统的首次适配算法 */
			unsigned int i, a;
			for (i = 0; i < man->frees; i++) {
				if (man->free[i].size >= size) {
					a = man->free[i].addr;
					man->free[i].addr += size;
					man->free[i].size -= size;
					if (man->free[i].size == 0) {
						man->frees--;
						for (; i < man->frees; i++) {
							man->free[i] = man->free[i + 1]; 
						}
					}
					return a;
				}
			}
			return 0;
		}
	}
}

/* 内存释放 */
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	
	/* 对于伙伴系统，需要特殊处理 */
	if (man->strategy == ALLOC_BUDDY) {
		int order = buddy_get_order(size);
		buddy_free(man, addr, order);
		return 0;
	}
	
	/* 传统的释放算法 */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	
	/* 尝试与前一个块合并 */
	if (i > 0) {
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			man->free[i - 1].size += size;
			if (i < man->frees) {
				if (addr + size == man->free[i].addr) {
					/* 与后一个块也可以合并 */
					man->free[i - 1].size += man->free[i].size;
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; 
					}
				}
			}
			return 0; 
		}
	}
	
	/* 尝试与后一个块合并 */
	if (i < man->frees) {
		if (addr + size == man->free[i].addr) {
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}
	
	/* 无法合并，创建新的空闲块 */
	if (man->frees < MEMMAN_FREES) {
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; 
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}
	
	/* 空闲列表已满 */
	man->losts++;
	man->lostsize += size;
	return -1; 
}

/* 4K对齐的内存分配 */
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}

/* 4K对齐的内存释放 */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}

/* 页面管理初始化 */
void page_init(struct MEMMAN *man, unsigned int start_addr, unsigned int end_addr)
{
	man->total_pages = (end_addr - start_addr) / PAGE_SIZE;
	man->free_pages = man->total_pages;
}

/* 分配单个页面 */
struct PAGE_DESC *page_alloc(struct MEMMAN *man)
{
	if (man->free_list == 0) {
		return 0;
	}
	
	struct PAGE_DESC *page = man->free_list;
	man->free_list = page->next;
	page->state = PAGE_USED;
	page->ref_count = 1;
	man->free_pages--;
	
	return page;
}

/* 释放单个页面 */
void page_free(struct MEMMAN *man, struct PAGE_DESC *page)
{
	if (page->ref_count > 1) {
		page->ref_count--;
		return;
	}
	
	page->state = PAGE_FREE;
	page->ref_count = 0;
	page->next = man->free_list;
	man->free_list = page;
	man->free_pages++;
}

/* 虚拟内存初始化 */
void vmem_init(struct MEMMAN *man)
{
	/* 简化实现：初始化页目录 */
	man->page_dir = (unsigned int *)memman_alloc_4k(man, PAGE_SIZE);
	if (man->page_dir == 0) {
		return;
	}
	
	/* 清空页目录 */
	int i;
	for (i = 0; i < 1024; i++) {
		man->page_dir[i] = 0;
	}
	
	man->virt_next = VIRT_BASE;
}

/* 虚拟内存分配 */
unsigned int vmem_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int virt_addr = man->virt_next;
	unsigned int pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	
	/* 为每个页面分配物理内存并建立映射 */
	unsigned int i;
	for (i = 0; i < pages; i++) {
		unsigned int phys_addr = memman_alloc_4k(man, PAGE_SIZE);
		if (phys_addr == 0) {
			/* 分配失败，需要回滚 */
			return 0;
		}
		
		if (vmem_map(man, virt_addr + i * PAGE_SIZE, phys_addr) != 0) {
			/* 映射失败 */
			memman_free_4k(man, phys_addr, PAGE_SIZE);
			return 0;
		}
	}
	
	man->virt_next += pages * PAGE_SIZE;
	return virt_addr;
}

/* 建立虚拟地址到物理地址的映射 */
int vmem_map(struct MEMMAN *man, unsigned int virt_addr, unsigned int phys_addr)
{
	/* 简化实现：这里应该操作页表 */
	return 0;
}

/* 虚拟内存释放 */
void vmem_free(struct MEMMAN *man, unsigned int virt_addr, unsigned int size)
{
	unsigned int pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	
	/* 为每个页面解除映射并释放物理内存 */
	unsigned int i;
	for (i = 0; i < pages; i++) {
		vmem_unmap(man, virt_addr + i * PAGE_SIZE);
	}
}

/* 解除虚拟地址映射 */
void vmem_unmap(struct MEMMAN *man, unsigned int virt_addr)
{
	/* 简化实现：这里应该清除页表项并释放对应的物理页面 */
}

/* 伙伴系统初始化 */
void buddy_init(struct MEMMAN *man)
{
	/* 简化实现：将所有可用内存加入到最大阶的空闲块中 */
}
