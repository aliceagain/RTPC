; G8RTOS_SchedulerASM.s
; Holds all ASM functions needed for the scheduler
; Note: If you have an h file, do not have a C file and an S file of the same name

	; Functions Defined
	.def G8RTOS_Start, PendSV_Handler

	; Dependencies
	.ref CurrentlyRunningThread, G8RTOS_Scheduler

	.thumb		; Set to thumb mode
	.align 2	; Align by 2 bytes (thumb mode uses allignment by 2 or 4)
	.text		; Text section

; Need to have the address defined in file 
; (label needs to be close enough to asm code to be reached with PC relative addressing)
RunningPtr: .field CurrentlyRunningThread, 32

; G8RTOS_Start
;	Sets the first thread to be the currently running thread
;	Starts the currently running thread by setting Link Register to tcb's Program Counter
G8RTOS_Start:

	.asmfunc
	ldr r0, RunningPtr
	ldr r0, [r0]
	ldr SP, [r0]

	add SP, #56
	pop {LR}
	add SP, #4

    cpsie i

	bx LR
	.endasmfunc

; PendSV_Handler
; - Performs a context switch in G8RTOS
; 	- Saves remaining registers into thread stack
;	- Saves current stack pointer to tcb
;	- Calls G8RTOS_Scheduler to get new tcb
;	- Set stack pointer to new stack pointer from new tcb
;	- Pops registers from thread stack
PendSV_Handler:
	
	.asmfunc
	CPSID I

	push {r11,r10,r9,r8,r7,r6,r5,r4}

	ldr r0, RunningPtr
	ldr r0, [r0]
	str SP, [r0]

	push {LR}
	bl G8RTOS_Scheduler
	pop {LR}

	ldr r0, RunningPtr
	ldr r0, [r0]
	ldr SP, [r0]

	pop {r11,r10,r9,r8,r7,r6,r5,r4}
	CPSIE I
	bx LR
	.endasmfunc
	
	; end of the asm file
	.align
	.end
