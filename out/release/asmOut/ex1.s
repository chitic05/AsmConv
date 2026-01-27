.data
n: .long 6
v: .word 10, 20, 40, 60, 100, 5
.text
.global _start
_start:
movl $v, %edi
movl $1, %ecx
movl (%edi, %ecx, 4), %edx
movl $1, %eax
movl $0, %ebx
int $0x80
