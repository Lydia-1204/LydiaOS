/* Enhanced Memory Management System for LydiaOS */
#include "bootpack.h"

#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

/* 全局内存管理器 */
struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

/* 伙伴块池和回收池 */
#define BUDDY_BLOCK_POOL_SIZE 8192  /* 增大池子大小 */
static struct BUDDY_BLOCK buddy_block_pool[BUDDY_BLOCK_POOL_SIZE];
static int buddy_block_pool_used = 0;
static struct BUDDY_BLOCK *buddy_block_free_list = 0;

/* 调试宏 */
#ifdef DEBUG_BUDDY
#define buddy_debug(fmt, ...) /* 实际实现中可以用printf */
#else
#define buddy_debug(fmt, ...)
#endif

/* 前向声明 */
static void buddy_init_recursive(struct MEMMAN *man, unsigned int addr, int order, unsigned int remaining_pages);

/* 内存测试函数 - 修正版本 */
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 检测486处理器 */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { 
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	/* 如果是486或更高版本，禁用缓存 */
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; 
		store_cr0(cr0);
	}

	/* 调用汇编版本的内存测试 */
	i = memtest_sub(start, end);

	/* 恢复缓存设置 */
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; 
		store_cr0(cr0);
	}

	return i;
}

/* 分配一个BUDDY_BLOCK，优先用回收池 */
static struct BUDDY_BLOCK *alloc_buddy_block(unsigned int addr, int order) {
	struct BUDDY_BLOCK *blk;
	if (buddy_block_free_list) {
		blk = buddy_block_free_list;
		buddy_block_free_list = blk->next;
	} else {
		if (buddy_block_pool_used >= BUDDY_BLOCK_POOL_SIZE) {
			buddy_debug("Buddy block pool exhausted!\n");
			return 0; /* 池满保护 */
		}
		blk = &buddy_block_pool[buddy_block_pool_used++];
	}
	blk->addr = addr;
	blk->order = order;
	blk->next = 0;
	return blk;
}

/* 释放一个BUDDY_BLOCK到回收池 */
static void free_buddy_block(struct BUDDY_BLOCK *blk) {
	if (blk == 0) return;
	blk->next = buddy_block_free_list;
	buddy_block_free_list = blk;
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
	if (man != 0) {
		man->strategy = strategy;
	}
}

/* 获取伙伴系统所需的阶数 */
int buddy_get_order(unsigned int size)
{
	int order = 0;
	unsigned int pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
	
	if (pages == 0) return 0;
	
	/* 找到能容纳pages的最小2的幂 */
	pages--; /* 减1是为了处理正好是2的幂的情况 */
	while (pages > 0) {
		pages >>= 1;
		order++;
	}
	
	return (order > BUDDY_MAX_ORDER) ? BUDDY_MAX_ORDER : order;
}

/* 正确的伙伴地址计算 */
static unsigned int buddy_get_buddy_addr(unsigned int addr, int order)
{
	unsigned int block_size = (1U << order) * PAGE_SIZE;
	unsigned int offset_from_start = addr - MEM_BUDDY_START;
	unsigned int block_index = offset_from_start / block_size;
	
	/* 伙伴块的索引：如果当前块索引是偶数，伙伴是下一个；如果是奇数，伙伴是前一个 */
	if (block_index & 1) {
		/* 奇数块，伙伴在左边 */
		return addr - block_size;
	} else {
		/* 偶数块，伙伴在右边 */
		return addr + block_size;
	}
}

