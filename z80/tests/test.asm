        .cpu Z80

; @test nop
; @expect pc=0102h
        org 0100h
nop_test:
        nop
        halt

; @test ld_r_r
; @init bc=00ABh
; @expect bc=ABABh
ld_r_r_test:
        ld      b,c
        halt

; @test alu_add
; @init af=1000h bc=2000h
; @expect a=30h
alu_add_test:
        add     a,b
        halt

; @test ld_rp_nn
; @init sp=8000h
; @expect hl=1234h pc=010Ah
ld_rp_nn_test:
        ld      hl,1234h
        halt

; @test jp_nn
; @expect pc=010Fh
jp_nn_test:
        jp      jp_nn_done
        nop
jp_nn_done:
        halt

; @test jr_cc_taken
; @init af=0040h
; @expect pc=0114h
jr_cc_taken_test:
        jr      z,jr_cc_target
        nop
        nop
jr_cc_target:
        halt

; @test halt
; @expect halted=1 pc=0115h
halt_test:
        halt

; @test cb_rlc_b
; @init bc=8000h
; @expect bc=0100h flag.c=1
cb_rlc_b_test:
        rlc     b
        halt

; @test ed_in_c
; @init bc=0012h
; @io 12h=5Ah
; @expect bc=5A12h
ed_in_c_test:
        in      b,(c)
        halt

; @test ldir
; @init hl=1000h de=2000h bc=0002h sp=8000h
; @mem 1000h=11h 1001h=22h
; @expect bc=0000h
; @expect_mem 2000h=11h 2001h=22h
ldir_test:
        ldir
        halt

; @test dd_ld_r_mem
; @init ix=3000h sp=8000h
; @mem 3005h=99h
; @expect a=99h
dd_ld_r_mem_test:
        ld      a,(ix+5)
        halt

; @test daa
; @init af=0900h
; @expect a=09h
daa_test:
        daa
        halt

; @test ex_isp_hl
; @init sp=8000h hl=1234h
; @mem 8000h=78h 8001h=56h
; @expect hl=5678h
; @expect_mem 8000h=34h 8001h=12h
ex_isp_hl_test:
        ex      (sp),hl
        halt

; @test ed_neg
; @init af=0100h
; @expect a=FFh
ed_neg_test:
        neg
        halt

; @test ed_adc_hl
; @init hl=1000h de=0200h af=0001h
; @expect hl=1201h
ed_adc_hl_test:
        adc     hl,de
        halt

; @test ed_ld_rp_inn
; @init sp=8000h
; @mem 4000h=34h 4001h=12h
; @expect bc=1234h
ed_ld_rp_inn_test:
        ld      bc,(4000h)
        halt

; @test ed_cpi
; @init af=5500h hl=1000h bc=0001h sp=8000h
; @mem 1000h=55h
; @expect hl=1001h bc=0000h flag.z=1
ed_cpi_test:
        cpi
        halt

; @test ld_ixh
; @init ix=0000h sp=8000h
; @expect ix=0200h
ld_ixh_test:
        db      0xDD
        ld      h,2
        halt

; @test trap_fallthrough
; @expect trap=1
trap_fallthrough_test:
        nop

; @test trap_bad_jp
; @expect trap=1
trap_bad_jp_test:
        jp      8000h
        halt

; @test ok_data_outside
; @mem 3005h=99h
; @expect a=99h
ok_data_outside_test:
        ld      a,(3005h)
        halt

; ---- expanded coverage ----

; @test sub_a_n
; @init af=3000h
; @expect a=10h flag.n=1 flag.c=0
sub_a_n_test:
        sub     20h
        halt

; @test and_a_r
; @init a=F0h b=0Fh
; @expect a=00h flag.z=1 flag.h=1 flag.c=0
and_a_r_test:
        and     b
        halt

; @test xor_a_r
; @init af=5500h bc=FF00h
; @expect a=AAh flag.z=0 flag.c=0
xor_a_r_test:
        xor     b
        halt

; @test or_a_r
; @init af=0F00h bc=F000h
; @expect a=FFh
or_a_r_test:
        or      b
        halt

; @test cp_a_n
; @init af=1000h
; @expect a=10h flag.z=1 flag.n=1
cp_a_n_test:
        cp      10h
        halt

