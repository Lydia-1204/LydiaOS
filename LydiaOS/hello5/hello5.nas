; Hello World 汇编版本程序
; Assembly version of Hello World program
; 演示汇编语言直接调用系统API

[FORMAT "WCOFF"]		; COFF目标文件格式
[INSTRSET "i486p"]		; 486处理器指令集
[BITS 32]				; 32位代码
[FILE "hello5.nas"]		; 源文件名

		GLOBAL	_HariMain	; 导出主函数

[SECTION .text]

_HariMain:
		MOV		EDX,2		; API功能号2: 输出字符串
		MOV		EBX,msg		; 字符串地址
		INT		0x40		; 调用系统API
		MOV		EDX,4		; API功能号4: 程序结束
		INT		0x40		; 调用系统API

[SECTION .data]

msg:
		DB	"hello, world", 0x0a, 0	; 字符串数据 + 换行符 + 结束符