/**************************************************************************
 **                                                                       *
 **                           LSI CONFIDENTIAL      *
 **                           PROPRIETARY NOTE                            *
 **                                                                       *
 **    This software contains information confidential and proprietary    *
 **    to LSI Corporation Inc.  It shall not be reproduced in whole or in *
 **    part, or transferred to other documents, or disclosed to third     *
 **    parties, or used for any purpose other than that for which it was  *
 **    obtained, without the prior written consent of LSI Corporation Inc.*
 **    (c) 2008-2014, LSI Corporation Inc.  All rights reserved.          *
 **                                                                       *
 **
 **
 **************************************************************************/

/********************************************************************
 * ncp_sm_denali_2041_init.c
 *
 *   Initialization routine for the 
 *   Denali System Memory Controller version 2041. 
 *
 *
 *   With the appropriate wrapper macros this code code can 
 *   be included and compiled for both u-boot and RTE. If you
 *   are editing this code from within the RTE please do not 
 *   add anything that will break the u-boot build!!
 *******************************************************************/

#ifndef UBOOT
#include "ncp.h"
#include "ncp_pvt.h"
#include "ncp_sysmem_ext.h"
#include "ncp_sysmem_lsiphy.h"
#endif

extern ncp_sm_poll_controller_fn_t  pollControllerFn;


#define NCP_SM_ENCODE_RTT_NOM(val)  \
    ( ( (val & 0x1) << 2 ) |  /* a2 */ \
      ( (val & 0x2) << 5 ) |  /* a6 */ \
      ( (val & 0x4) << 7 ) )  /* a9 */
      

#define NCP_SM_ENCODE_DRV_IMP(val)  \
    ( ( (val & 0x1) << 1 ) |  /* a1 */ \
      ( (val & 0x2) << 4 ) )  /* a5 */ 





/*
 * DDR timing parameters 
 */
#ifndef UBOOT
    extern ncp_uint16_t tRFC_vals_ns[6] ;
#else
	ncp_uint16_t tRFC_vals_ns[6] = { 0, 90, 110, 160, 260, 350 };
#endif

typedef enum {
    NCP_DDR_CLOCK_400_MHZ = 0,
    NCP_DDR_CLOCK_533_MHZ,
    NCP_DDR_CLOCK_667_MHZ,
    NCP_DDR_CLOCK_800_MHZ,
    NCP_DDR_CLOCK_933_MHZ,
    NCP_DDR_CLOCK_1033_MHZ,
    NCP_DDR_CLOCK_MAX
} ncp_ddr_clock_speed_idx_t;

typedef struct {
    ncp_ddr_clock_speed_idx_t  idx;
    ncp_uint16_t               clk_mhz;
} ncp_ddr_clock_speeds_t;

ncp_uint16_t ncp_ddr_clock_speeds[7] = {
     400 , 533 , 667 , 800 , 933 , 1033, 0xffff 
} ;



typedef struct {
    ncp_uint8_t   base_CL;
    ncp_uint8_t   tRC[4];
    ncp_uint8_t   tRASmin;
    ncp_uint8_t   tFAW_1kb;
    ncp_uint8_t   tFAW_2kb;
} ncp_ddr_speedbin_vals_t;


/* speedbin values
 *   since we already have a separate entry for each 
 *   speed bin we pre-calculate the values in clocks. 
 *
 *   tRP is speedbin dependent, but can be calculated as 
 *
 *     tRP  = base_CL + (CL - base_CL);
 *     tRCD = tRP
 */
ncp_ddr_speedbin_vals_t speedbin_vals[6] = {
    {  5,   { 20, 21,  0,  0 },  16,  16,  20 },        /* 400 MHz */
    {  6,   { 26, 27, 28,  0 },  20,  20,  27 },        /* 533 MHz */
    {  7,   { 31, 32, 33, 34 },  24,  20,  30 },        /* 667 MHz */
    {  8,   { 36, 37, 38, 39 },  28,  24,  32 },        /* 800 MHz */
    { 10,   { 42, 43, 44, 45 },  32,  26,  33 },        /* 933 MHz */
    { 11,   { 46, 47, 48, 49 },  36,  38,  38 }         /* 1066 MHz */
};


#ifdef AXM_35xx
#include "ncp_utils.c"
#endif