; @test inc_r
; @init bc=00FFh
; @expect bc=0000h flag.z=1
inc_r_test:
        inc     c
        halt

; @test dec_rp
; @init hl=0001h
; @expect hl=0000h
dec_rp_test:
        dec     hl
        halt

; @test ld_r_n
; @expect a=42h
ld_r_n_test:
        ld      a,42h
        halt

; @test ld_hl_mem
; @init hl=2000h
; @mem 2000h=77h
; @expect a=77h
ld_hl_mem_test:
        ld      a,(hl)
        halt

; @test scf_ccf
; @init af=0000h
; @expect flag.c=0 flag.n=0
scf_ccf_test:
        scf
        ccf
        halt

; @test cpl
; @init af=0F00h
; @expect a=F0h flag.n=1 flag.h=1
cpl_test:
        cpl
        halt

; @test rlca
; @init af=8000h
; @expect a=01h flag.c=1
rlca_test:
        rlca
        halt

; @test push_pop
; @init sp=8000h af=1234h
; @expect af=1234h sp=8000h
push_pop_test:
        push    af
        pop     af
        halt

; @test call_ret
; @init sp=8000h
; @expect a=55h
call_ret_test:
        jr      call_ret_main
call_ret_sub:
        ld      a,55h
        ret
call_ret_main:
        call    call_ret_sub
        halt

; @test jr_cc_not_taken
; @init af=0000h
; @expect a=01h
jr_cc_not_taken_test:
        jr      z,jr_cc_nt_skip
        ld      a,01h
        halt
jr_cc_nt_skip:
        ld      a,FFh
        halt

; @test djnz_loop
; @init b=02h c=00h
; @expect b=00h
djnz_loop_test:
        ld      a,00h
djnz_loop_body:
        inc     a
        djnz    djnz_loop_body
        halt

; @test ex_de_hl
; @init de=1111h hl=2222h
; @expect de=2222h hl=1111h
ex_de_hl_test:
        ex      de,hl
        halt

; @test exx
; @init bc=1111h de=2222h hl=3333h bc_=AAAAh de_=BBBBh hl_=CCCCh
; @expect bc=AAAAh de=BBBBh hl=CCCCh
exx_test:
        exx
        halt

; @test jp_hl
; @expect a=99h
jp_hl_test:
        ld      hl,jp_hl_target
        jp      (hl)
        nop
jp_hl_target:
        ld      a,99h
        halt

; @test bit_set_res
; @init bc=0000h
; @expect bc=8000h flag.z=0
bit_set_res_test:
        set     7,b
        bit     7,b
        halt

; @test cb_srl
; @init bc=0100h
; @expect bc=0000h flag.c=1 flag.z=1
cb_srl_test:
        srl     b
        halt

; @test ed_sbc_hl
; @init hl=1000h bc=0001h af=0001h
; @expect hl=0FFEh
ed_sbc_hl_test:
        sbc     hl,bc
        halt

; @test ed_ldi
; @init hl=1000h de=2000h bc=0001h
; @mem 1000h=ABh
; @expect bc=0000h hl=1001h de=2001h
; @expect_mem 2000h=ABh
ed_ldi_test:
        ldi
        halt

; @test ed_out_c
; @init bc=1234h
; @io 34h=00h
; @expect b=56h
ed_out_c_test:
        ld      b,56h
        out     (c),b
        in      b,(c)
        halt

; @test ed_ld_inn_rp
; @init bc=89ABh sp=8000h
; @expect_mem 5000h=ABh 5001h=89h
ed_ld_inn_rp_test:
        ld      (5000h),bc
        halt

; @test ed_im1
; @expect im=1
ed_im1_test:
        im      1
        halt

; @test ed_rld
; @init af=1200h hl=4000h
; @mem 4000h=34h
; @expect a=13h
; @expect_mem 4000h=42h
ed_rld_test:
        rld
        halt

; @test ed_rrd
; @init af=1200h hl=4000h
; @mem 4000h=34h
; @expect a=14h
; @expect_mem 4000h=23h
ed_rrd_test:
        rrd
        halt

; @test fd_ld_iy
; @init iy=2000h
; @mem 2003h=66h
; @expect a=66h
fd_ld_iy_test:
        ld      a,(iy+3)
        halt

