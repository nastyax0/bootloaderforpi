.global drop_to_el1
.global el1_entry
.global el1_vectors
.align 11

drop_to_el1:

    // EL1 is AArch64
    mrs x0, scr_el3
    orr x0, x0, #(1 << 10)
    msr scr_el3, x0

    // clean EL1 state
    msr sctlr_el1, xzr

    // EL1 stack
    ldr x0, =0x400000
    msr sp_el1, x0

    // EL1h mode
    mov x0, #0x5
    msr spsr_el3, x0

    // EL1 vectors
    ldr x0, =el1_vectors
    msr vbar_el1, x0

    // EL1 entry
    ldr x0, =el1_entry
    msr elr_el3, x0

    isb
    eret


el1_entry:

    ldr x0, =0x400000
    mov sp, x0

    bl uart_init

    mov x0, #'E'
    bl uart_send

    mov x0, #'L'
    bl uart_send

    mov x0, #'1'
    bl uart_send

    mov x0, #'J'
    bl uart_send

    mov x0, #'\n'
    bl uart_send
    
    ldr x0, =0x200000
    br x0

el1_vectors:
    b .
    .space 0x80
    b .
    .space 0x80
    b .
    .space 0x80
    b .