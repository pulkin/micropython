    .file   "gchelper.s"
    .text

    .align  4
    .global gc_helper_get_regs_and_sp
    .type   gc_helper_get_regs_and_sp, @function
gc_helper_get_regs_and_sp:

    sw   $s0, 0($a0)
    sw   $s1, 4($a0)
    sw   $s2, 8($a0)
    sw   $s3, 12($a0)
    sw   $s4, 16($a0)
    sw   $s5, 20($a0)
    sw   $s6, 24($a0)
    sw   $s7, 28($a0)

    move $v0, $sp
    jr   $ra 

    .size   gc_helper_get_regs_and_sp, .-gc_helper_get_regs_and_sp
