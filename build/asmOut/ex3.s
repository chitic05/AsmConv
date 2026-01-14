.data
n: .long 5234
s: .long 0
formatAfSuma: .ascii "Suma cifrelor numarului este: %ld\n"
.text
.global _start
_start:
movl n, %ecx
movl $10, %ebx
movl $5234, %eax
movl $0, %edx
movl $523, %eax
movl $4, %edx
movl $4, s
movl $523, %ecx
movl $523, %eax
movl $0, %edx
movl $52, %eax
movl $3, %edx
movl $7, s
movl $52, %ecx
movl $52, %eax
movl $0, %edx
movl $5, %eax
movl $2, %edx
movl $9, s
movl $5, %ecx
movl $5, %eax
movl $0, %edx
movl $0, %eax
movl $5, %edx
movl $14, s
movl $0, %ecx
subl $4, %esp
movl s, 0(%esp)
subl $4, %esp
movl $formatAfSuma, 0(%esp)
call printf
movl 0(%esp), %ebx
addl $4, %esp
movl 0(%esp), %ebx
addl $4, %esp
subl $4, %esp
movl stdout, 0(%esp)
call fflush
addl $4, %esp
movl $1, %eax
movl $0, %ebx
int $0x80
