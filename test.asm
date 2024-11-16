%include lib:syscall

loadi $r5, 0x778899AABBDDEEFF
print_hex $r5
print_endl

zext $r6, $r5, 16
print_hex $r6
print_endl

sext $r7, $r5, 16
print_hex $r7
print_endl

exit