/* 改进的地址验证函数 */
static int buddy_addr_valid(unsigned int addr, int order)
{
	unsigned int block_size = (1U << order) * PAGE_SIZE;
	
	/* 检查地址范围 */
	if (addr < MEM_BUDDY_START || addr >= MEM_BUDDY_END) {
		buddy_debug("Address 0x%x out of range [0x%x, 0x%x)\n", 
					addr, MEM_BUDDY_START, MEM_BUDDY_END);
		return 0;
	}
	
	/* 检查是否会越界 */
	if (addr + block_size > MEM_BUDDY_END) {
		buddy_debug("Block at 0x%x size 0x%x exceeds end 0x%x\n", 
					addr, block_size, MEM_BUDDY_END);
		return 0;
	}
	
	/* 放宽对齐检查 - 只要求页面对齐 */
	if (addr & (PAGE_SIZE - 1)) {
		buddy_debug("Address 0x%x not page aligned\n", addr);
		return 0;
	}
	
	/* 检查块是否在正确的边界上 */
	unsigned int offset_from_start = addr - MEM_BUDDY_START;
	if (offset_from_start & (block_size - 1)) {
		buddy_debug("Address 0x%x not aligned to block size 0x%x\n", 
					addr, block_size);
		return 0;
	}
	
	return 1;
}

/* 伙伴系统分配 */
unsigned int buddy_alloc(struct MEMMAN *man, int order)
{
	if (order > BUDDY_MAX_ORDER || order < 0 || man == 0) {
		buddy_debug("Invalid order %d or null memman\n", order);
		return 0;
	}
	
	buddy_debug("Allocating order %d\n", order);
	
	/* 查找合适大小的块 */
	int current_order = order;
	while (current_order <= BUDDY_MAX_ORDER) {
		if (man->buddy_free[current_order] != 0) {
			/* 找到空闲块 */
			struct BUDDY_BLOCK *block = man->buddy_free[current_order];
			man->buddy_free[current_order] = block->next;
			unsigned int addr = block->addr;
			free_buddy_block(block);
			
			buddy_debug("Found block at 0x%x order %d\n", addr, current_order);
			
			/* 验证地址有效性 */
			if (!buddy_addr_valid(addr, current_order)) {
				buddy_debug("Invalid block address 0x%x order %d\n", addr, current_order);
				return 0;
			}
			
			/* 分割块直到达到需要的大小 */
			while (current_order > order) {
				current_order--;
				unsigned int buddy_addr = addr + (1U << current_order) * PAGE_SIZE;
				
				buddy_debug("Splitting: addr=0x%x, buddy=0x%x, order=%d\n", 
							addr, buddy_addr, current_order);
				
				/* 验证伙伴地址 */
				if (!buddy_addr_valid(buddy_addr, current_order)) {
					buddy_debug("Invalid buddy address during split\n");
					return 0;
				}
				
				struct BUDDY_BLOCK *buddy = alloc_buddy_block(buddy_addr, current_order);
				if (!buddy) {
					buddy_debug("Failed to allocate buddy block\n");
					return 0;
				}
				buddy->next = man->buddy_free[current_order];
				man->buddy_free[current_order] = buddy;
			}
			
			buddy_debug("Allocation successful: 0x%x order %d\n", addr, order);
			return addr;
		}
		current_order++;
	}
	
	buddy_debug("No suitable block found for order %d\n", order);
	return 0; /* 分配失败 */
}

/* 伙伴系统释放 */
void buddy_free(struct MEMMAN *man, unsigned int addr, int order)
{
	if (man == 0 || order < 0 || order > BUDDY_MAX_ORDER) {
		buddy_debug("Invalid parameters: man=%p, order=%d\n", man, order);
		return;
	}
	
	/* 验证地址有效性 */
	if (!buddy_addr_valid(addr, order)) {
		buddy_debug("Invalid address 0x%x for order %d\n", addr, order);
		return;
	}
	
	buddy_debug("Freeing addr=0x%x order=%d\n", addr, order);
	
	/* 尝试与伙伴合并 */
	while (order < BUDDY_MAX_ORDER) {
		unsigned int buddy_addr = buddy_get_buddy_addr(addr, order);
		
		buddy_debug("Looking for buddy at 0x%x\n", buddy_addr);
		
		/* 验证伙伴地址 */
		if (!buddy_addr_valid(buddy_addr, order)) {
			buddy_debug("Buddy address 0x%x invalid\n", buddy_addr);
			break;
		}
		
		/* 在同阶空闲列表中查找伙伴 */
		struct BUDDY_BLOCK **prev = &man->buddy_free[order];
		struct BUDDY_BLOCK *curr = man->buddy_free[order];
		int found = 0;
		
		while (curr) {
			if (curr->addr == buddy_addr) {
				/* 找到伙伴，合并 */
				buddy_debug("Found buddy, merging\n");
				*prev = curr->next;
				free_buddy_block(curr);
				
				/* 合并后的起始地址是两个地址中较小的 */
				if (buddy_addr < addr) {
					addr = buddy_addr;
				}
				order++;
				found = 1;
				break;
			}
			prev = &curr->next;
			curr = curr->next;
		}
		
		if (!found) {
			buddy_debug("No buddy found at 0x%x\n", buddy_addr);
			break;
		}
	}
	
	/* 将最终块加入空闲列表 */
	struct BUDDY_BLOCK *block = alloc_buddy_block(addr, order);
	if (!block) {
		buddy_debug("Failed to allocate block structure\n");
		return;
	}
	block->next = man->buddy_free[order];
	man->buddy_free[order] = block;
	
	buddy_debug("Final free: addr=0x%x order=%d\n", addr, order);
}

