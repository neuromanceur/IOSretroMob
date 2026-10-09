.syntax unified
.arm
.section .text.start,"ax",%progbits
.global _start
_start:
 ldr r0,[sp]
 add r1,sp,#4
 bl main
1: b 1b
.global api_dlopen
api_dlopen:
 ldr ip,2f
 ldr pc,[ip]
2: .word 0x20000
.global api_dlsym
api_dlsym:
 ldr ip,3f
 ldr pc,[ip]
3: .word 0x20004
.section .data.imports,"aw",%progbits
.word 0,0
