.data
n: .long 10
.text
.global _start
_start:
movl n, %ecx
movl $1, %eax
movl $10, %eax
movl $0, %edx
movl $90, %eax
movl $0, %edx
movl $720, %eax
movl $0, %edx
movl $5040, %eax
movl $0, %edx
movl $30240, %eax
movl $0, %edx
movl $151200, %eax
movl $0, %edx
movl $604800, %eax
movl $0, %edx
movl $1814400, %eax
movl $0, %edx
movl $3628800, %eax
movl $0, %edx
movl $3628800, %eax
movl $0, %edx
movl $1, %eax
movl $0, %ebx
int $0x80