/* 递归创建伙伴块的辅助函数 */
static void buddy_init_recursive(struct MEMMAN *man, unsigned int addr, int order, unsigned int remaining_pages)
{
	if (remaining_pages == 0 || order < 0) {
		return;
	}
	
	unsigned int pages_in_block = 1U << order;
	
	if (pages_in_block <= remaining_pages) {
		/* 可以创建一个完整的块 */
		if (buddy_addr_valid(addr, order)) {
			struct BUDDY_BLOCK *block = alloc_buddy_block(addr, order);
			if (block) {
				block->next = man->buddy_free[order];
				man->buddy_free[order] = block;
				buddy_debug("Created block: addr=0x%x order=%d\n", addr, order);
			}
		}
		
		/* 处理剩余内存 */
		buddy_init_recursive(man, addr + pages_in_block * PAGE_SIZE, 
							order, remaining_pages - pages_in_block);
	} else {
		/* 当前阶数太大，尝试更小的阶数 */
		buddy_init_recursive(man, addr, order - 1, remaining_pages);
	}
}

/* 改进的伙伴系统初始化 */
void buddy_init(struct MEMMAN *man)
{
	if (man == 0) return;
	
	buddy_debug("Initializing buddy system\n");
	
	/* 重置池 */
	buddy_block_pool_used = 0;
	buddy_block_free_list = 0;
	
	/* 清空所有空闲列表 */
	int i;
	for (i = 0; i <= BUDDY_MAX_ORDER; i++) {
		man->buddy_free[i] = 0;
	}
	
	unsigned int start = MEM_BUDDY_START;
	unsigned int end = MEM_BUDDY_END;
	
	buddy_debug("Memory range: 0x%x - 0x%x\n", start, end);
	
	/* 验证内存范围 */
	if (start >= end || (end - start) < PAGE_SIZE) {
		buddy_debug("Invalid memory range\n");
		return;
	}
	
	/* 页对齐 */
	start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	end = end & ~(PAGE_SIZE - 1);
	
	if (start >= end) {
		buddy_debug("No memory left after alignment\n");
		return;
	}
	
	buddy_debug("Aligned range: 0x%x - 0x%x\n", start, end);
	
	/* 改进的初始化策略：从最大块开始，递归分割 */
	unsigned int total_pages = (end - start) / PAGE_SIZE;
	
	/* 找到能容纳所有内存的最大阶数 */
	int max_possible_order = 0;
	unsigned int pages_in_max_order = 1;
	while (pages_in_max_order < total_pages && max_possible_order < BUDDY_MAX_ORDER) {
		max_possible_order++;
		pages_in_max_order <<= 1;
	}
	
	/* 如果总页数不是2的幂，则需要分割 */
	if (pages_in_max_order > total_pages) {
		max_possible_order--;
	}
	
	buddy_debug("Total pages: %d, max order: %d\n", total_pages, max_possible_order);
	
	/* 递归创建块 */
	buddy_init_recursive(man, start, max_possible_order, total_pages);
	
	buddy_debug("Buddy system initialized\n");
	
	/* 调试：打印初始状态 */
	for (i = 0; i <= BUDDY_MAX_ORDER; i++) {
		int count = 0;
		struct BUDDY_BLOCK *block = man->buddy_free[i];
		while (block) {
			count++;
			block = block->next;
		}
		if (count > 0) {
			buddy_debug("Order %d: %d blocks\n", i, count);
		}
	}
}

