.data
v: .long 26, 12, 3, 56, 3, 18, 27, 35, 15
n: .long 9
max1: .long 0
max2: .long 0
formatAf: .ascii "%ld\n"
.text
.global _start
_start:
movl n, %ecx
movl $v, %edi
movl $0, %eax
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl %ebx, max2
movl %edx, max1
movl $1, %eax
movl $8, %ecx
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl max2, %ebx
movl max1, %esi
movl %edx, max2
movl $2, %eax
movl $7, %ecx
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl max2, %ebx
movl $3, %eax
movl $6, %ecx
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl %ebx, max2
movl %edx, max1
movl $4, %eax
movl $5, %ecx
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl max2, %ebx
movl $5, %eax
movl $4, %ecx
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl max2, %ebx
movl $6, %eax
movl $3, %ecx
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl max2, %ebx
movl max1, %esi
movl %edx, max2
movl $7, %eax
movl $2, %ecx
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl max2, %ebx
movl max1, %esi
movl %edx, max2
movl $8, %eax
movl $1, %ecx
movl (%edi,%eax,4), %edx
movl max1, %ebx
movl max2, %ebx
movl $9, %eax
movl $0, %ecx
movl max2, %edx
subl $4, %esp
movl max2, 0(%esp)
subl $4, %esp
movl $formatAf, 0(%esp)
call printf
movl 0(%esp), %ebx
addl $4, %esp
movl 0(%esp), %ebx
addl $4, %esp
movl $1, %eax
movl $0, %ebx
int $0x80
