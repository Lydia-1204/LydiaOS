; LydiaOS 核心汇编函数库
; 提供系统底层硬件访问接口、中断处理和内存管理功能
; TAB=4

[FORMAT "WCOFF"]				; COFF目标文件格式
[INSTRSET "i486p"]			; 486处理器指令集
[BITS 32]					; 32位代码
[FILE "naskfunc.nas"]		; 源文件名

; ===== 全局函数声明 =====
; I/O操作函数
		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
		GLOBAL	_io_in8,  _io_in16,  _io_in32
		GLOBAL	_io_out8, _io_out16, _io_out32

; 系统寄存器操作
		GLOBAL	_io_load_eflags, _io_store_eflags
		GLOBAL	_load_gdtr, _load_idtr
		GLOBAL	_load_cr0, _store_cr0
		GLOBAL	_load_tr

; 中断处理程序包装器
		GLOBAL	_asm_inthandler20, _asm_inthandler21
		GLOBAL	_asm_inthandler2c, _asm_inthandler0c
		GLOBAL	_asm_inthandler0d, _asm_end_app

; 内存和应用程序管理
		GLOBAL	_memtest_sub
		GLOBAL	_farjmp, _farcall
		GLOBAL	_asm_hrb_api, _start_app

; ===== 外部函数声明 =====
		EXTERN	_inthandler20, _inthandler21
		EXTERN	_inthandler2c, _inthandler0d
		EXTERN	_inthandler0c
		EXTERN	_hrb_api

[SECTION .text]

; ===== CPU控制函数 =====

_io_hlt:	; void io_hlt(void) - CPU休眠等待中断
		HLT
		RET

_io_cli:	; void io_cli(void) - 禁用中断
		CLI
		RET

_io_sti:	; void io_sti(void) - 启用中断
		STI
		RET

_io_stihlt:	; void io_stihlt(void) - 启用中断并休眠
		STI
		HLT
		RET

; ===== 端口I/O函数 =====

_io_in8:	; int io_in8(int port) - 读取8位端口数据
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AL,DX
		RET

_io_in16:	; int io_in16(int port) - 读取16位端口数据
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,0
		IN		AX,DX
		RET

_io_in32:	; int io_in32(int port) - 读取32位端口数据
		MOV		EDX,[ESP+4]		; port
		IN		EAX,DX
		RET

_io_out8:	; void io_out8(int port, int data) - 写入8位端口数据
		MOV		EDX,[ESP+4]		; port
		MOV		AL,[ESP+8]		; data
		OUT		DX,AL
		RET

_io_out16:	; void io_out16(int port, int data) - 写入16位端口数据
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,AX
		RET

_io_out32:	; void io_out32(int port, int data) - 写入32位端口数据
		MOV		EDX,[ESP+4]		; port
		MOV		EAX,[ESP+8]		; data
		OUT		DX,EAX
		RET

; ===== 系统寄存器操作函数 =====

_io_load_eflags:	; int io_load_eflags(void) - 读取标志寄存器
		PUSHFD		
		POP		EAX
		RET

_io_store_eflags:	; void io_store_eflags(int eflags) - 设置标志寄存器
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD
		RET

_load_gdtr:		; void load_gdtr(int limit, int addr) - 加载全局描述符表寄存器
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LGDT	[ESP+6]
		RET

_load_idtr:		; void load_idtr(int limit, int addr) - 加载中断描述符表寄存器
		MOV		AX,[ESP+4]		; limit
		MOV		[ESP+6],AX
		LIDT	[ESP+6]
		RET

_load_cr0:		; int load_cr0(void) - 读取控制寄存器CR0
		MOV		EAX,CR0
		RET

_store_cr0:		; void store_cr0(int cr0) - 设置控制寄存器CR0
		MOV		EAX,[ESP+4]
		MOV		CR0,EAX
		RET

_load_tr:		; void load_tr(int tr) - 加载任务寄存器
		LTR		[ESP+4]			; tr
		RET

; ===== 中断处理程序包装器 =====

_asm_inthandler20:	; 定时器中断处理包装器(IRQ0)
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler20
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler21:	; 键盘中断处理包装器(IRQ1)
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler21
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler2c:	; 鼠标中断处理包装器(IRQ12)
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler2c
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		IRETD

