xor $imr, 0x1
or $isr, 0x3
load $r1, after
syscall 5
exit

ininterrupt:
.byte "Interrupt!\n"
after:
.byte "Program End.\n"

.org 0x400
load $r1, ininterrupt
syscall 5
zero $isr
rti
