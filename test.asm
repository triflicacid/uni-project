push 0xdead
push 4
call print
exit

print:
loadu $r1, 12($fp)
shr $r1, 32
syscall 1
ret
