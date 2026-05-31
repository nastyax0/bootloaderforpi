.global _start
_start:
    ldr x0, =0x3F201000
    mov w1, #'H'
    str w1, [x0]

    ldr x0, =0x400000
    mov sp, x0

    ldr x0, =kernel_main
    br x0

hang:
    b hang

.globl anticompiler
anticompiler:
    ret



    