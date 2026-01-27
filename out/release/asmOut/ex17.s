.data
n: .long 6
s: .long 0
.text
.global _start
_start:
movl n, %ecx
movl $0, %eax
movl $0, %ebx
movl $6, %eax
movl $36, %eax
movl $0, %edx
movl $36, %ebx
movl $5, %ecx
movl $5, %eax
movl $25, %eax
movl $0, %edx
movl $61, %ebx
movl $4, %ecx
movl $4, %eax
movl $16, %eax
movl $0, %edx
movl $77, %ebx
movl $3, %ecx
movl $3, %eax
movl $9, %eax
movl $0, %edx
movl $86, %ebx
movl $2, %ecx
movl $2, %eax
movl $4, %eax
movl $0, %edx
movl $90, %ebx
movl $1, %ecx
movl $1, %eax
movl $1, %eax
movl $0, %edx
movl $91, %ebx
movl $0, %ecx
movl %ebx, s
movl $1, %eax
movl $0, %ebx
int $0x80
