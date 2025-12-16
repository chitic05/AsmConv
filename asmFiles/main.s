.data

    x: .long 42
    ch: .byte 'a'
.text

.global main

main:
    mov $1, %eax
    mov $0, %ebx
    int $0x80
