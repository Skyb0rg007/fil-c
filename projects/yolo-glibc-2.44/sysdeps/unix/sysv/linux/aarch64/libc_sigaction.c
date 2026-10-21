/* Copyright (C) 1997-2026 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

/* Required for AArch32 compatibility. */
#define SA_RESTORER	0x04000000

/* Fil-C: always return from handlers through our own restorer rather than the
   vDSO's.  If rt_sigreturn fails (say, a seccomp filter denies it), the vDSO's
   trampoline runs on into whatever follows it, while ours traps.  */
extern void restore_rt (void) asm ("__restore_rt") attribute_hidden;

#define SET_SA_RESTORER(kact, act)		\
 ({						\
    (kact)->sa_flags |= SA_RESTORER;		\
    (kact)->sa_restorer = &restore_rt;		\
 })

#define RESET_SA_RESTORER(act, kact)		\
  (act)->sa_restorer = (kact)->sa_restorer;

#include <sysdeps/unix/sysv/linux/libc_sigaction.c>

/* Debuggers and unwinders recognize this exact instruction sequence as a signal
   trampoline.  Fil-C never lets SIGILL have a handler, so the udf kills us.  */
asm
  (
   "	.text\n"
   "	nop\n"
   "	.type __restore_rt,%function\n"
   "__restore_rt:\n"
   "	mov x8, #" CFI_STRINGIFY (__NR_rt_sigreturn) "\n"
   "	svc #0\n"
   "	udf #0\n"
   "	.size __restore_rt, .-__restore_rt\n"
   );
