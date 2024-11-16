; =========================
; =  Macros for Syscalls  =
; =========================

%macro print_hex val
    load $r1, val
    syscall 0
%end

%macro print_int val
    load $r1, val
    syscall 1
%end

%macro print_float val
    load $r1, val
    syscall 2
%end

%macro print_double val
    load $r1, val
    syscall 3
%end

%macro print_char ch
    load $r1, ch
    syscall 4
%end

%macro print_endl
    load $r1, 0x0d0a ; "\r\n"
    syscall 4
%end

%macro print_str str
    load $r1, str
    syscall 5
%end

%macro read_int
    syscall 6
%end

%macro read_float
    syscall 7
%end

%macro read_double
    syscall 8
%end

%macro read_char
    syscall 9
%end

%macro read_str dst maxlen
    load $r1, dst
    load $r2, maxlen
    syscall 10
%end
