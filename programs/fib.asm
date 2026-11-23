; fib.asm
        LOADI A, 6
        CALL fib
        OUTN
        HALT

fib:    LOADI B, 0
        CMP A, B
        JZ ret_a
        LOADI B, 1
        CMP A, B
        JZ ret_a

        PUSH A
        DEC A
        CALL fib
        POP B
        PUSH A
        MOV A, B
        DEC A
        DEC A
        CALL fib
        POP B
        ADD A, B
ret_a:  RET