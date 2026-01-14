.text
.global _start
_start:
movl $0b11011111111110111111, %eax ; 0b este folosit pentru baza 2
xorl %ebx, %ebx
xorl %ecx, %ecx ; daca aplicam xor intre o valoare si ea insasi, obtinem 0; asadar, aceasta linie este echivalenta cu mov $0, %ecx
movl $32, %edx
et_loop:
testl $1, %eax
jz et_zero
incl %ecx
cmpl %ebx, %ecx
jle et_next
movl %ecx, %ebx
et_next:
shrl $1, %eax
decl %edx
jnz et_loop
jmp et_exit
et_zero:
xorl %ecx, %ecx
jmp et_next
et_exit:
movl %ebx, %eax
movl $1, %ebx
movl $1, %eax
int $0x80