/* 最佳适配分配算法 */
unsigned int bestfit_alloc(struct MEMMAN *man, unsigned int size)
{
	if (man == 0 || size == 0) {
		return 0;
	}
	
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
	if (man == 0) return 0;
	
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

/* 智能内存分配 - 根据策略选择算法 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	if (man == 0 || size == 0) {
		return 0;
	}
	
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
	if (man == 0 || size == 0) {
		return -1;
	}
	
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
	if (man == 0) return;
	man->total_pages = (end_addr - start_addr) / PAGE_SIZE;
	man->free_pages = man->total_pages;
}

/* 分配单个页面 */
struct PAGE_DESC *page_alloc(struct MEMMAN *man)
{
	if (man == 0 || man->free_list == 0) {
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
	if (man == 0 || page == 0) return;
	
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
	if (man == 0) return;
	
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
	if (man == 0 || size == 0) return 0;
	
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
	if (man == 0 || man->page_dir == 0) return -1;
	
	/* 假设页目录和页表均为 4KB，1024 项，每项4字节 */
	unsigned int pd_idx = (virt_addr >> 22) & 0x3ff;
	unsigned int pt_idx = (virt_addr >> 12) & 0x3ff;

	unsigned int *page_dir = man->page_dir;
	unsigned int *page_table;

	/* 如果页目录项不存在，则分配一个新的页表 */
	if ((page_dir[pd_idx] & 0x1) == 0) {
		page_table = (unsigned int *)memman_alloc_4k(man, PAGE_SIZE);
		if (page_table == 0) return -1;
		int i;
		for (i = 0; i < 1024; i++) page_table[i] = 0;
		page_dir[pd_idx] = ((unsigned int)page_table) | 0x7; /* present, rw, user */
	} else {
		page_table = (unsigned int *)(page_dir[pd_idx] & 0xfffff000);
	}

	/* 设置页表项 */
	page_table[pt_idx] = (phys_addr & 0xfffff000) | 0x7; /* present, rw, user */
	return 0;
}

/* 虚拟内存释放 */
void vmem_free(struct MEMMAN *man, unsigned int virt_addr, unsigned int size)
{
	if (man == 0 || size == 0) return;
	
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
	if (man == 0 || man->page_dir == 0) return;
	
	unsigned int pd_idx = (virt_addr >> 22) & 0x3ff;
	unsigned int pt_idx = (virt_addr >> 12) & 0x3ff;
	unsigned int *page_dir = man->page_dir;
	if ((page_dir[pd_idx] & 0x1) == 0) return; /* 无页表 */
	unsigned int *page_table = (unsigned int *)(page_dir[pd_idx] & 0xfffff000);
	unsigned int entry = page_table[pt_idx];
	if ((entry & 0x1) == 0) return; /* 无映射 */

	unsigned int phys_addr = entry & 0xfffff000;
	/* 释放物理页面 */
	memman_free_4k(man, phys_addr, PAGE_SIZE);
	/* 清除页表项 */
	page_table[pt_idx] = 0;
}

/* 页面地址转换函数 */
unsigned int page_to_addr(struct PAGE_DESC *page)
{
	if (page == 0) return 0;
	return page->phys_addr;
}

/* 地址到页面描述符转换 */
struct PAGE_DESC *addr_to_page(struct MEMMAN *man, unsigned int addr)
{
	if (man == 0 || man->page_table == 0) return 0;
	
	/* 简化实现：假设线性映射 */
	unsigned int page_index = (addr - MEM_BUDDY_START) / PAGE_SIZE;
	if (page_index >= man->total_pages) return 0;
	
	return &man->page_table[page_index];
}