; @test dd_cb_bit
; @init ix=3000h
; @mem 3002h=08h
; @expect flag.z=0
dd_cb_bit_test:
        bit     3,(ix+2)
        halt

; @test rst_38
; @init sp=8000h
; @expect trap=1
rst_38_test:
        rst     38h
        halt

; @test di_ei
; @expect iff1=1 iff2=1
di_ei_test:
        di
        ei
        halt

; @test ld_sp_hl
; @init hl=9000h
; @expect sp=9000h
ld_sp_hl_test:
        ld      sp,hl
        halt

; @test ld_ihl_n
; @init hl=4000h
; @expect_mem 4000h=5Ah
ld_ihl_n_test:
        ld      (hl),5Ah
        halt

; @test ld_ihl_r
; @init hl=4000h bc=B200h
; @expect_mem 4000h=B2h
ld_ihl_r_test:
        ld      (hl),b
        halt

; @test inc_ihl
; @init hl=4000h
; @mem 4000h=10h
; @expect_mem 4000h=11h
inc_ihl_test:
        inc     (hl)
        halt

; @test dec_ihl
; @init hl=4000h
; @mem 4000h=10h
; @expect_mem 4000h=0Fh
dec_ihl_test:
        dec     (hl)
        halt

; @test alu_a_ihl
; @init af=1000h hl=4000h
; @mem 4000h=22h
; @expect a=32h
alu_a_ihl_test:
        add     a,(hl)
        halt

; @test dd_ld_ixd_n
; @init ix=3000h
; @expect_mem 3005h=A5h
dd_ld_ixd_n_test:
        ld      (ix+5),0A5h
        halt

; @test dd_ld_h_ixd
; @init ix=4000h hl=0000h
; @mem 4001h=12h 4000h=34h
; @expect hl=1234h ix=4000h
dd_ld_h_ixd_test:
        ld      h,(ix+1)
        ld      l,(ix+0)
        halt

; @test dd_ld_ixd_h
; @init ix=4000h hl=A1B2h
; @expect_mem 4003h=A1h
dd_ld_ixd_h_test:
        ld      (ix+3),h
        halt

; @test add_hl_rp
; @init hl=1000h bc=0234h
; @expect hl=1234h
add_hl_rp_test:
        add     hl,bc
        halt

; @test ret_cc_taken
; @init sp=8000h af=0040h
; @expect a=99h
ret_cc_taken_test:
        jr      ret_cc_main
ret_cc_help:
        ret     z
        ld      a,00h
        ret
ret_cc_main:
        call    ret_cc_help
        ld      a,99h
        halt

; ---- broader instruction coverage ----

; @test adc_a_n
; @init af=1001h
; @expect a=31h flag.c=0
adc_a_n_test:
        adc     a,20h
        halt

; @test sbc_a_r
; @init af=3001h bc=1000h
; @expect a=1Fh flag.n=1
sbc_a_r_test:
        sbc     a,b
        halt

; @test add_a_n
; @init af=F000h
; @expect a=10h flag.c=1
add_a_n_test:
        add     a,20h
        halt

; @test rrca
; @init af=0100h
; @expect a=80h flag.c=1
rrca_test:
        rrca
        halt

; @test rla
; @init af=8001h
; @expect a=01h flag.c=1
rla_test:
        rla
        halt

; @test rra
; @init af=0100h
; @expect a=00h flag.c=1
rra_test:
        rra
        halt

; @test ld_a_inn
; @mem 4000h=A5h
; @expect a=A5h
ld_a_inn_test:
        ld      a,(4000h)
        halt

; @test ld_inn_a
; @init af=5A00h
; @expect_mem 4000h=5Ah
ld_inn_a_test:
        ld      (4000h),a
        halt

; @test ld_a_ibc
; @init bc=4000h
; @mem 4000h=C3h
; @expect a=C3h
ld_a_ibc_test:
        ld      a,(bc)
        halt

; @test ld_ide_a
; @init de=4000h af=B100h
; @expect_mem 4000h=B1h
ld_ide_a_test:
        ld      (de),a
        halt

; @test in_a_n
; @io 55h=77h
; @expect a=77h
in_a_n_test:
        in      a,(55h)
        halt

