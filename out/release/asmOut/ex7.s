.data
.text
.global _start
_start:
movl $0, %ecx
movl $3, %eax
movl $5, %ebx
movl $1, %ecx
movl $1, %eax
movl $0, %ebx
int $0x80
