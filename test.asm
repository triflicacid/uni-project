load $r1, 0x0
jmpz end

load $ret, 0xdead
exit

end:
load $ret, 0xdad
exit