_asm_inthandler0c:	; 栈段错误异常处理包装器(INT 0x0C)
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler0c
		CMP		EAX,0
		JNE		_asm_end_app
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4			
		IRETD

_asm_inthandler0d:	; 一般保护错误异常处理包装器(INT 0x0D)
		STI
		PUSH	ES
		PUSH	DS
		PUSHAD
		MOV		EAX,ESP
		PUSH	EAX
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_inthandler0d
		CMP		EAX,0			
		JNE		_asm_end_app	; 如果返回非0，终止应用程序
		POP		EAX
		POPAD
		POP		DS
		POP		ES
		ADD		ESP,4		
		IRETD

; ===== 内存测试函数 =====

_memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
				; 内存测试子程序，用于检测可用内存范围
		PUSH	EDI					
		PUSH	ESI
		PUSH	EBX
		MOV		ESI,0xaa55aa55			; pat0 = 0xaa55aa55 测试模式1
		MOV		EDI,0x55aa55aa			; pat1 = 0x55aa55aa 测试模式2
		MOV		EAX,[ESP+12+4]			; i = start 起始地址
mts_loop:
		MOV		EBX,EAX
		ADD		EBX,0xffc				; p = i + 0xffc 指向块末尾
		MOV		EDX,[EBX]				; old = *p 保存原始值
		MOV		[EBX],ESI				; *p = pat0 写入测试模式1
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff 翻转所有位
		CMP		EDI,[EBX]				; if (*p != pat1) goto fin 检查是否为预期值
		JNE		mts_fin
		XOR		DWORD [EBX],0xffffffff	; *p ^= 0xffffffff 再次翻转
		CMP		ESI,[EBX]				; if (*p != pat0) goto fin 检查是否恢复
		JNE		mts_fin
		MOV		[EBX],EDX				; *p = old 恢复原始值
		ADD		EAX,0x1000				; i += 0x1000 下一个4KB页
		CMP		EAX,[ESP+12+8]			; if (i <= end) goto mts_loop 检查是否到达结束
		JBE		mts_loop
		POP		EBX
		POP		ESI
		POP		EDI
		RET
mts_fin:
		MOV		[EBX],EDX				; *p = old 恢复原始值
		POP		EBX
		POP		ESI
		POP		EDI
		RET

; ===== 远程跳转和调用函数 =====

_farjmp:		; void farjmp(int eip, int cs) - 远程跳转
		JMP		FAR	[ESP+4]				; eip, cs
		RET

_farcall:		; void farcall(int eip, int cs) - 远程调用
		CALL	FAR	[ESP+4]				; eip, cs
		RET

; ===== 应用程序API接口 =====

_asm_hrb_api:	; HRB应用程序API调用包装器
		STI
		PUSH	DS
		PUSH	ES
		PUSHAD		
		PUSHAD		
		MOV		AX,SS
		MOV		DS,AX
		MOV		ES,AX
		CALL	_hrb_api
		CMP		EAX,0		
		JNE		_asm_end_app
		ADD		ESP,32
		POPAD
		POP		ES
		POP		DS
		IRETD

_asm_end_app:	; 应用程序结束处理
		MOV		ESP,[EAX]
		MOV		DWORD [EAX+4],0
		POPAD
		RET					

_start_app:		; void start_app(int eip, int cs, int esp, int ds, int *tss_esp0)
				; 启动用户应用程序
		PUSHAD		
		MOV		EAX,[ESP+36]	; eip
		MOV		ECX,[ESP+40]	; cs
		MOV		EDX,[ESP+44]	; esp
		MOV		EBX,[ESP+48]	; ds
		MOV		EBP,[ESP+52]	; tss_esp0
		MOV		[EBP  ],ESP		; 保存内核栈指针
		MOV		[EBP+4],SS		; 保存内核栈段
		MOV		ES,BX
		MOV		DS,BX
		MOV		FS,BX
		MOV		GS,BX

		OR		ECX,3			; 设置用户特权级(CPL=3)
		OR		EBX,3		    ; 设置用户特权级(CPL=3)
		PUSH	EBX				; ss
		PUSH	EDX			    ; esp
		PUSH	ECX			    ; cs
		PUSH	EAX		        ; eip
		RETF					; 返回到用户态
