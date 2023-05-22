global input_asm
global output_asm
global exit_asm
global sqrt_asm

global stub_entry

section .text

input_asm:
        sub     rsp, 88
        mov     edx, 7
        mov     edi, 1
        mov  rax, 9071333137731145
        lea     rsi, [rsp+8]
        mov     QWORD  [rsp+8], rax
        mov rax, 1
        syscall
        mov     edx, 63
        lea     rsi, [rsp+16]
        xor     edi, edi
        mov rax, 0
        syscall
        movsx   edx, BYTE  [rsp+16]
        cmp     dl, 45
        je      .L18
        xor     esi, esi
        xor     ecx, ecx
        cmp     dl, 10
        je      .L16
.L6:
        lea     rax, [rsp+16]
        add     rcx, rax
        xor     eax, eax
.L4:
        sub     edx, 48
        lea     rax, [rax+rax*4]
        add     rcx, 1
        movsx   rdx, edx
        lea     rax, [rdx+rax*2]
        movsx   edx, BYTE  [rcx]
        cmp     dl, 10
        jne     .L4
        test    sil, sil
        je      .L19
        imul    rax, rax, -100
        add     rsp, 88
        ret
.L19:
        lea     rax, [rax+rax*4]
        add     rsp, 88
        lea     rax, [rax+rax*4]
        sal     rax, 2
        ret
.L18:
        movsx   edx, BYTE  [rsp+17]
        mov     esi, 1
        mov     ecx, 1
        cmp     dl, 10
        jne     .L6
.L16:
        xor     eax, eax
        add     rsp, 88
        ret

output_asm:
        mov  rax, 2322261283259569487
        push    rbx
        mov     edx, 8
        mov     rbx, rdi
        mov     edi, 1
        sub     rsp, 96
        lea     rsi, [rsp+7]
        mov     QWORD  [rsp+7], rax
        mov     BYTE  [rsp+15], 0
        mov rax, 1
        syscall
        mov     rdi, rbx
        pxor    xmm0, xmm0
        mov  rax, 2951479051793528259
        neg     rdi
        movups   [rsp+64], xmm0
        cmovs   rdi, rbx
        mov     BYTE  [rsp+80], 10
        mov     BYTE  [rsp+77], 46
        mov     rdx, rdi
        movups   [rsp+16], xmm0
        shr     rdx, 2
        movups   [rsp+32], xmm0
        mul     rdx
        movups   [rsp+48], xmm0
        mov     rsi, rdx
        shr     rsi, 2
        lea     rax, [rsi+rsi*4]
        mov     rcx, rsi
        lea     rax, [rax+rax*4]
        sal     rax, 2
        sub     rdi, rax
        mov     rsi, rdi
        mov  rdi, -3689348814741910323
        mov     rax, rsi
        mul     rdi
        shr     rdx, 3
        lea     rax, [rdx+rdx*4]
        add     edx, 48
        add     rax, rax
        mov     BYTE  [rsp+78], dl
        sub     rsi, rax
        add     esi, 48
        mov     BYTE  [rsp+79], sil
        test    rcx, rcx
        je      .L30
        lea     r8, [rsp+76]
        mov     rsi, r8
.L28:
        mov     rax, rcx
        mul     rdi
        shr     rdx, 3
        lea     rax, [rdx+rdx*4]
        add     rax, rax
        sub     rcx, rax
        add     ecx, 48
        mov     BYTE  [rsi], cl
        mov     rcx, rdx
        mov     rdx, rsi
        sub     rsi, 1
        test    rcx, rcx
        jne     .L28
        lea     eax, [r8+4]
        sub     eax, edx
        lea     edx, [rax+1]
.L27:
        test    rbx, rbx
        jns     .L29
        mov     ecx, 63
        sub     ecx, eax
        movsx   rax, ecx
        mov     BYTE  [rsp+16+rax], 45
        mov     eax, edx
        add     edx, 1
.L29:
        cdqe
        mov     ecx, 64
        movsx   rdx, edx
        mov     edi, 1
        sub     rcx, rax
        lea     rsi, [rsp+16+rcx]
        mov rax, 1
        syscall
        add     rsp, 96
        pop     rbx
        ret
.L30:
        mov     edx, 4
        mov     eax, 3
        jmp     .L27

exit_asm:
        mov rax, 0x3c
        mov rdi, 0
        syscall

sqrt_asm:
        cvtsi2sd  xmm1, rdi
        sqrtsd    xmm1, xmm1
        cvttsd2si  rdi, xmm1
        imul rax, rdi, 10
        ret

stub_entry:
        push    rbp
        mov     rbp, rsp
        sub     rsp, 80
        mov  rax, 8391086210778428489
        mov  rdx, 8245921732014730340
        mov     QWORD  [rbp-80], rax
        mov     QWORD  [rbp-72], rdx
        mov  rax, 7310312400256061984
        mov  rdx, 7885630463374418252
        mov     QWORD  [rbp-64], rax
        mov     QWORD  [rbp-56], rdx
        mov  rax, 7953674092612970864
        mov  rdx, 8675375941656911972
        mov     QWORD  [rbp-48], rax
        mov     QWORD  [rbp-40], rdx
        mov  rax, 7308324466020672357
        mov  rdx, 5989838517548181024
        mov     QWORD  [rbp-32], rax
        mov     QWORD  [rbp-24], rdx
        mov  rax, 2948282220237600
        mov     QWORD  [rbp-18], rax
        lea     rax, [rbp-80]
        mov     edx, 70
        mov     rsi, rax
        mov     edi, 1
        mov     rax, 1
        syscall

        jmp exit_asm