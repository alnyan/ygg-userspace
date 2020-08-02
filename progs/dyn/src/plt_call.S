.section .text
.global object_resolver_entry
object_resolver_entry:
    xchgq (%rsp), %rdi
    xchgq 8(%rsp), %rsi

    pushq %rdx
    pushq %rcx
    pushq %r8
    pushq %r9

    call object_resolver

    popq %r9
    popq %r8
    popq %rcx
    popq %rdx

    popq %rsi
    popq %rdi

    jmpq *%rax
