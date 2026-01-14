.data
n: .long 5234
s: .long 0
formatAfSuma: .ascii "Suma cifrelor numarului este: %ld\n"
.text
.global _start
_start:
movl n, %ecx
movl $10, %ebx
et_loop:
cmp $0, %ecx
je et_afisare
movl %ecx, %eax
xorl %edx, %edx
divl %ebx
addl %edx, s
movl %eax, %ecx
jmp et_loop
et_afisare:
pushl s
push $formatAfSuma
call printf
popl %ebx
popl %ebx
pushl stdout
call fflush
addl $4, %esp
et_exit:
movl $1, %eax
xorl %ebx, %ebx
int $0x80