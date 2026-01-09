.data
    x: .long 10
.text

.global main

main:
    add x, %eax
    sub $5, %eax
    add %eax, %ebx
