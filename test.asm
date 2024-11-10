jal debug_print
exit 42

debug_print:
load $r1, message
syscall 5
jmp $rip

message:
.byte "Hello!"