; @test out_n_a
; @init af=9900h
; @io 55h=00h
; @expect a=99h
out_n_a_test:
        out     (55h),a
        in      a,(55h)
        halt

; @test ex_af_af
; @init af=1111h af_=2222h
; @expect af=2222h
ex_af_af_test:
        ex      af,af'
        halt

; @test jp_cc_taken
; @init af=0040h
; @expect a=42h
jp_cc_taken_test:
        jp      z,jp_cc_yes
        ld      a,00h
        jr      jp_cc_taken_end
jp_cc_yes:
        ld      a,42h
jp_cc_taken_end:
        halt

; @test jp_cc_not_taken
; @init af=0000h
; @expect a=01h
jp_cc_not_taken_test:
        jp      z,jp_cc_no
        ld      a,01h
        jr      jp_cc_nt_end
jp_cc_no:
        ld      a,FFh
jp_cc_nt_end:
        halt

; @test call_cc_taken
; @init sp=8000h af=0040h
; @expect a=77h
call_cc_taken_test:
        jr      call_cc_main
call_cc_sub:
        ld      a,77h
        ret
call_cc_main:
        call    z,call_cc_sub
        halt

; @test call_cc_not_taken
; @init sp=8000h af=0000h
; @expect a=01h
call_cc_not_taken_test:
        jr      call_cc_nt_main
call_cc_skip:
        ld      a,FFh
        ret
call_cc_nt_main:
        call    z,call_cc_skip
        ld      a,01h
        halt

; @test cb_rrc
; @init bc=0100h
; @expect bc=8000h flag.c=1
cb_rrc_test:
        rrc     b
        halt

; @test cb_rl
; @init bc=8000h af=0001h
; @expect bc=0100h flag.c=1
cb_rl_test:
        rl      b
        halt

; @test cb_rr
; @init bc=0100h af=0000h
; @expect bc=0000h flag.c=1
cb_rr_test:
        rr      b
        halt

; @test cb_sla
; @init bc=4000h
; @expect bc=8000h flag.c=0
cb_sla_test:
        sla     b
        halt

; @test cb_sra
; @init bc=8000h
; @expect bc=C000h flag.c=0
cb_sra_test:
        sra     b
        halt

; @test cb_res
; @init bc=FF00h
; @expect bc=7F00h
cb_res_test:
        res     7,b
        halt

; @test lddr
; @init hl=1001h de=2001h bc=0002h sp=8000h
; @mem 1000h=11h 1001h=22h
; @expect bc=0000h
; @expect_mem 2000h=11h 2001h=22h
lddr_test:
        lddr
        halt

; @test cpir
; @init af=5500h hl=1000h bc=0003h sp=8000h
; @mem 1000h=11h 1001h=55h 1002h=22h
; @expect hl=1002h bc=0001h flag.z=1
cpir_test:
        cpir
        halt

; @test ed_retn
; @init sp=8000h iff1=0 iff2=1
; @expect iff1=1
ed_retn_test:
        ld      hl,ed_retn_halt
        push    hl
        retn
ed_retn_halt:
        halt

; @test ed_reti
; @init sp=8000h iff1=0 iff2=1
; @expect iff1=1 a=33h
ed_reti_test:
        ld      hl,ed_reti_cont
        push    hl
        reti
ed_reti_cont:
        ld      a,33h
        halt

; @test ed_im2
; @expect im=2
ed_im2_test:
        im      2
        halt

; @test ed_im0
; @expect im=0
ed_im0_test:
        im      1
        im      0
        halt

; @test ld_i_a
; @init af=AB00h
; @expect i=ABh
ld_i_a_test:
        ld      i,a
        halt

; @test ld_a_i
; @init i=5Ch af=0000h iff2=1
; @expect a=5Ch flag.pv=1
ld_a_i_test:
        ld      a,i
        halt

; @test add_ix_rp
; @init ix=1000h bc=0234h
; @expect ix=1234h
add_ix_rp_test:
        add     ix,bc
        halt

; @test add_iy_rp
; @init iy=2000h de=0100h
; @expect iy=2100h
add_iy_rp_test:
        add     iy,de
        halt

