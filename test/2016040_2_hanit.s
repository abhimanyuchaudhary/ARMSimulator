mov r0, #0
swi 0x6c
mov r1, r0
mov r2, #0
mov r3, #1
loop:
	cmp r1, #1
	ble loop_exit
	add r4, r3, r2
	mov r2, r3
	mov r3, r4
	sub r1, r1, #1

b loop
loop_exit:
mov r1, r2
mov r0, #1
swi 0x6b
swi 0x11