.data
    x: .long 1, 0, 31
    y: .long 0
.text

.global main

main:
    add $x, %eax
    add 8(%eax), %ebx

