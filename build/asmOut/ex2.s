.data
n: .long 6
s: .long 0
.text
.global main
main:
movl $1048576, %esp
movl n, %ecx
mov $0, %eax
mov $0, %ebx
movl %ecx, %eax
mov $36, %eax
mov $0, %edx
mov $36, %ebx
mov $5, %ecx
movl %ecx, %eax
mov $25, %eax
mov $0, %edx
mov $61, %ebx
mov $4, %ecx
movl %ecx, %eax
mov $16, %eax
mov $0, %edx
mov $77, %ebx
mov $3, %ecx
movl %ecx, %eax
mov $9, %eax
mov $0, %edx
mov $86, %ebx
mov $2, %ecx
movl %ecx, %eax
mov $4, %eax
mov $0, %edx
mov $90, %ebx
mov $1, %ecx
movl %ecx, %eax
mov $1, %eax
mov $0, %edx
mov $91, %ebx
mov $0, %ecx
movl %ebx, s
movl $1, %eax
mov $0, %ebx
int $0x80
