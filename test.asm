load $r1, string
syscall 5
exit

string:
.byte "Hello, World!"
