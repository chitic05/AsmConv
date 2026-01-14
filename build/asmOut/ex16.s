.data
x: .long 77
.text
.global _start
_start:
movl x, %eax
movl $1, %ebx
movl $2, %ebx
movl $4, %ebx
movl $8, %ebx
movl $16, %ebx
movl $32, %ebx
movl $64, %ebx
movl $128, %ebx
movl $1, %eax
movl $0, %ebx
int $0x80
