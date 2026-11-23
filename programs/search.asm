; search.asm
LOADI A, 7
STORE [0x800], A
LOADI A, 3
STORE [0x801], A
LOADI A, 9
STORE [0x802], A
LOADI A, 4
STORE [0x803], A
LOADI A, 12
STORE [0x804], A

LOADI H, 0x800
LOADI B, 9

loop:   LOAD A, [H]
        CMP A, B
        JZ found
        PUSH B
        LOADI B, 0
        CMP A, B
        POP B
        JZ missing
        INC H
        JMP loop

found:  HLOW
        OUTN
        HALT

missing: LOADI A, 63
        OUT
        HALT