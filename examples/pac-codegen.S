/* Compile-only research artifact. Not a boot image and never executed. */
.text
.p2align 2
.globl _unixnu_pac_codegen_only
_unixnu_pac_codegen_only:
  pacia x0, x1
  autia x0, x1
  pacga x2, x0, x1
  ret
