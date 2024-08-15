load $r1, string
syscall 5
exit

.space 128
string:
.byte "Hello, World!"
