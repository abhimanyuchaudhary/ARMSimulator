mov r0, #0 
swi 0x6c
mov r3, r0
mov r4, #1
mov r2, #0
loop:
	mov r0, #0
	swi 0x6c
	add r2, r2, r0
	add r4, r4, #1
	cmp r4, r3
	ble loop
mov r0, #1
mov r1, r2
swi 0x6b
swi 0x11