ncp_st_t
ncp_sm_denali_2041_init(
    ncp_dev_hdl_t dev,
    ncp_uint32_t  smId,
    ncp_sm_parms_t *parms)
{
    ncp_st_t ncpStatus = NCP_ST_SUCCESS;
    ncp_uint32_t value;
    ncp_uint32_t value2;
    ncp_uint32_t max_row;
    ncp_uint32_t max_col;
    ncp_uint32_t col_diff;
    ncp_uint32_t row_diff;
    ncp_uint32_t ctl_32 = 0, ctl_33 = 0, ctl_34 = 0;
    ncp_region_id_t ctlReg = NCP_REGION_ID(sm_nodes[smId], NCP_SYSMEM_TGT_DENALI);
    ncp_uint32_t rttWr;
    ncp_uint32_t *p32;
    ncp_uint8_t clkIdx;
    ncp_uint16_t clkMhz;
    ncp_uint16_t tRFC;
    ncp_ddr_speedbin_vals_t *pVals;
    ncp_uint8_t tRC;
    ncp_uint8_t tRP;
    ncp_uint8_t tRASmin;
    ncp_uint16_t tREF;
    ncp_uint16_t tWR;


    NCP_COMMENT("Sysmem %d Denali Controller init", smId);

    /*printf(" parms->ddrClockSpeedMHz = %d\n", parms->ddrClockSpeedMHz );*/

    clkIdx = NCP_DDR_CLOCK_400_MHZ;
    while (parms->ddrClockSpeedMHz > ncp_ddr_clock_speeds[clkIdx] ) clkIdx++;
   
    if (clkIdx == NCP_DDR_CLOCK_MAX) 
    {
        NCP_CALL(NCP_ST_ERROR);
    }

    clkMhz = ncp_ddr_clock_speeds[clkIdx];

    /*printf(" parms->ddrClockSpeedMHz = %d, clkIdx=%d, using %d MHz\n", 
        parms->ddrClockSpeedMHz, clkIdx, clkMhz);*/


    tRFC = (ncp_uint16_t) ncp_ns_to_clk(clkMhz, 
                 tRFC_vals_ns [ parms->sdram_device_density ] );

    
    /*printf("dens=%d\n", parms->sdram_device_density);*/

    /*printf("tRFCns=%d\n", 
                 tRFC_vals_ns [ parms->sdram_device_density ] );*/


    pVals = &speedbin_vals[clkIdx];
    tRC = pVals->tRC[parms->CAS_latency - pVals->base_CL]; 
    tRP = pVals->base_CL + (parms->CAS_latency - pVals->base_CL);
    tRASmin = pVals->tRASmin;
    tREF =  (parms->high_temp_dram == TRUE) ?  3900 : 7800;
    tWR = ncp_ns_to_clk(clkMhz, 15); 

    /*printf("rdimm_ctl_0_0=0x%x rdim_ctl_0_1=0x%x rdimm_misc=0x%x\n",
		parms->rdimm_ctl_0_0, parms->rdimm_ctl_0_1, parms->rdimm_misc);*/

    /* DENALI_CTL_00 */
    /* TODO: at some point the AP field may be st */
    ncr_write32(ctlReg,  0x0000, 0x00010100);

    /* DENALI_CTL_01 */
    ncr_write32(ctlReg,  0x0004, 0x01010100);

    /* DENALI_CTL_02 */
    ncr_write32(ctlReg,  0x0008, 0x00000100);

    /* DENALI_CTL_04 */
    ncr_write32(ctlReg,  0x0010, 0x00000001);

    /* DENALI_CTL_06 */
    ncr_write32(ctlReg,  0x0018, 0x00010000);

    /* DENALI_CTL_07 */
    ncr_write32(ctlReg,  0x001c, 0x00000001);

    /* DENALI_CTL_11 */
    value = 0;
    if (parms->primary_bus_width == 2) {
        /* set the controller half-data path mode for 32-bit bus */
        SV( ncp_denali_DENALI_CTL_11_t, reduc, 1 );
    }
    /* registered DIMM support */
    SV( ncp_denali_DENALI_CTL_11_t, reg_dimm_enable, (parms->rdimm_misc & 0x1) );
    ncr_write32(ctlReg, 0x002c, value);


    /* DENALI_CTL_12 */
    ncr_write32(ctlReg,  0x0030, 0x01000001);

    /* DENALI_CTL_14 */

    if (parms->version == NCP_CHIP_ACP35xx)
    {
        /* bit24 set indicates free-running/limited WRR latency control
	 * setting to free-running as default
	 * turn off tref_enable- gets turned back on at runtime_adjust */
        ncr_write32(ctlReg,  0x0038, 0x01000100);
    }
    else
    {
        ncr_write32(ctlReg,  0x0038, 0x00010100);
    }

    /* DENALI_CTL_16 */
    if (parms->version == NCP_CHIP_ACP35xx)
    {
        /* bit[1:0] axi0_fifo_type_reg; clock_domain relativity between port-0 and core
	 * 0: async
	 * 1: 2:1 port:core pseudo-sync
	 * 2: 1:2 port:core pseudo-sync
	 * 3: sync
	 */
        ncr_write32(ctlReg,  0x0040, 0x00000000);
    }

    /* DENALI_CTL_17 */
    if (parms->version == NCP_CHIP_ACP35xx)
    {
        /* bit[1:0] axi1_fifo_type_reg; clock_domain relativity between port-1 and core
	 * 0: async
	 * 1: 2:1 port:core pseudo-sync
	 * 2: 1:2 port:core pseudo-sync
	 * 3: sync
   	 *
    	 * bit[9:8] axi1_port_ordering
	 */
        ncr_write32(ctlReg,  0x0044, 0x00000100);
    }
    else
    {
        ncr_write32(ctlReg,  0x0044, 0x00000100);
    }

    /* DENALI_CTL_18 */
    if (parms->version == NCP_CHIP_ACP35xx)
    {
        /* bit[1:0] axi2_fifo_type_reg; clock_domain relativity between port-2 and core
	 * 0: async
	 * 1: 2:1 port:core pseudo-sync
	 * 2: 1:2 port:core pseudo-sync
	 * 3: sync
   	 *
    	 * bit[9:8] axi2_port_ordering
	 */
    	ncr_write32(ctlReg,  0x0048, 0x00000200);
    }
    else
    {
    	ncr_write32(ctlReg,  0x0048, 0x00000200);
    }

    /* DENALI_CTL_19 */
    if (parms->version == NCP_CHIP_ACP35xx)
    {
        /* bit[1:0] axi3_fifo_type_reg; clock_domain relativity between port-2 and core
	 * 0: async
	 * 1: 2:1 port:core pseudo-sync
	 * 2: 1:2 port:core pseudo-sync
	 * 3: sync
   	 *
    	 * bit[9:8] axi3_port_ordering
	 */
    	ncr_write32(ctlReg,  0x004c, 0x00000300);
    }
    else
    {
    	ncr_write32(ctlReg,  0x004c, 0x00000300);
    }

    /* DENALI_CTL_20 */
    /* TODO!! */
    ncr_write32(ctlReg,  0x0050, 0x00000000);

    /* DENALI_CTL_21 */
    if (parms->version == NCP_CHIP_ACP35xx)
    {
	/* bit[9:8] weighted RR weight sharing
	 * 8: port 0 & 1
	 * 9: port 2 & 3
	 */
        ncr_write32(ctlReg,  0x0054, 0x02000300);
    }
    else
    {
        ncr_write32(ctlReg,  0x0054, 0x02000000);
    }

    /* DENALI_CTL_22 */
    ncr_write32(ctlReg,  0x0058, 0x03ff0000);

    /* DENALI_CTL_23 */
    ncr_write32(ctlReg,  0x005c, 0x00ff0000);

    /* DENALI_CTL_24 */
    /* q_fullness [19:16]
     * bits [31:24] tcke -
     * should be at least 6 to cover up to 2133 MHz 
     */
    ncr_write32(ctlReg,  0x0060, 0x06000000);

    /* DENALI_CTL_25 
     */
    value = 0;
    SV( ncp_denali_DENALI_CTL_25_t, w2r_diffcs_dly, parms->added_rank_switch_delay);
    SV( ncp_denali_DENALI_CTL_25_t, trtp, 6);
    SV( ncp_denali_DENALI_CTL_25_t, trrd, 5);
    ncr_write32(ctlReg,  0x0064, value);

    /* DENALI_CTL_26 */
    value = 0x04040a00;
    if (parms->address_mirroring)
    {
        /* set the controller half-data path mode for 32-bit bus */
        SV( ncp_denali_DENALI_CTL_26_t, address_mirroring, 0xa);
    }
    /* For 35xx the axi0_priority0/1_relative_priority set to 0x4 - Check */
    ncr_write32(ctlReg,  0x0068, value);

    /* DENALI_CTL_27 */
    /* For 35xx the axi0_priority2/3_relative_priority, axi1_priority0/1_relative_priority set to 0x4/0x3 */
    ncr_write32(ctlReg,  0x006c, 0x03030404);

    /* DENALI_CTL_28 */
    /* For 35xx the axi1_priority2/3_relative_priority, axi2_priority0/1_relative_priority set to 0x3/0x2 */
    ncr_write32(ctlReg,  0x0070, 0x02020303);

    /* DENALI_CTL_29 */
    /* For 35xx the axi2_priority2/3_relative_priority, axi3_priority0/1_relative_priority set to 0x2/0x1 */
    ncr_write32(ctlReg,  0x0074, 0x01010202);

    /* DENALI_CTL_30 */
    /* For 35xx the axi3_priority2/3_relative_priority set to 0x1/0x1 */
    ncr_write32(ctlReg,  0x0078, 0x0c000101);

    /* DENALI_CTL_31 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_31_t, dram_class, 6 );
    SV( ncp_denali_DENALI_CTL_31_t, cs_map, parms->topology );
    ncr_write32(ctlReg,  0x007c, value);

    /* DENALI_CTL_32 */
    /* DENALI_CTL_33 */
    /* DENALI_CTL_34 */
    /* These registers set up ODT based on topology. */
    /* Currently ACP2500 only supports two ranks */
    switch( parms->topology ) {
        case 0x1:
            ctl_32  = 0x00000000;
            ctl_33  = 0x00010000;
            ctl_34  = 0x00000201;
            break;
        case 0x3:
    	    if (parms->version == NCP_CHIP_ACP35xx)
	    {
		/* setting ODT_XX_MAP_CSX for dual-rank only support
		 * extract from parms when ase/params configurability
		 * is supported */
                ctl_32  = 0x01020000; /* ddr3 does not allow ODT during read */
                ctl_33  = 0x03030000; /* dual-rank support WR */
                ctl_34  = 0x00000000; /* cs2,3 invalid */
	    }
	    else
	    {
                ctl_32  = 0x00000000;
                ctl_33  = 0x02010000;
                ctl_34  = 0x00000000;
	    }
            break;
#if 0
        case 0x5:
            ctl_32  = 0x00040000;
            ctl_33  = 0x00040001;
            ctl_34  = 0x00000001;
            break;
        case 0xf:
            ctl_32  = 0x08040000;
            ctl_33  = 0x08040201;
            ctl_34  = 0x00000201;
            break;
#endif
        default:
            printf( "%d: Unsupported Topology.\n", parms->topology );
            acp_failure( __FILE__, __FUNCTION__, __LINE__ );
            break;
    }
    /* DENALI_CTL_32 */
    ncr_write32(ctlReg,  0x0080, ctl_32);
    /* DENALI_CTL_33 */
    ncr_write32(ctlReg,  0x0084, ctl_33);
    /* DENALI_CTL_34 */
    ncr_write32(ctlReg,  0x0088, ctl_34);

    /* DENALI_CTL_35 */
    ncr_write32(ctlReg,  0x008c, 0x00000002);

    /* DENALI_CTL_36 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_36_t, twtr, ncp_ns_to_clk(clkMhz, 8) );  /* : 7.5ns */
    SV( ncp_denali_DENALI_CTL_36_t, trp, parms->CAS_latency );
    ncr_write32(ctlReg,  0x0090, value );

    /* DENALI_CTL_37 */
    /* TODO : LARRY */
    ncr_write32(ctlReg,  0x0094, 0x00000808);

    /* DENALI_CTL_38 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_38_t, caslat_lin, ( parms->CAS_latency * 2 ));
    ncr_write32(ctlReg,  0x0098, value);

    /* DENALI_CTL_39 */
    value = 0;
    /* tDAL = CL + tWR  = CL + 15ns */
    SV( ncp_denali_DENALI_CTL_39_t, tdal, 
            ( parms->CAS_latency + ncp_ns_to_clk(clkMhz, 15) ) ); 

    /* 
     * due to controller bug, rdlat_adj must be 3 or greater.
     * we set it to 4
     */
    SV( ncp_denali_DENALI_CTL_39_t, rdlat_adj,  4);
    ncr_write32(ctlReg,  0x009c, value);

    /* DENALI_CTL_40 */
    /* TFAW depends on page size and speed bin  
     *
     * In general, x8 parts have a 1kB page size and x16
     * parts have a 2kB page size. The exception is 8Gbit
     * parts which have 2kB page size regardless of organization.
     */
    value = 0;
    if ( (NCP_SM_SDRAM_DENSITY_8GBIT == parms->sdram_device_density) || 
         (NCP_SM_SDRAM_WIDTH_16BITS  == parms->sdram_device_width)) 
    {
        SV(ncp_denali_DENALI_CTL_40_t, tfaw, pVals->tFAW_2kb);
    } else {
        SV(ncp_denali_DENALI_CTL_40_t, tfaw, pVals->tFAW_1kb);
    }
    SV(ncp_denali_DENALI_CTL_40_t, tdfi_phy_rdlat, parms->phy_rdlat);
    ncr_write32(ctlReg,  0x00a0, value);

    /* DENALI_CTL_41 */
     /*
     * twr_int has been moved to CTL_335 for 5500
     */
    value = 0;
    SV( ncp_denali_DENALI_CTL_41_t, tmrd, 4 );
    SV( ncp_denali_DENALI_CTL_41_t, trc, tRC);
    SV( ncp_denali_DENALI_CTL_41_t, twr_int, tWR);
    ncr_write32(ctlReg,  0x00a4, value);

    /* DENALI_CTL_42 */
    ncr_write32(ctlReg,  0x00a8, 0x00002819);

      
       

    /* DENALI_CTL_60 */
    /* ;ODO: tdfi_rdlvl_resplat = CL + AL + 15 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_60_t, tdfi_rdlvl_resplat, parms->CAS_latency + 15 );
    ncr_write32(ctlReg,  0x00f0, value);

    /* DENALI_CTL_61 */
    ncr_write32(ctlReg,  0x00f4, 0x04000300);

    /* DENALI_CTL_62 */
    /* trcd_int has been moved to CTL335 for 5500 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_62_t, trcd_int, parms->CAS_latency );
    SV( ncp_denali_DENALI_CTL_62_t, tras_min, pVals->tRASmin ); 
    SV( ncp_denali_DENALI_CTL_62_t, tmod, 12 );
    ncr_write32(ctlReg,  0x00f8, value);

    /* DENALI_CTL_63 */
    /* For 35xx the axi0_priority_relax set to 0x64 */
    ncr_write32(ctlReg,  0x00fc, 0x00006400);

    /* DENALI_CTL_64 */
    /* For 35xx the axi1/2_priority_relax set to 0x64 */
    ncr_write32(ctlReg,  0x0100, 0x00640064);

    /* DENALI_CTL_65 */
    /* For 35xx the axi3_priority_relax set to 0x64 */
    ncr_write32(ctlReg,  0x0104, 0x00400064);

    /* DENALI_CTL_69 */
    ncr_write32(ctlReg,  0x0114, 0x18621862);

    /* DENALI_CTL_70 */
    /*
     *   TREF[29:16] reg temp 7800ns hi temp 3900ns
     */
    value = 0;
    SV(ncp_denali_DENALI_CTL_70_t, tref, ncp_ns_to_clk(clkMhz, tREF));
    ncr_write32(ctlReg,  0x0118, value);

    /* DENALI_CTL_84 */
    /* upper half word is tpdex 
     * - should be at least 7 for 2133 MHz 
     */
    ncr_write32(ctlReg,  0x0150, 0x00070200);

    /* DENALI_CTL_85 */     
    /* upper half word is txpdll  
     * - should be at least 0x1a for 2133MHz
     */
    ncr_write32(ctlReg,  0x0154, 0x001a0000);

    /* DENALI_CTL_86 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_86_t, txsr, 0x0200 );
    SV( ncp_denali_DENALI_CTL_86_t,
        txsnr, ( tRFC + (ncp_ns_to_clk(clkMhz, 10))));
    ncr_write32(ctlReg,  0x0158, value);

    /* DENALI_CTL_89 */
    ncr_write32(ctlReg,  0x0164, 0x00000000);

    /* DENALI_CTL_93 */
    ncr_write32(ctlReg,  0x0174, 0x00000008);

    /* DENALI_CTL_94 */
    ncr_write32(ctlReg,  0x0178, 0x00000190);

    /* DENALI_CTL_95 */
    ncr_write32(ctlReg,  0x017c, 0x00008000);

    /* DENALI_CTL_126 */
    ncr_write32(ctlReg,  0x01f8, 0x000000a0);

    /* DENALI_CTL_181 */
    ncr_write32(ctlReg,  0x02d4, 0x00010000);

    /* DENALI_CTL_182 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_182_t, r2w_samecs_dly, 2);
    SV( ncp_denali_DENALI_CTL_182_t, r2w_diffcs_dly, parms->added_rank_switch_delay + 1);
    SV( ncp_denali_DENALI_CTL_182_t, r2r_diffcs_dly, parms->added_rank_switch_delay);
    ncr_write32( ctlReg, 0x2d8, value);

    /* DENALI_CTL_183 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_183_t, w2w_diffcs_dly, parms->added_rank_switch_delay);
    SV( ncp_denali_DENALI_CTL_183_t, tccd, 4);
    ncr_write32( ctlReg, 0x2dc, value);

    /* DENALI_CTL_184 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_184_t, trp_ab, parms->CAS_latency );
    SV( ncp_denali_DENALI_CTL_184_t, add_odt_clk_difftype_diffcs, 2);
    SV( ncp_denali_DENALI_CTL_184_t, add_odt_clk_sametype_diffcs, 2);
    ncr_write32(ctlReg,  0x02e0, value);

    /* DENALI_CTL_185 */
    /* 
     * tdfi_rdlvl_rr = 0x10c
     * tdfi_wrlvl_ww = 0xc8
     */ 
    ncr_write32(ctlReg,  0x02e4, 0x00c8010c);

    /* DENALI_CTL_186 */
    ncr_write32(ctlReg,  0x02e8, 0x02000100);

    /* DENALI_CTL_187 */
    /* DENALI_CTL_188 */
    /* MR0 */
    /*
     * The MR0 3-bit 'WR' field [11:9] specifies the value of tWR in 
     * clocks thusly:
     *
     *      000 : 16 
     *      001 :  5
     *      010 :  6
     *      011 :  7
     *      100 :  8
     *      101 : 10
     *      110 : 12
     *      111 : 14
     *
     *  We calculate the base value from the tWR setting accordingly.
     */

    if (tWR >= 7) 
    {
        value = ((tWR >> 1) & 0x7) << 9;
    } 
    else 
    {
        value = (tWR - 4) << 9;
    }

    if( parms->CAS_latency < 12 ) {
        value |= ( ( parms->CAS_latency - 4 ) << 4 );
    } else {
      value |= ( 0x0004 | ( ( parms->CAS_latency - 12 ) << 4 ) );
    }
    value |= value << 16 ;
    ncr_write32(ctlReg,  0x02ec, value);
    ncr_write32(ctlReg,  0x02f0, value);

    /* DENALI_CTL_189 */
    /* DENALI_CTL_190 */
    /* MR1 rank 0 - 1 */
    value   = NCP_SM_ENCODE_RTT_NOM(parms->per_sysmem[smId].sdram_rtt_nom[0]);
    value  |= NCP_SM_ENCODE_DRV_IMP(parms->per_sysmem[smId].sdram_data_drv_imp[0]);
    value2  = NCP_SM_ENCODE_RTT_NOM(parms->per_sysmem[smId].sdram_rtt_nom[1]);
    value2 |= NCP_SM_ENCODE_DRV_IMP(parms->per_sysmem[smId].sdram_data_drv_imp[1]);

    value |= value2 << 16; 
    ncr_write32(ctlReg,  0x02f4, value);

    /* MR1 rank 2 - 3  */
    value   = NCP_SM_ENCODE_RTT_NOM(parms->per_sysmem[smId].sdram_rtt_nom[2]);
    value  |= NCP_SM_ENCODE_DRV_IMP(parms->per_sysmem[smId].sdram_data_drv_imp[2]);
    value2  = NCP_SM_ENCODE_RTT_NOM(parms->per_sysmem[smId].sdram_rtt_nom[3]);
    value2 |= NCP_SM_ENCODE_DRV_IMP(parms->per_sysmem[smId].sdram_data_drv_imp[3]);

    value |= value2 << 16; 
    ncr_write32(ctlReg,  0x02f8, value);

    /* DENALI_CTL_191 */
    /* DENALI_CTL_192 */
    /* MR2 : FREQDEP */
    /* rank 0 - 1 */
    rttWr  = parms->per_sysmem[smId].sdram_rtt_wr[0] ;
    value  = ( rttWr << 9) ; 
    value |= ( parms->CAS_write_latency - 5 ) << 3;

    rttWr  = parms->per_sysmem[smId].sdram_rtt_wr[1];
    value2  = ( rttWr << 9) ; 
    value2 |= ( parms->CAS_write_latency - 5 ) << 3;

    value |= ( value2 << 16 );
    ncr_write32(ctlReg,  0x02fc, value);

    /* rank 2 - 3 */
    rttWr  = parms->per_sysmem[smId].sdram_rtt_wr[2];
    value  = ( rttWr << 9) ; 
    value |= ( parms->CAS_write_latency - 5 ) << 3;

    rttWr  = parms->per_sysmem[smId].sdram_rtt_wr[3];
    value2  = ( rttWr << 9) ; 
    value2 |= ( parms->CAS_write_latency - 5 ) << 3;

    value |= ( value2 << 16 );
    ncr_write32(ctlReg,  0x0300, value);
       
    /* DENALI_CTL_223 - tdfi_rdlvl_max */
    ncr_write32(ctlReg,  0x037c, 0x00100000);

    /* DENALI_CTL_225 */
    ncr_write32(ctlReg,  0x0384, 0x00100000);


    /* DENALI_CTL_227 */
    /* bits [15:8] tckesr 
     * should be at least 7 for 2133 MHz (one more than tcke)
     */
    ncr_write32(ctlReg,  0x038c, 0x02000704);

    /* DENALI_CTL_228 */
    ncr_write32(ctlReg,  0x0390, 0x02000200);

    /* DENALI_CTL_229 */
    value = 0;
    SV(ncp_denali_DENALI_CTL_229_t, tras_max, ncp_ns_to_clk(clkMhz, (tREF * 9)));
    ncr_write32(ctlReg,  0x0394, value);

    /* DENALI_CTL_230 */
    value = 0x02000000;
    SV( ncp_denali_DENALI_CTL_230_t, trfc, tRFC);
    ncr_write32(ctlReg,  0x0398, value);

    /* DENALI_CTL_232 */
    ncr_write32(ctlReg,  0x03a0, 0x07070000);

    if (parms->rdimm_misc & 0x1)
    {
        /* DENALI_CTL_234 */
        ncr_write32(ctlReg,  0x03a8, parms->rdimm_ctl_0_0);

        /* DENALI_CTL_235 */
        ncr_write32(ctlReg,  0x03ac, parms->rdimm_ctl_0_1);
    }
    else
    {
	/* is this really needed ? */
        /* DENALI_CTL_234 */
        ncr_write32(ctlReg,  0x03a8, 0x24101065);

        /* DENALI_CTL_235 */
        ncr_write32(ctlReg,  0x03ac, 0x00001370);
    }

    /* DENALI_CTL_236 */
    if (parms->rdimm_misc & 0x1)
    {
 	/* specify which cs to enable for DIMM0 */
        ncr_write32(ctlReg,  0x03b0, 0x00000003);
    }

#if 0
    /* not needed, as currently supporting only 1 rdimm per sysmem intf */
    /* DENALI_CTL_237 */
    ncr_write32(ctlReg,  0x03b4, 0x13030154);

    /* DENALI_CTL_238 */
    ncr_write32(ctlReg,  0x03b8, 0x00001367);
#endif

    /* DENALI_CTL_239 */
    value = 0x000fff0c; /* legacy value */
    /* [3:0] cs_map_dimm_1: currently only supporting 1 rdimm so this can be zero */
    SV( ncp_denali_DENALI_CTL_239_t, cs_map_dimm_1, 0);
    /* [23:8]: 1 bit for each ctl-word */
    SV( ncp_denali_DENALI_CTL_239_t, rdimm_cw_map, ((parms->rdimm_misc & 0xffff0000)>>16));
    ncr_write32(ctlReg,  0x03bc, value);

    /* DENALI_CTL_240 */
    value = 0x00000800;
    SV( ncp_denali_DENALI_CTL_240_t, rdimm_cw_hold_cke_en, (parms->rdimm_misc & 0x1));
    SV( ncp_denali_DENALI_CTL_240_t, rdimm_tmrd, 8);
    ncr_write32(ctlReg,  0x03c0, value);

    /* DENALI_CTL_241 */
    if (parms->rdimm_misc & 0x1)
    {
	/* rdimm_tstab is freq based, unit is cycles: for 933 it is 5usec, for other freq's it is 6 usec */
	value = ((clkMhz == 933) ? 5 : 6);
	value *= clkMhz;
    	SV( ncp_denali_DENALI_CTL_241_t, rdimm_tstab, value);
        ncr_write32(ctlReg,  0x03c4, value);
    }
    else
    {
     	/* is this really needed ?. */
        ncr_write32(ctlReg,  0x03c4, 0x000012c0);
    }

    /* DENALI_CTL_242 */
    value = 0x0;
    SV( ncp_denali_DENALI_CTL_242_t, tref_interval, ((parms->rdimm_misc & 0x1) ? 0x8 : 0x5));
    ncr_write32(ctlReg,  0x03c8, value);

    /* DENALI_CTL_244 */
    /* bits [7:0] cksre 
     * bits [15:8] cksrx 
     * both should be 0xb for 2133 MHz
     */
  	ncr_write32(ctlReg,  0x03d0, 0x00000b0b);
       

    /* DENALI_CTL_279 */
    if (parms->version == NCP_CHIP_ACP35xx)
    {
    	/* For 35xx the axi0_start_addr0 is 28:8 instead of 29:8
	 * port_addr_protection_en set to 1 */
    	ncr_write32(ctlReg,  0x045c, 0x00000001);
    }
    else
    {
    	ncr_write32(ctlReg,  0x045c, 0x00000000);
    }
       
    if (parms->version == NCP_CHIP_ACP35xx)
    {
    	/* DENALI_CTL_280 */
	/* axi0_end_addr_0 max'd */
    	ncr_write32(ctlReg,  0x0460, 0x001fffff);

    	/* DENALI_CTL_281 */
	/* axi0_range_prot_bits_0
	 * 0: priviledged & secureonly
	 * 1: secure (with or without privilege)
	 * 2: priviledged (with or without secure)
	 * 3: full access */
    	ncr_write32(ctlReg,  0x0464, 0x00000003);

    	/* DENALI_CTL_282 */
	/* axi0_start_addr_1 */
    	ncr_write32(ctlReg,  0x0468, 0x00000000);

    	/* DENALI_CTL_283 */
	/* axi0_end_addr_1 */
    	ncr_write32(ctlReg,  0x046c, 0x001fffff);

    	/* DENALI_CTL_284 */
	/* axi0_range_prot_bits_1
	 * 0: priviledged & secureonly
	 * 1: secure (with or without privilege)
	 * 2: priviledged (with or without secure)
	 * 3: full access */
    	ncr_write32(ctlReg,  0x0470, 0x00000003);

    	/* DENALI_CTL_292 */
	/* axi1_end_addr_0 */
    	ncr_write32(ctlReg,  0x0490, 0x001fffff);

    	/* DENALI_CTL_293 */
	/* axi1_range_prot_bits_0
	 * 0: priviledged & secureonly
	 * 1: secure (with or without privilege)
	 * 2: priviledged (with or without secure)
	 * 3: full access */
    	ncr_write32(ctlReg,  0x0494, 0x00000003);

    	/* DENALI_CTL_295 */
	/* axi1_end_addr_1 */
    	ncr_write32(ctlReg,  0x049c, 0x001fffff);

    	/* DENALI_CTL_296 */
	/* axi1_range_prot_bits_1
	 * 0: priviledged & secureonly
	 * 1: secure (with or without privilege)
	 * 2: priviledged (with or without secure)
	 * 3: full access */
    	ncr_write32(ctlReg,  0x04a0, 0x00000003);

    	/* DENALI_CTL_304 */
	/* axi2_end_addr_0 */
    	ncr_write32(ctlReg,  0x04c0, 0x001fffff);

    	/* DENALI_CTL_305 */
	/* axi2_range_prot_bits_0
	 * 0: priviledged & secureonly
	 * 1: secure (with or without privilege)
	 * 2: priviledged (with or without secure)
	 * 3: full access */
    	ncr_write32(ctlReg,  0x04c4, 0x00000003);

    	/* DENALI_CTL_307 */
	/* axi2_end_addr_1 */
    	ncr_write32(ctlReg,  0x04cc, 0x001fffff);

    	/* DENALI_CTL_308 */
	/* axi2_range_prot_bits_1
	 * 0: priviledged & secureonly
	 * 1: secure (with or without privilege)
	 * 2: priviledged (with or without secure)
	 * 3: full access */
    	ncr_write32(ctlReg,  0x04d0, 0x00000003);

    	/* DENALI_CTL_316 */
	/* axi3_end_addr_0 */
    	ncr_write32(ctlReg,  0x04f0, 0x001fffff);

    	/* DENALI_CTL_317 */
	/* axi3_range_prot_bits_0
	 * 0: priviledged & secureonly
	 * 1: secure (with or without privilege)
	 * 2: priviledged (with or without secure)
	 * 3: full access */
    	ncr_write32(ctlReg,  0x04f4, 0x00000003);

    	/* DENALI_CTL_319 */
	/* axi3_end_addr_1 */
    	ncr_write32(ctlReg,  0x04fc, 0x001fffff);

    	/* DENALI_CTL_320 */
	/* axi3_range_prot_bits_1
	 * 0: priviledged & secureonly
	 * 1: secure (with or without privilege)
	 * 2: priviledged (with or without secure)
	 * 3: full access */
    	ncr_write32(ctlReg,  0x0500, 0x00000003);
    }

#if 0
    /* DENALI_CTL_327 */
    ncr_write32(ctlReg,  0x051c, 0x000079ea);
#endif
    if (parms->version == NCP_CHIP_ACP35xx)
    {
    	ncr_write32(ctlReg,  0x051c, 0x0000f370);
    }

    /* DENALI_CTL_328 */
    ncr_write32(ctlReg,  0x0520, 0x01010002);

    /* DENALI_CTL_329 */
    if (parms->version == NCP_CHIP_ACP35xx)
    {
    	ncr_write32(ctlReg,  0x0524, 0x01010100);
    }
    else
    {
    	ncr_write32(ctlReg,  0x0524, 0x03010100);
    }

    /* DENALI_CTL_330 */
    ncr_write32(ctlReg,  0x0528, 0x00000003);

    /* DENALI_CTL_332 */
    ncr_write32(ctlReg,  0x0530, 0x00000040);

    /* DENALI_CTL_333 */
    /* 
     * The row/column/bank addressing has changed in the new 
     * version of the denali controller. It is specified as 
     * the difference in number of row/column bits between
     * the SDRAM device and the max row/column bits in this
     * instantiation of the controller. Not all Axxia devices
     * have the same number of row/col bits, so we read the 
     * controller to determine the max row/column bits and 
     * calculate the diff for the given SDRAM. 
     *
     *     
     *   Density         Width        row bits       col bits
     *   512Mb           x8             13               10
     *   512Mb           x16            12               10
     *   1Gb             x8             14               10
     *   1Gb             x16            13               10
     *   2Gb             x8             15               10
     *   2Gb             x16            14               10
     *   4Gb             x8             16               10
     *   4Gb             x16            15               10
     *   8Gb             x8             16               11
     *   8Gb             x16            16               10
     *
     *
     *  sdram_device_density is defined as follows:
     *   0=256M, 1=512Mb, 2=1Gb, 3=2Gb, 4=4Gb, 5=8Gb, and 6=16Gb
     *
     *  sdram_device_width is defined as follows:
     *   0=x4, 1=x8, 2=x16, and 3=x32 (note that x4 and x32 aren't supported)
     *
	 *  So, for all densities except 8Gb:
     *      row_bits = 11 + dens + (2-width)
     *      row_diff = max_row - row_bits
     *               = max_row - (11 + dens + 2 - width) 
     *               = max_row + width - dens - 13
     *      col_bits = 10
     *
     * For 8Gb parts : 
     *      row_bits = 16
     *      col_bits = 12 - width
     *      col_diff = max_col - (col_bits) = max_col - (12 - width)
     *               = max_col + width - 12
     */

    ncr_read32(ctlReg, 0x80, &value);
    max_col = (value & 0x0000ff00) >> 8;
    
    ncr_read32(ctlReg, 0x98, &value);
    max_row = (value & 0x00ff0000) >> 16;

    value = 0;
    if (parms->sdram_device_density == NCP_SM_SDRAM_DENSITY_8GBIT) {
        row_diff = max_row - 16;
        col_diff = max_col + parms->sdram_device_width - 12;
    }
    else 
    {
        row_diff = max_row + parms->sdram_device_width - parms->sdram_device_density - 13;
        col_diff = max_col - 10;
    }

    SV(ncp_denali_DENALI_CTL_333_t, row_diff, row_diff);
    SV(ncp_denali_DENALI_CTL_333_t, col_diff,  col_diff);
    if (parms->version == NCP_CHIP_ACP35xx)
    {
        /* Enable rw_same_page_en page grouping */
    	SV( ncp_denali_DENALI_CTL_333_t, rw_same_page_en, 1);
    }
    ncr_write32(ctlReg,  0x0534, value);

    /* DENALI_CTL_334 */
    value = 0;
    SV( ncp_denali_DENALI_CTL_334_t, todtl_2cmd, 8);
    ncr_write32(ctlReg,  NCP_DENALI_CTL_334, value);

    if (parms->version == NCP_CHIP_ACP55xx) 
    {
        /* DENALI_CTL_335 */
        value = 0;
        SV( ncp_denali_DENALI_CTL_335_t, twr, tWR);
        SV( ncp_denali_DENALI_CTL_335_t, trcd, tRP); /* tRCD = tRP */
        ncr_write32(ctlReg,  NCP_DENALI_CTL_335, value);

        /* DENALI_CTL_337 */
        /*
         * zq_interval specifies the auto-zq calibration interval
         * in 'long count sequences'. By default each long count 
         * sequence is 1024 clocks. 
         *
         * an interval of zero indicates that auto-zq calibration 
         * is disabled so we don't touch the register. 
         */
        if (parms->zqcs_interval) {
            value = parms->zqcs_interval * 800 / 1024;
            if (value == 0) 
            {
                value = 1;
            }

            ncr_write32(ctlReg,  NCP_DENALI_CTL_337, value);
        }

        /* DENALI_CTL_339 */
        value = 0;
        SV( ncp_denali_DENALI_CTL_339_t, todth_wr, 6);
        SV( ncp_denali_DENALI_CTL_339_t, todth_rd, 6);
        ncr_write32(ctlReg,  NCP_DENALI_CTL_339, value);


        /*
         * if any RTTnom value is non-zero then enable ODT 
         */
        p32 = (ncp_uint32_t *) parms->per_sysmem[smId].sdram_rtt_nom;
        if (p32 != 0) 
        {
            value = 0;
            SV(ncp_denali_DENALI_CTL_340_t, odt_en, 1);
            ncr_write32(ctlReg,  NCP_DENALI_CTL_340, value);
        }

        /* DENALI_CTL_420 */
        value = 0;
/*         SV( ncp_denali_DENALI_CTL_420_t, wr_order_req, 1); */    /* NOT YET */
        SV( ncp_denali_DENALI_CTL_420_t, rd_to_odth, 4);
        ncr_write32(ctlReg,  NCP_DENALI_CTL_420, value);
    }
    else if (parms->version == NCP_CHIP_ACP35xx)
    {
        /* DENALI_CTL_335 */
        /* on 3500 twr is CTL_335 bits[6:0] */
        /* this differs from the CTL_335 register definition for 5500 */
        ncr_write32(ctlReg,  NCP_DENALI_CTL_335, tWR);

        ncr_write32(ctlReg,  NCP_DENALI_CTL_344, tRP);  /* tRCD = tRP */

        /* DENALI_CTL_345 */
        /*
         * zq_interval specifies the auto-zq calibration interval
         * in 'long count sequences'. By default each long count 
         * sequence is 1024 clocks. 
         *
         * an interval of zero indicates that auto-zq calibration 
         * is disabled so we don't touch the register. 
         */
        if (parms->zqcs_interval) {
            value = parms->zqcs_interval * 800 / 1024;
            if (value == 0) 
            {
                value = 1;
            }

            ncr_write32(ctlReg,  NCP_DENALI_CTL_345, value);
        }

        /* DENALI_CTL_349 */
        value = 0x606;
        /* todth_wr = 6 */
        /* todth_rd = 6 */
        /*
         * if any RTTnom value is non-zero then enable ODT 
         */
        p32 = (ncp_uint32_t *) parms->per_sysmem[smId].sdram_rtt_nom;
        if (p32 != 0) 
        {
            /* odt_en = 1 */
	    value |= 0x10000;
        }
        ncr_write32(ctlReg,  NCP_DENALI_CTL_349, value);

        /* DENALI_CTL_351 */
	/* axi0_r/w_priority */
        ncr_write32(ctlReg,  0x057c, 0x02020000);

        /* DENALI_CTL_352 */
	/* axi1_r/w_priority */
        ncr_write32(ctlReg,  0x0580, 0x02020000);

        /* DENALI_CTL_353 */
	/* axi2_r/w_priority */
        ncr_write32(ctlReg,  0x0584, 0x02020000);

        /* DENALI_CTL_354 */
	/* axi3_r/w_priority */
        ncr_write32(ctlReg,  0x0588, 0x02020000);

        /* DENALI_CTL_355 */
	/* allowed read/write ID's for port 0 addr range 0 */
        ncr_write32(ctlReg,  0x058c, 0xffffffff);
        /* DENALI_CTL_356 */
	/* identifies which 4 id's to be used to index into range above */
        ncr_write32(ctlReg,  0x0590, 0x000f000f);

        /* DENALI_CTL_357 */
	/* allowed read/write ID's for port 0 addr range 1 */
        ncr_write32(ctlReg,  0x0594, 0xffffffff);
        /* DENALI_CTL_358 */
	/* identifies which 4 id's to be used to index into range above */
        ncr_write32(ctlReg,  0x0598, 0x000f000f);

        /* DENALI_CTL_359 */
	/* allowed read/write ID's for port 1 addr range 0 */
        ncr_write32(ctlReg,  0x059c, 0xffffffff);
        /* DENALI_CTL_360 */
	/* identifies which 4 id's to be used to index into range above */
        ncr_write32(ctlReg,  0x05a0, 0x000f000f);

        /* DENALI_CTL_361 */
	/* allowed read/write ID's for port 1 addr range 1 */
        ncr_write32(ctlReg,  0x05a4, 0xffffffff);
        /* DENALI_CTL_362 */
	/* identifies which 4 id's to be used to index into range above */
        ncr_write32(ctlReg,  0x05a8, 0x000f000f);

        /* DENALI_CTL_363 */
	/* allowed read/write ID's for port 2 addr range 0 */
        ncr_write32(ctlReg,  0x05ac, 0xffffffff);
        /* DENALI_CTL_364 */
	/* identifies which 4 id's to be used to index into range above */
        ncr_write32(ctlReg,  0x05b0, 0x000f000f);

        /* DENALI_CTL_365 */
	/* allowed read/write ID's for port 2 addr range 1 */
        ncr_write32(ctlReg,  0x05b4, 0xffffffff);
        /* DENALI_CTL_366 */
	/* identifies which 4 id's to be used to index into range above */
        ncr_write32(ctlReg,  0x05b8, 0x000f000f);

        /* DENALI_CTL_367 */
	/* allowed read/write ID's for port 3 addr range 0 */
        ncr_write32(ctlReg,  0x05bc, 0xffffffff);
        /* DENALI_CTL_368 */
	/* identifies which 4 id's to be used to index into range above */
        ncr_write32(ctlReg,  0x05c0, 0x000f000f);

        /* DENALI_CTL_369 */
	/* allowed read/write ID's for port 3 addr range 1 */
        ncr_write32(ctlReg,  0x05c4, 0xffffffff);
        /* DENALI_CTL_370 */
	/* identifies which 4 id's to be used to index into range above */
        ncr_write32(ctlReg,  0x05c8, 0x000f000f);

        /* DENALI_CTL_372 */
 	/* chk bit 0 is reserved ? */
        ncr_write32(ctlReg,  0x05d0, 0x00000001);

        /* DENALI_CTL_373 */
 	/* bit-24 disable addr collision detection extension
	 * using micro page mask for cmd q placement and selection.
	 *
 	 * bit-8 disable ecc reporting and correcting, there is enable func separately */
        ncr_write32(ctlReg,  0x05d4, 0x01000000);

        /* DENALI_CTL_374 */
        value = 0;
        /* rd_to_odth = 3 */
	value |= ((3 & 0x3f) << 8);
	/* what about delay from wr to odt assertion ?. */
        ncr_write32(ctlReg,  NCP_DENALI_CTL_374, value);
    }

NCP_RETURN_LABEL
    return ncpStatus;
}

ncp_st_t
ncp_sm_denali_enable(
    ncp_dev_hdl_t dev,
    ncp_uint32_t  smId,
    ncp_sm_parms_t *parms)
{
    ncp_st_t ncpStatus = NCP_ST_SUCCESS;
    ncp_uint32_t mask, value;
    ncp_region_id_t ctlReg = NCP_REGION_ID(sm_nodes[smId], NCP_SYSMEM_TGT_DENALI);

#ifndef AXM_35xx
    if (parms->ddrRecovery == TRUE)
    {
        /* exit self-refresh */
        mask = value = 0;
        SMAV(ncp_denali_DENALI_CTL_07_t, pwrup_srefresh_exit, 1);
        ncr_modify32(ctlReg,  NCP_DENALI_CTL_07, mask, value);
    }
#endif

    /* enable the controller ! */
    mask = value = 0;
    SMAV(ncp_denali_DENALI_CTL_12_t, start, 1);
    ncr_modify32(ctlReg,  NCP_DENALI_CTL_12, mask, value);

    /* poll for MC init done */
    NCP_CALL(pollControllerFn(dev, ctlReg, NCP_SM_MC_INIT_DONE));
                

NCP_RETURN_LABEL
    return ncpStatus;
}
