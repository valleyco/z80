        .cpu Z80

        org     0000h

start:
        ld      a,0B6h
        out     (14h),a

        xor     a
        out     (11h),a

        ld      a,01h
        out     (12h),a

        ld      a,80h
        out     (10h),a

        halt

        ld      a,0EEh
        out     (14h),a
        halt

        org     0066h

nmi_handler:
        ld      a,(8001h)
        or      a
        jr      nz,nmi_done

        in      a,(13h)
        ld      (8000h),a
        ld      a,01h
        ld      (8001h),a
        ld      a,0D0h
        out     (10h),a
        halt

nmi_done:
        ld      a,(8000h)
        out     (14h),a
        halt