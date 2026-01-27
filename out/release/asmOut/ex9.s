.data
x1: .ascii "maria"
x2: .long 0x61676362
x3: .ascii "maria2"
.text
.global _start
_start:
movl $4, %eax
movl $1, %ebx
movl $x1, %ecx
movl $14, %edx
int $0x80
movl $1, %eax
movl $0, %ebx
int $0x80
