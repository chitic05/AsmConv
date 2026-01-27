.data
.text
.global _start
_start:
mov $0xff67ffff, %ecx
mov $0x5263dfaa, %eax
cmp %ecx, %eax
jge et_exit
et_1:
mov $4, %ecx
et_exit:
mov $1, %eax
xor %ebx, %ebx
int $0x80