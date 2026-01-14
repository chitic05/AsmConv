.data
.text
.global _start
_start:
movl $0, %ecx
movl $3, %eax ; incarca 3 in EAX
movl $5, %ebx ; incarca 5 in EBX
cmp %ebx, %eax ; compara EAX cu EBX
jge greater_or_equal ; sare la ’greater_or_equal’ daca EAX >= EBX
# Cod pentru cazul in care EAX < EBX
movl $1, %ecx ; setam ECX la 1
jmp end ; sare la sfarsit
greater_or_equal:
# Cod pentru cazul in care EAX >= EBX
movl $2, %ecx ; setam ECX la 2
end:
movl $1, %eax
xor %ebx, %ebx
int $0x80