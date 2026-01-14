.data
n: .long 6
v: .word 10, 20, 40, 60, 100, 5
.text
.global _start
_start:
lea v, %edi
mov $1, %ecx
mov (%edi, %ecx, 4), %edx
etexit:
mov $1, %eax
mov $0, %ebx
int $0x80