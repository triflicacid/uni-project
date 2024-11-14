%include lib:syscall
main:
    ; get number of arguments
    print_str s_num_args
    load $r1, (0x0) ;(0xfffff)
    syscall 1
    exit

s_num_args:
    .byte "number of arguments: "
