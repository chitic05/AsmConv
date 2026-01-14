.data
n: .long 10
t1: .long 0
t2: .long 1
.text
.global _start
_start:
movl n, %ecx
movl $8, %ecx
movl t2, %eax
movl t1, %ebx
movl $1, %ebx
movl %ebx, t2
movl %eax, t1
movl t2, %eax
movl t1, %ebx
movl $2, %ebx
movl %ebx, t2
movl %eax, t1
movl t2, %eax
movl t1, %ebx
movl $3, %ebx
movl %ebx, t2
movl %eax, t1
movl t2, %eax
movl t1, %ebx
movl $5, %ebx
movl %ebx, t2
movl %eax, t1
movl t2, %eax
movl t1, %ebx
movl $8, %ebx
movl %ebx, t2
movl %eax, t1
movl t2, %eax
movl t1, %ebx
movl $13, %ebx
movl %ebx, t2
movl %eax, t1
movl t2, %eax
movl t1, %ebx
movl $21, %ebx
movl %ebx, t2
movl %eax, t1
movl t2, %eax
movl t1, %ebx
movl $34, %ebx
movl %ebx, t2
movl %eax, t1
movl $1, %eax
movl $0, %ebx
int $0x80
