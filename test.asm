xor $imr, 0x1
or $isr, 0x3
load $r1, string_after
syscall 5
exit

string_interrupt:
.byte "Interrupt!\n"
string_after:
.byte "Program End.\n"

interrupt_handler:
load $r1, string_interrupt
syscall 5
zero $isr
rti
