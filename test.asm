%include lib:syscall
main:
    ; get number of arguments
    print_str s_num_args
    load $r1, (0x0) ;(0xfffff)
    add.d $r1, 10
    syscall 1
    exitz 42
    exit

s_num_args:
    .byte "number of arguments: "
