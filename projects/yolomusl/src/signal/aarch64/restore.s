.global __restore
.hidden __restore
.type __restore,%function
__restore:
.global __restore_rt
.hidden __restore_rt
.type __restore_rt,%function
__restore_rt:
	mov x8,#139 // SYS_rt_sigreturn
	svc 0
	// If rt_sigreturn fails (say, a seccomp filter denies it), we must not run on into
	// whatever follows. Fil-C never lets SIGILL have a handler, so this kills us.
	udf #0
