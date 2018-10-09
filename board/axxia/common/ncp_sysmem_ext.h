/*
 *  Copyright (C) 2016 Intel (john.jacques@intel.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __NCP_SYSMEM_EXT_H__
#define __NCP_SYSMEM_EXT_H__

#ifndef UBOOT
#include "ncp_pvt.h"
/* #define SM_PLL_533_MHZ */


/*
 * the number syscache clients on the ASIC.
 * There are 8 syscaches for v1 and v2.
 * There will only be 4 for v3. We'll worry about v3 later. 
 */
NCP_API ncp_uint32_t sc_nodes[NCP_SYSCACHE_NUM_NODES];

/*
 * Includes both sysmem and treemem nodes
 */
NCP_API ncp_uint32_t sm_nodes[NCP_EXTMEM_NUM_NODES];

#endif

#ifndef NCP_CHECK_DISABLED

#define NCP_SYSMEM_VALIDATE_HANDLE(_myHdl) ({ \
    ncp_st_t _st = NCP_ST_SUCCESS; \
    if ( _myHdl == NULL ) {\
           _st = NCP_ST_SYSMEM_INVALID_HANDLE; \
    } \
    _st; \
})
#else /* #ifndef NCP_CHECK_DISABLED */
#define NCP_SYSMEM_VALIDATE_HANDLE(_myHdl) NCP_ST_SUCCESS
#endif

#define NCP_SYSCACHE_ENABLE_INIT_VAL    0xed

/* Targets for sysmem nodes */
#define NCP_SYSMEM_TGT_DENALI     0
#define NCP_SYSMEM_TGT_PHY        1
#define NCP_SYSMEM_TGT_PRF        2

/* targets for classifier tree memory nodes */
#define NCP_TREEMEM_TGT_AXI       8
#define NCP_TREEMEM_TGT_DDR       9
#define NCP_TREEMEM_TGT_PHY      10 



/*
 * the syscache 'mode' can be set to either 'performance' or 'debug'.
 * The difference is how the 'field_order' field is set in the 
 * mem_addr_munge register. 
 * For 'performance' mode we set this to 0 (id_hash_tag_index), and 
 * for 'debug' mode we set this to 2 (id_tag_index). 
 */
#define NCP_SYSCACHE_MODE_PERFORMANCE   0x0
#define NCP_SYSCACHE_MODE_DEBUG         0x2


#define NCP_SM_MAX_RANKS    4

#ifndef __UBOOT__

/*
 * enumerated type for the four 'standard' topologies
 * the values are assigned as a bit mask of which ranks
 * (chip selects) exist.
 */
typedef enum {
    NCP_SM_TOPO_ONE_SINGLE_RANK = 0x1,
    NCP_SM_TOPO_ONE_DUAL_RANK   = 0x3,
    NCP_SM_TOPO_TWO_SINGLE_RANK = 0x5,
    NCP_SM_TOPO_TWO_DUAL_RANK   = 0xf
} ncp_sm_topology_t;

/*
 * enumerated type for the SDRAM device density
 * these are defined to match the values used by 
 * DDR3 SDRAM DIMM module SPD. The actual value
 * can be calculated using the formula 
 *
 *   density_in_mbit = 256Mbit * (2 ^ density)
 *
 * For now we don't define the uncommon values,
 * it's not clear that the controller/phy can 
 * even support them. 
 */

#define NCP_SM_256_MBIT  ( 256 * 1024 * 1024 )

typedef enum {
    /*    
    NCP_SM_SDRAM_DENSITY_256MBIT  = 0,
    */
    NCP_SM_SDRAM_DENSITY_512MBIT  = 1,
    NCP_SM_SDRAM_DENSITY_1GBIT    = 2,
    NCP_SM_SDRAM_DENSITY_2GBIT    = 3,
    NCP_SM_SDRAM_DENSITY_4GBIT    = 4,
    NCP_SM_SDRAM_DENSITY_8GBIT    = 5,
    NCP_SM_SDRAM_DENSITY_16GBIT   = 6
} ncp_sm_sdram_density_t;

