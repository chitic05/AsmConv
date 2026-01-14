.data
.text
.global _start
_start:
movl $0xff67ffff, %ecx
movl $0x5263dfaa, %eax
movl $1, %eax
movl $0, %ebx
int $0x80
