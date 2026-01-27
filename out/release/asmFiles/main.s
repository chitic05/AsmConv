.data
x: .long 1, 2, 3, 4, 5, 6, 1
n: .long 7

.text
.global _start
_start:
    movl $0, %ebx         
    movl $x, %edi          
    movl $0, %ecx       

et_loop:
    cmpl n, %ecx          
    jge et_exit

    movl (%edi, %ecx, 4), %eax
    cmpl $4, %eax         
    je et_exit

    addl %eax, %ebx
    incl %ecx
    jmp et_loop

et_exit:
    movl $1, %eax        
    int $0x80             