typedef enum {
    NCP_SM_SDRAM_WIDTH_4BITS  = 0,
    NCP_SM_SDRAM_WIDTH_8BITS  = 1,
    NCP_SM_SDRAM_WIDTH_16BITS = 2,
    NCP_SM_SDRAM_WIDTH_32BITS = 3
} ncp_sm_sdram_width_t;

/*
 * these are the values for RTT_nom as defined in the JEDEC spec
 */
typedef enum {
    NCP_SM_SDRAM_RTT_IMP_DISABLED = 0,
    NCP_SM_SDRAM_RTT_IMP_60_OHM   = 1,
    NCP_SM_SDRAM_RTT_IMP_120_OHM  = 2,
    NCP_SM_SDRAM_RTT_IMP_40_OHM   = 3,
    NCP_SM_SDRAM_RTT_IMP_20_OHM   = 4,
    NCP_SM_SDRAM_RTT_IMP_30_OHM   = 5
} ncp_sm_sdram_rtt_imp_t;

typedef enum {
    NCP_SM_SDRAM_DRV_IMP_40_OHM = 0,
    NCP_SM_SDRAM_DRV_IMP_34_OHM = 1
} ncp_sm_sdram_drv_imp_t;

typedef enum {
    NCP_SM_PRIMARY_BUS_WIDTH_8BITS  = 0,
    NCP_SM_PRIMARY_BUS_WIDTH_16BITS = 1,
    NCP_SM_PRIMARY_BUS_WIDTH_32BITS = 2,
    NCP_SM_PRIMARY_BUS_WIDTH_64BITS = 3
} ncp_sm_primary_bus_width_t;

typedef enum {
    NCP_SM_DDR3_MODE = 6,
    NCP_SM_DDR4_MODE = 10
} ncp_dram_class_t;

typedef enum {
    NCP_SM_REFRESH_MODE_1X = 0,
    NCP_SM_REFRESH_MODE_2X = 1,
    NCP_SM_REFRESH_MODE_4X = 2
} ncp_refresh_mode_t;

#ifdef _MSC_VER
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((packed)) {
#endif
    unsigned char sdram_rtt_nom[4];
    unsigned char sdram_rtt_wr[4];
    unsigned char sdram_data_drv_imp[4];
    unsigned long phy_min_cal_delay;
    unsigned long phy_adr_phase_select;
    unsigned long phy_dp_io_vref_set;
    unsigned long phy_adr_io_vref_set;
    unsigned long phy_rdlvl_cmp_even;
    unsigned long phy_rdlvl_cmp_odd;
    unsigned long phy_write_align_finetune;
} ncp_per_sysmem_parms_t;

