%include lib:syscall

main:
    ; get number of arguments
    print_str s_num_args
    print_int (0x0) ;(0xfffff)
    exit

s_num_args:
    .byte "number of arguments: "
