.data
n: .long 9
v: .long 12, 15, 5, 15, 4, 1, 7, 15, 1
maxim: .long 0
ap: .long 0
.text
.global _start
_start:
movl n, %ecx
movl $v, %edi
movl n, %ebx
movl $0, %ebx
movl (%edi, %ebx, 4), %edx
movl %edx, maxim
movl n, %ebx
movl $1, %ebx
movl (%edi, %ebx, 4), %edx
movl %edx, maxim
movl n, %ebx
movl $2, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $3, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $4, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $5, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $6, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $7, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $8, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ecx
movl $v, %edi
movl n, %ebx
movl $0, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $1, %ebx
movl (%edi, %ebx, 4), %edx
movl $1, ap
movl n, %ebx
movl $2, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $3, %ebx
movl (%edi, %ebx, 4), %edx
movl $2, ap
movl n, %ebx
movl $4, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $5, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $6, %ebx
movl (%edi, %ebx, 4), %edx
movl n, %ebx
movl $7, %ebx
movl (%edi, %ebx, 4), %edx
movl $3, ap
movl n, %ebx
movl $8, %ebx
movl (%edi, %ebx, 4), %edx
movl $1, %eax
movl $0, %ebx
int $0x80
