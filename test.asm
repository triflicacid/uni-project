load $r1, 0x0
bz end

load $ret, 0xdead
exit

end:
load $ret, 0xdad
exit
