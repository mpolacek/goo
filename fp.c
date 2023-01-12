struct S { int a, b, c; char pad[300]; };

void f1 (struct S *);

int f2 (void)
{
  struct S s = { 1, 2, 3 };
  f1 (&s);
  return s.a + s.b + s.c;
}

#if 0
Without frame pointers:

0000000000000000 <f2>:
   0:	48 81 ec 48 01 00 00 	sub    $0x148,%rsp
   7:	31 c0                	xor    %eax,%eax
   9:	b9 27 00 00 00       	mov    $0x27,%ecx
   e:	48 89 e7             	mov    %rsp,%rdi
  11:	f3 48 ab             	rep stos %rax,%es:(%rdi)
  14:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 1b <f2+0x1b>
			17: R_X86_64_PC32	.LC0-0x4
  1b:	48 89 e7             	mov    %rsp,%rdi
  1e:	c7 44 24 08 03 00 00 	movl   $0x3,0x8(%rsp)
  25:	00 
  26:	48 89 04 24          	mov    %rax,(%rsp)
  2a:	e8 00 00 00 00       	call   2f <f2+0x2f>
			2b: R_X86_64_PLT32	f1-0x4
  2f:	8b 44 24 04          	mov    0x4(%rsp),%eax
  33:	03 04 24             	add    (%rsp),%eax
  36:	03 44 24 08          	add    0x8(%rsp),%eax
  3a:	48 81 c4 48 01 00 00 	add    $0x148,%rsp
  41:	c3                   	ret    

With frame pointers:

0000000000000000 <f2>:
   0:	55                   	push   %rbp
   1:	31 c0                	xor    %eax,%eax
   3:	b9 27 00 00 00       	mov    $0x27,%ecx
   8:	48 89 e5             	mov    %rsp,%rbp
   b:	48 81 ec 40 01 00 00 	sub    $0x140,%rsp
  12:	48 8d bd c0 fe ff ff 	lea    -0x140(%rbp),%rdi
  19:	f3 48 ab             	rep stos %rax,%es:(%rdi)
  1c:	48 8b 05 00 00 00 00 	mov    0x0(%rip),%rax        # 23 <f2+0x23>
			1f: R_X86_64_PC32	.LC0-0x4
  23:	48 8d bd c0 fe ff ff 	lea    -0x140(%rbp),%rdi
  2a:	c7 85 c8 fe ff ff 03 	movl   $0x3,-0x138(%rbp)
  31:	00 00 00 
  34:	48 89 85 c0 fe ff ff 	mov    %rax,-0x140(%rbp)
  3b:	e8 00 00 00 00       	call   40 <f2+0x40>
			3c: R_X86_64_PLT32	f1-0x4
  40:	8b 85 c4 fe ff ff    	mov    -0x13c(%rbp),%eax
  46:	03 85 c0 fe ff ff    	add    -0x140(%rbp),%eax
  4c:	03 85 c8 fe ff ff    	add    -0x138(%rbp),%eax
  52:	c9                   	leave  
  53:	c3                   	ret    
#endif
