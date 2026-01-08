.data
    x: .long 0b00000001, 100, 41
    y: .long 10
.text

.global main

main:
    add x, x
    add x, %ebx
