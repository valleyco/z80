        .cpu Z80

        org     0000h

start:
        ld      a,80h
        out     (14h),a

        in      a,(07h)
        cp      04h
        jr      nz,fail

        ld      a,0B6h
        out     (14h),a
        halt

fail:
        ld      a,0A6h
        out     (14h),a
        halt