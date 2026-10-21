	nop
.global __restore_rt
.hidden __restore_rt
.type __restore_rt,@function
__restore_rt:
	mov $15, %rax
	syscall
	# If rt_sigreturn fails (say, a seccomp filter denies it), we must not run on into
	# whatever follows. Fil-C never lets SIGILL have a handler, so this kills us.
	ud2
.size __restore_rt,.-__restore_rt