#ifdef _MSC_VER
#pragma pack(1)
typedef struct {
#else
typedef struct __attribute__((packed, aligned(8))) {
#endif
    unsigned char sdram_rtt_nom[4];		/* packed as intf-0 rank-0, intf-0 rank-1, intf-0 rank-2, intf-0 rank-3 */
    unsigned char sdram_rtt_wr[4];		/* packed as intf-0 rank-0, intf-0 rank-1, intf-0 rank-2, intf-0 rank-3 */
    unsigned char sdram_data_drv_imp[4];	/* packed as intf-0 rank-0, intf-0 rank-1, intf-0 rank-2, intf-0 rank-3 */
    unsigned char sdram_rtt_park[4];		/* packed as intf-0 rank-0, intf-0 rank-1, intf-0 rank-2, intf-0 rank-3 */
    ncp_uint8_t	dq_map_0[18]; 		/* defines the DQ mapping of the output DQ pins from the MC/PHY per DIMM. For X9 specify all 18 bytes, for XLF specify info in 7:0 and 17th location. CMEM uses locations 5:0 */
    ncp_uint8_t	dq_map_1[18]; 		/* defines the DQ mapping of the output DQ pins from the MC/PHY per DIMM. For X9 specify all 18 bytes, for XLF specify info in 7:0 and 17th locatio. CMEM does not use this  */
    ncp_uint8_t	dq_map_odd_rank_swap_0; /* defines which chip-select per DIMM will use the DQ swapping. 4 bits representing chip-selects */
    ncp_uint8_t	dq_map_odd_rank_swap_1; /* XLF only: 4 bits representing chip-selects */
} ncp_per_sysmem_sdram_parms_t;

#endif	/* __UBOOT__ */

typedef enum {
    NCP_SM_MC_INIT_DONE = 1,
    NCP_SM_LP_OP_DONE,
    NCP_SM_BIST_DONE,
    NCP_SM_LVL_OP_DONE,
    NCP_SM_MR_OP_DONE
} ncp_sm_poll_event_t;

/*
 * PHY training modes.
 * These are the field definitions for the Denali 'sw_leveling_mode' 
 */
typedef enum {
    NCP_SYSMEM_PHY_NO_LEVELING = 0,
    NCP_SYSMEM_PHY_WRITE_LEVELING,
    NCP_SYSMEM_PHY_READ_LEVELING,
    NCP_SYSMEM_PHY_GATE_TRAINING 
} ncp_sm_phy_training_mode_t;

#ifdef UBOOT
#define NCP_CHIP_ACP34xx             1
#define NCP_CHIP_ACP32xx             2
#define NCP_CHIP_ACP25xx             6
#define NCP_CHIP_ACP25xx_V2          7
#define NCP_CHIP_ACP55xx             9       /* AXM55xx, aka X7     */
#define NCP_CHIP_ACP55xxV2_FPGA     10       /* X7v2, FPGA only  */
#define NCP_CHIP_ACP56xx            11       /* AXM56xx, aka X9     */
#define NCP_CHIP_ACP15xx            15       /* AXE15xx, aka X15    */
#define NCP_CHIP_ACP35xx            16       /* AXM35xx, aka X3     */
#define NCP_CHIP_ACPXLF             24       /* XLF */

extern unsigned ddrRecovery;
#else 
/* RTE code */
typedef ncp_uint32_t 
(*ncp_sm_intr_status_fn_t) (
    ncp_dev_hdl_t   dev,
    ncp_region_id_t regionId,
    ncp_uint32_t    mask);

typedef ncp_st_t 
(*ncp_sm_poll_controller_fn_t) (
    ncp_dev_hdl_t   dev,
    ncp_region_id_t regionId,
    ncp_sm_poll_event_t event);


typedef ncp_uint32_t 
(*ncp_sm_ecc_enb_fn_t) (
    ncp_dev_hdl_t   dev,
    ncp_region_id_t regionId,
    ncp_uint32_t    value);

typedef struct {
    ncp_uint8_t                 version;
    ncp_bool_t                  auto_detect;
    ncp_uint8_t                 num_interfaces;
    ncp_uint8_t                 num_ranks_per_interface;
    ncp_sm_topology_t           topology;
    ncp_sm_sdram_density_t      sdram_device_density;
    ncp_sm_sdram_width_t        sdram_device_width;
    ncp_sm_primary_bus_width_t  primary_bus_width;
    ncp_uint8_t                 CAS_latency;
    ncp_uint8_t                 CAS_write_latency;
    ncp_bool_t                  enableECC;
    ncp_uint8_t                 min_phy_cal_delay;
    ncp_uint8_t                 min_ctrl_roundtrip_delay;
    ncp_bool_t                  single_bit_mpr;
    ncp_uint32_t                rdcal_cmp_even;
    ncp_uint32_t                rdcal_cmp_odd;
    ncp_bool_t                  enable_deskew;
    ncp_bool_t                  enable_rdlvl;
    ncp_bool_t                  enable_auto_cpc;

    ncp_uint8_t                 added_rank_switch_delay;
    ncp_bool_t                  high_temp_dram;

    ncp_uint32_t                sdram_rtt_nom;
    ncp_uint32_t                sdram_rtt_wr;
    ncp_uint32_t                sdram_data_drv_imp;
    ncp_uint32_t                phy_adr_imp;
    ncp_uint32_t                phy_dat_imp;
    ncp_uint32_t                phy_rcv_imp;

    ncp_uint8_t                 phy_rdlat;
    ncp_uint8_t                 address_mirroring;
    ncp_uint8_t                 fixed_read_lat;
    ncp_uint8_t                 bubble_clobber;
    ncp_uint8_t                 syscacheMode;
    ncp_bool_t                  syscacheDisable;

    /* new for 5500 */
    ncp_uint32_t                zqcs_interval;
    ncp_bool_t                  enable_runtime_updates;
    ncp_uint8_t                 dramPrechargePolicy;
    ncp_uint32_t                open_page_size;
    ncp_per_sysmem_parms_t      per_mem[2];

    ncp_uint32_t                flags;
    ncp_bool_t                  half_mem;
    ncp_uint16_t                cmemMR1[2];

    ncp_bool_t                 ddrRetentionEnable;
    ncp_bool_t                 ddrRecovery;

    ncp_uint16_t               ddrClockSpeedMHz;
    ncp_uint8_t                 num_bytelanes;
    ncp_int64_t                 totalSize;

    /* RDIMM support */
    ncp_uint32_t		rdimm_ctl_0_0;
    ncp_uint32_t		rdimm_ctl_0_1;
    ncp_uint32_t		rdimm_misc;

    /* ODT Rd/Wr map support */
    ncp_uint32_t		read_ODT_ctl;
    ncp_uint32_t		write_ODT_ctl;

    /* new for 5600 */
    ncp_dram_class_t		dram_class;
    ncp_uint8_t			additive_latency;	/* number of dram clock cycles, typically (CAS_latency - MR1[A4:A3]) */
    ncp_uint8_t			binned_CAS_latency; 	/* from part definition like CL-nRCD-nRP */
    ncp_uint32_t		tck_ps;  		/* time period or rate at which part runs in pico-sec */
    ncp_refresh_mode_t		refresh_mode;		/* 1 means 1x, 2 means 2x, 4 means 4x */
    ncp_bool_t                  address_inversion;	/* applies only to ddr4 */
    ncp_uint8_t                 bstlen;			/* encoded burst_length during init and calculations */
    ncp_bool_t                  dm_masking;		/* data masking support availability on external dram */
    ncp_uint32_t		rdimm_ctl_0_2;		/* 152 bits (16 4-bit words + 11 8-bit words) of Std Control word per DIMM */
    ncp_uint32_t		rdimm_ctl_0_3;
    ncp_uint32_t		rdimm_ctl_0_4;
    ncp_uint32_t		rdimm_ctl_1_0;
    ncp_uint32_t		rdimm_ctl_1_1;
    ncp_uint32_t		rdimm_ctl_1_2;
    ncp_uint32_t		rdimm_ctl_1_3;
    ncp_uint32_t		rdimm_ctl_1_4;
    ncp_uint8_t			vref_cs;		/* target chip-select for vref training operation */
    ncp_uint8_t			vref_val; 		/* VREF value per DRAM for all chip-selects */
    ncp_uint16_t		wr_protect_en_mask;	/* 16-bit Mask indicating write protection enable per region */
    ncp_bool_t			dbi_rd_en;              /* data-bus inversion for rd enable */
    ncp_bool_t			dbi_wr_en;              /* data-bus inversion for wr enable */
    ncp_bool_t			ca_parity_en;           /* Common Address Parity enable */
    ncp_uint16_t		rdlvl_interval;		/* Max long count sequences between data eye training */
    ncp_uint16_t		rdlvl_gate_interval;	/* Max long count sequences between gate training */
    ncp_uint8_t			preamble_support;	/* defines the read (bit0) and write (bit 1) preamble support of b'0-1 cycle, or b'1-2 cycle preamble */
    ncp_uint8_t			crc_mode;		/* Bit(0): 1 to enable crc gen/checking, Bit(1): 1: performed in MC, 0: in PHY */

    ncp_per_sysmem_sdram_parms_t    per_smem[4];	/* X9 uses 0,1 indices, XLF uses 0,1,2,3 indices */
    ncp_uint32_t		packedDqDmDqsODT;
    ncp_uint32_t		packedAddrCmdCtrlOI;
    ncp_uint32_t		packedClockOI;
    ncp_uint32_t		packedDqDmDqsOI;
    ncp_uint32_t		packedAddrCmdCtrlClkSlewRates;
    ncp_uint32_t		packedDqDmDqsSlewRates;
    ncp_uint32_t		packedPHYTrainingOptions; /* All training options per RDL positions 9..17 in PIR */

    /* new for XLF */
    ncp_uint8_t         interface_sel; /* to support x1,x2,x3,x4 DRAM configuration as a bit-mask */
} ncp_sm_parms_t;

/* 
 * user-specified configuration parameters 
 */
typedef struct {
    ncp_sm_parms_t  sm_parms;
    ncp_sm_parms_t  cm_parms;
    ncp_mutex_t lock;
} ncp_sysmem_t;


 
/* maximum number of DIMMS */
#define NCP_SM_MAX_NUM_DIMMS 2

#define NCP_SYSMEM_PHY_TRAIN_DELAY_LOOPS 100
#define NCP_SYSMEM_PHY_TRAIN_DELAY_USEC  10

#define NCP_SM_DENALI_V1_ECC_INTR_BITS 0x3c
#define NCP_SM_DENALI_V1_ERROR_INTR_BITS 0x3feff

#define NCP_SM_DENALI_V2_ECC_INTR_BITS 0x78
#define NCP_SM_DENALI_V2_ERROR_INTR_BITS 0x7fdff


#define NCP_SM_TREEMEM_STATUS_CODE_OFFSET (NCP_ST_TREEMEM_PHY_CPC_CAL_TIMEOUT - NCP_ST_SYSMEM_PHY_CPC_CAL_TIMEOUT) 

NCP_API ncp_st_t
ncp_sysmem_config(ncp_t *ncp, ncp_map_t *map);

NCP_API ncp_st_t
ncp_sysmem_destroy(ncp_t *ncp);

ncp_st_t
ncp_sysmem_warm_restart(ncp_t *ncp);

NCP_API ncp_st_t
ncp_memory_layout_config(ncp_t *ncp, ncp_map_t *map);

NCP_API ncp_st_t
ncp_sysmem_init_fpga (ncp_dev_hdl_t dev, ncp_uint32_t smId, ncp_sm_parms_t *parms);

NCP_API ncp_st_t
ncp_treemem_init_fpga (ncp_dev_hdl_t dev, ncp_uint32_t treememId, ncp_sm_parms_t *parms);

NCP_API ncp_st_t
ncp_treemem_init_fpga_x9 (ncp_dev_hdl_t dev, ncp_uint32_t treememId, ncp_sm_parms_t *parms);

NCP_API ncp_st_t
ncp_sysmem_init_ibmphy (ncp_dev_hdl_t dev, 
                      ncp_uint32_t smId, 
                      ncp_sm_parms_t *parms);

NCP_API ncp_st_t
ncp_sysmem_init_lsiphy (ncp_dev_hdl_t dev, 
                      ncp_uint32_t smId, 
                      ncp_sm_parms_t *parms);

NCP_API ncp_st_t
ncp_sysmem_init_synopphy (ncp_dev_hdl_t dev, 
                      ncp_uint32_t smId, 
                      ncp_sm_parms_t *parms);
NCP_API ncp_st_t
ncp_treemem_init_synopphy (ncp_dev_hdl_t dev, 
                      ncp_uint32_t cmId, 
                      ncp_sm_parms_t *parms);

NCP_API ncp_st_t
ncp_syscache_disable (ncp_t *ncp);

NCP_API ncp_st_t
ncp_syscache_enable (ncp_t *ncp);

NCP_API ncp_st_t
ncp_syscache_init (ncp_t *ncp);

NCP_API ncp_st_t 
ncp_elm_init( ncp_dev_hdl_t dev, ncp_sm_parms_t *parms);

NCP_API ncp_st_t 
ncp_elm_sysmem_fill( ncp_dev_hdl_t dev, ncp_sm_parms_t *parms);


NCP_API ncp_st_t
ncp_cm_controller_init(
        ncp_dev_hdl_t dev,
        ncp_uint32_t  smId,
        ncp_sm_parms_t *parms);

NCP_API ncp_st_t
ncp_cm_dram_init(
        ncp_dev_hdl_t dev,
        ncp_uint32_t  num_cmem);

NCP_API ncp_st_t
ncp_sm_denali_enable(
    ncp_dev_hdl_t dev,
    ncp_uint32_t  smId,
    ncp_sm_parms_t *parms);
#endif
#endif