; @test push_pop_ix
; @init sp=8000h ix=1234h
; @expect ix=1234h sp=8000h
push_pop_ix_test:
        push    ix
        ld      ix,0000h
        pop     ix
        halt

; @test jp_ix
; @expect a=88h
jp_ix_test:
        ld      ix,jp_ix_target
        jp      (ix)
        nop
jp_ix_target:
        ld      a,88h
        halt

; @test ld_sp_ix
; @init ix=9000h
; @expect sp=9000h
ld_sp_ix_test:
        ld      sp,ix
        halt

; @test inc_ixd
; @init ix=3000h
; @mem 3004h=10h
; @expect_mem 3004h=11h
inc_ixd_test:
        inc     (ix+4)
        halt

; @test dec_iyd
; @init iy=3000h
; @mem 3006h=10h
; @expect_mem 3006h=0Fh
dec_iyd_test:
        dec     (iy+6)
        halt

; @test alu_a_ixd
; @init af=1000h ix=3000h
; @mem 3002h=05h
; @expect a=15h
alu_a_ixd_test:
        add     a,(ix+2)
        halt

; @test ld_ixd_r
; @init ix=3000h bc=C200h
; @expect_mem 3007h=C2h
ld_ixd_r_test:
        ld      (ix+7),b
        halt

; @test inc_rp
; @init bc=FFFFh
; @expect bc=0000h
inc_rp_test:
        inc     bc
        halt

; @test dec_r
; @init bc=0000h
; @expect bc=FF00h flag.z=0 flag.n=1
dec_r_test:
        dec     b
        halt

; @test rst_08
; @init sp=8000h
; @expect trap=1
rst_08_test:
        rst     08h
        halt

; @test cpl_scf
; @init af=AA00h
; @expect a=55h flag.c=1 flag.n=0
cpl_scf_test:
        cpl
        scf
        halt

; @test ed_ld_sp_inn
; @init sp=0000h
; @mem 4000h=00h 4001h=80h
; @expect sp=8000h
ed_ld_sp_inn_test:
        ld      sp,(4000h)
        halt

; @test ed_ld_inn_hl
; @init hl=BEEFh
; @expect_mem 5000h=EFh 5001h=BEh
ed_ld_inn_hl_test:
        ld      (5000h),hl
        halt

; @test sbc_a_ihl
; @init af=5001h hl=4000h
; @mem 4000h=10h
; @expect a=3Fh flag.n=1
sbc_a_ihl_test:
        sbc     a,(hl)
        halt

; @test and_a_n
; @init af=F500h
; @expect a=05h flag.z=0 flag.h=1
and_a_n_test:
        and     0Fh
        halt

; @test xor_a_n
; @init af=FF00h
; @expect a=00h flag.z=1
xor_a_n_test:
        xor     0FFh
        halt

; @test or_a_n
; @init af=0F00h
; @expect a=FFh
or_a_n_test:
        or      0F0h
        halt

; @test cp_a_r
; @init af=2000h bc=2000h
; @expect a=20h flag.z=1 flag.n=1
cp_a_r_test:
        cp      b
        halt

; @test dd_cb_res
; @init ix=3000h
; @mem 3001h=FFh
; @expect_mem 3001h=FEh
dd_cb_res_test:
        res     0,(ix+1)
        halt

; @test dd_cb_set
; @init ix=3000h
; @mem 3001h=00h
; @expect_mem 3001h=80h
dd_cb_set_test:
        set     7,(ix+1)
        halt

; @test fd_push_pop_iy
; @init sp=8000h iy=ABCDh
; @expect iy=ABCDh sp=8000h
fd_push_pop_iy_test:
        push    iy
        ld      iy,0000h
        pop     iy
        halt

; @test copy_block_like
; @init hl=2000h de=1000h bc=000Fh
; @mem 1000h=A0h 1001h=A1h 1002h=A2h 100Fh=AFh
; @expect_mem 2000h=A0h 2001h=A1h 2002h=A2h
; @expect c=00h
copy_block_like_test:
        inc     c
copy_block_like_loop:
        dec     c
        jr      z,copy_block_like_done
        ld      a,(de)
        ld      (hl),a
        inc     de
        inc     hl
        jr      copy_block_like_loop
copy_block_like_done:
        halt
