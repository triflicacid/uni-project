%include lib:stdio

jal print
exit

print:
load $r1, message
syscall 5
jmp $rip

message:
.byte "Hello!"
