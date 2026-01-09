.data
    x: .long 3
.text

.global main

main:
    add x, %eax
    sub $2, %eax
    movb %al, %bl
