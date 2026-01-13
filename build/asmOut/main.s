.data
x: .long 77
.text
.global main
main:
movl $1048576, %esp
movl x, %eax
movl $1, %ebx
mov $2, %ebx
mov $4, %ebx
mov $8, %ebx
mov $16, %ebx
mov $32, %ebx
mov $64, %ebx
mov $128, %ebx
movl $1, %eax
mov $0, %ebx
int $0x80
