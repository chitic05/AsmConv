.data
    ch: .long 32, 100, 41
.text

.global main

main:
    mov $1, %eax
    mov $0, %ebx
    int $0x80
