/*
 *  Copyright (C) 2009 LSI Corporation
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>

#if defined (CONFIG_AXXIA_344X) | defined (CONFIG_AXXIA_342X)
#include "regs/ncp_denali_regs.h"
#include "regs/ncp_denali_reg_defines.h"
#include "regs/ncp_phy_regs.h"
#include "regs/ncp_phy_reg_defines.h"
#endif

#if defined (CONFIG_AXXIA_25xx) 
#include "regs/ncp_denali_regs_acp2500.h"
#include "regs/ncp_denali_reg_defines_acp2500.h"
#include "regs/ncp_phy_regs_acp2500.h"
#include "regs/ncp_phy_reg_defines_acp2500.h"
#endif


/*
  ===============================================================================
  ===============================================================================
  Parameters
  ===============================================================================
  ===============================================================================
*/

#define PARAMETERS_MAGIC 0x12af34ec

#define PARAMETERS_GLOBAL_IGNORE_VOLTAGE     0x00000001
#define PARAMETERS_GLOBAL_IGNORE_CLOCKS      0x00000010
#define PARAMETERS_GLOBAL_IGNORE_SYSMEM      0x00000100
#define PARAMETERS_GLOBAL_IGNORE_PCIESRIO    0x00001000

#define PARAMETERS_GLOBAL_RUN_SYSMEM_BIST    0x80000000
#define PARAMETERS_GLOBAL_SET_L2_FTC         0x40000000
#define PARAMETERS_GLOBAL_SET_L2_DISABLE_SL  0x20000000
#define PARAMETERS_GLOBAL_DISABLE_RESET      0x10000000
#define PARAMETERS_GLOBAL_SM_PHY_REG_RESTORE 0x08000000
#define PARAMETERS_GLOBAL_SM_PHY_REG_DUMP    0x04000000

typedef struct {
	unsigned long version;
	unsigned long flags;
} __attribute__((packed)) parameters_global_t;

typedef struct {
	unsigned long control;
} __attribute__((packed)) parameters_pciesrio_t;

typedef struct {
	unsigned long checksum;
	unsigned long version;
	unsigned long vofs;
	unsigned long tvid;
	unsigned long long twait;
	unsigned long VIDChecks;
	unsigned char vidLT[64];
} __attribute__((packed)) parameters_voltage_t;

typedef struct {
#ifdef CONFIG_AXXIA_25xx
	unsigned long syspll_prms;
	unsigned long syspll_ctrl;
	unsigned long syspll_mcgc;
	unsigned long syspll_mcgc1;
	unsigned long syspll_psd;
	unsigned long ppcpll_prms;
	unsigned long ppcpll_ctrl;
	unsigned long ppcpll_mcgc;
	unsigned long ppcpll_mcgc1;
	unsigned long ppcpll_psd;
	unsigned long smpll_prms;
	unsigned long smpll_ctrl;
	unsigned long smpll_mcgc;
	unsigned long smpll_mcgc1;
	unsigned long smpll_psd;
	unsigned long tmpll_prms;
	unsigned long tmpll_ctrl;
	unsigned long tmpll_mcgc;
	unsigned long tmpll_mcgc1;
	unsigned long tmpll_psd;
	unsigned long per_mcgc;
	unsigned long per_mcgc1;
#else
	unsigned long sys_control;
	unsigned long sys_lftune_upper;
	unsigned long sys_lftune_lower;
	unsigned long sys_fftune;
	unsigned long sys_locktune;
	unsigned long ppc_control;
	unsigned long ppc_lftune_upper;
	unsigned long ppc_lftune_lower;
	unsigned long ppc_fftune;
	unsigned long ppc_locktune;
	unsigned long ddr0_control;
	unsigned long ddr1_control;
	unsigned long ddr_lftune_upper;
	unsigned long ddr_lftune_lower;
	unsigned long ddr_fftune;
	unsigned long ddr_locktune;
#endif
} __attribute__ ((packed)) parameters_clocks_t;

typedef struct {
	unsigned long version;
	unsigned long auto_detect;
	unsigned long num_interfaces;
	unsigned long num_ranks_per_interface;
	unsigned long topology;
	unsigned long sdram_device_density;
	unsigned long sdram_device_width;
	unsigned long primary_bus_width;
	unsigned long CAS_latency;
	unsigned long CAS_write_latency;
	unsigned long enableECC;
	unsigned long enable_deskew;
	unsigned long enable_rdlvl;
	unsigned long enable_auto_cpc;
	unsigned long min_phy_cal_delay;
	unsigned long min_ctrl_roundtrip_delay;
	unsigned long single_bit_mpr;
	unsigned long rdcal_cmp_even;
	unsigned long rdcal_cmp_odd;
	unsigned long phy_rdlat;
	unsigned long added_rank_switch_delay;
	unsigned long high_temp_dram;
	unsigned long sdram_rtt_nom;
	unsigned long sdram_rtt_wr;
	unsigned long sdram_data_drv_imp;
	unsigned long phy_adr_imp;
	unsigned long phy_dat_imp;
	unsigned long phy_rcv_imp;
	unsigned long sysCacheMode;
	unsigned long syscacheDisable;
	unsigned long half_mem;
	unsigned long address_mirroring;
} __attribute__((packed)) parameters_sysmem_t;

typedef struct {
	unsigned long sysmemSize;
	unsigned long sysmemOffset;
	unsigned long clocksSize;
	unsigned long clocksOffset;
	unsigned long voltageSize;
	unsigned long voltageOffset;
	unsigned long pciesrioSize;
	unsigned long pciesrioOffset;
	unsigned long globalSize;
	unsigned long globalOffset;
	unsigned long version;
	unsigned long checksum;
	unsigned long size;
	unsigned long magic;
} __attribute__((packed)) parameters_header_t;

#define PARAMETERS_HEADER_ADDRESS \
(LCM + (128 * 1024) - sizeof(parameters_header_t))
static parameters_header_t *header =
(parameters_header_t *)PARAMETERS_HEADER_ADDRESS;

static parameters_global_t *global = (parameters_global_t *)1;
static parameters_pciesrio_t *pciesrio = (parameters_pciesrio_t *)1;
static parameters_voltage_t *voltage = (parameters_voltage_t *)1;
static parameters_clocks_t *clocks = (parameters_clocks_t *)1;
static parameters_sysmem_t *sysmem = (parameters_sysmem_t *)1;

#define PARAMETERS_OFFSET (127 * 1024)
#define PARAMETERS_ADDRESS (LCM + PARAMETERS_OFFSET)
#define PARAMETERS_SIZE (1024)

/*
  ===============================================================================
  ===============================================================================
  Local
  ===============================================================================
  ===============================================================================
*/

unsigned long sysmem_size = 1;
unsigned long reset_enabled = 1;
unsigned long ncp_sm_phy_reg_restore = 1;
unsigned long ncp_sm_phy_reg_dump = 1;

/*
  ----------------------------------------------------------------------
  fill_sysmem
*/

static void
fill_sysmem(unsigned long long address, unsigned long long size,
	    unsigned long syscache_nodes)
{
	unsigned long sc_nodes[] = {
		0x20, 0x1e, 0x21, 0x1d, 0x11, 0x12, 0x10, 0x13
	};
	unsigned long sc_version;
	int i;

	ncr_read32(NCP_REGION_ID(sc_nodes[0], 0xff), 0, &sc_version);
	sc_version &= 0xffff;

	if (1 <= sc_version) {
		/* After version 0, use the hardware function. */
		unsigned long long sysmem_size_bytes = (1ULL << sysmem_size);
		unsigned long words;

		words = (unsigned long)
			(sysmem_size_bytes / (16 * syscache_nodes)) - 1;

		for (i = 0; i < syscache_nodes; ++i) {
			ncr_write32(NCP_REGION_ID(sc_nodes[i], 0), 0x2c, words);
			ncr_write32(NCP_REGION_ID(sc_nodes[i], 5), 0x0, 0);
			ncr_write32(NCP_REGION_ID(sc_nodes[i], 5), 0x4, 0);
			ncr_write32(NCP_REGION_ID(sc_nodes[i], 5), 0x8, 0);
			ncr_write32(NCP_REGION_ID(sc_nodes[i], 5), 0xc, 0);
		}

		for (i = 0; i < syscache_nodes; ++i) {
			unsigned long temp;

			do {
				udelay(100);
				ncr_read32(NCP_REGION_ID(sc_nodes[i], 0), 0x2c,
					   &temp);
			} while (0 != temp);
		}
	} else {
		/* Version 0, no hardware function available. */
		unsigned long long offset = 0;
		unsigned long remaining = (1 << (sysmem_size - 20));

		while (256 <= remaining) {
			unsigned long tlb_offset;

			tlb_offset =
				((offset & 0xffffffff00000000ULL) >> 32) |
				(offset & 0xfffff000UL);

			/* Create a TLB entry. */
			__asm__ __volatile__ ( "tlbwe %1,%0,0\n"               \
					       "tlbwe %2,%0,1\n"               \
					       "tlbwe %3,%0,2\n"               \
					       "isync\n" : :
					       "r" (0x80000000),
					       "r" (0xa00009f0),
					       "r" (tlb_offset),
					       "r" (0x00030207));

			memset((void *)0xa0000000, 0, (256 * 1024 * 1024));

			/* Take down the TLB entry. */
			__asm__ __volatile__ ( "tlbwe %1,%0,0\n"               \
					       "tlbwe %2,%0,1\n"               \
					       "tlbwe %3,%0,2\n"               \
					       "isync\n"
					       : :
					       "r" (0x80000000),
					       "r" (0xa0000000),
					       "r" (0x00000000),
					       "r" (0x00000000));

			offset += (256 * 1024 * 1024);
			remaining -= 256;
		}
	}

	return;
}

#ifndef CONFIG_AXXIA_25xx

/*
  ----------------------------------------------------------------------
  read_vc
*/

static int
read_vc(unsigned long *vc)
{
	unsigned long value;
	int rc;

	rc = ncr_modify32(NCP_REGION_ID(0xa, 0x10), 0x48,
			  0x77000000, 0x77000000);
	rc |= ncr_read32(NCP_REGION_ID(0xa, 0x10), 0x40, &value);
	*vc = (value & 0x700) >> 8;

	if (0 != rc)
		return -1;

	return 0;
}

/*
  ----------------------------------------------------------------------
  write_vc
*/

static int
write_vc(unsigned long vc)
{
	int rc;
	unsigned long value;

	value = (vc & 0x7) << 16;
	rc = ncr_modify32(NCP_REGION_ID(0xa, 0x10), 0x48,
			  0x00770000, (0x00700000 | value));

	if (0 != rc)
		return -1;

	return 0;
}

/*
  ----------------------------------------------------------------------
  read_vid
*/

static int
read_vid(unsigned long *vid)
{
	unsigned long value;
	int rc;

	rc = ncr_modify32(NCP_REGION_ID(0xa, 0x10), 0x48,
			  0x00000600, 0x00000600);
	rc |= ncr_read32(NCP_REGION_ID(0xa, 0x10), 0x40, &value);
	*vid = value & 0xff;

	if (0 != rc)
		return -1;

	return 0;
}

/*
  ----------------------------------------------------------------------
  write_vid
*/

static int
write_vid(unsigned long vid)
{
	int rc;
	unsigned long value;

	value = vid & 0xff;

	if (0x5a > value) {
#ifdef VID_VERBOSE
		printf("Ignoring VID value out of range (less than 0x5a).\n");
#endif
		return 0;
	}

#ifdef VID_VERBOSE
	printf("Writing VID, input=0x%x value=0x%x.\n", vid, value);
#endif
	rc = ncr_modify32(NCP_REGION_ID(0xa, 0x10), 0x48,
			  0x000001ff, (0x00000100 | value));

	if (0 != rc)
		return -1;

	return 0;
}

/*
  ----------------------------------------------------------------------
  voltage_init
*/

static int
voltage_init(void)
{
	unsigned long ecid_93;
	unsigned long svb_psro;
	unsigned long VC;
	unsigned long VIDsafe;
	unsigned long VIDrun;
	int rc = 0;

	/*
	  Read bit 93 and the svb_psro field from the ECID fuses.
	*/

	rc |= ncr_read32(NCP_REGION_ID(0xa, 0x10), 0x34, &svb_psro);

	if (0 != (svb_psro & 0x20000000))
		ecid_93 = 1;
	else
		ecid_93 = 0;

	svb_psro = (svb_psro & 0x07c00000) >> 22;

	/*
	  Read the board power supply architecture from VC[2:0] and
	  VIDsafe from VID[7:0].
	*/

	rc |= read_vc(&VC);

	if (0 == VC) {
#ifdef VID_VERBOSE
		printf("Powering up in Generic Core Supply Configuration.\n");
#endif

		return 0;
	} else if ((1 == VC) ||
		   (4 == VC) ||
		   (5 == VC) ||
		   (6 == VC) ||
		   (7 == VC)) {
		/*
		  Unsupported VC codes: 1, 4, 5, 6, and 7.
		*/
#ifdef VID_VERBOSE
		printf("Unsupported VC Setting (%d), Defaulting to Generic "
		       "Power Supply Configuration.\n", VC);
#endif

		return 0;
	} else if (2 == VC) {
		if (0 == ecid_93) {
#ifdef VID_VERBOSE
			printf("Unsupported ECID [93] Setting, Defaulting "
			       "to Generic Power Supply Configuration.\n");
#endif

			return 0;
		} else {
#ifdef VID_VERBOSE
			printf("Powering up in AVS Configuration.\n");
#endif
		}
	} else if (3 == VC) {
#ifdef VID_VERBOSE
		printf("Entering AVS test mode for LSI lab use only.\n");
#endif
	} else {
		/*
		  Unhandled VC code.
		*/
#ifdef VID_VERBOSE
		printf("Unhandled VC Code (%d), Defaulting to Generic "
		       "Power Supply Configuration.\n", VC);
#endif

		return -1;
	}

	/*
	  Read the current VID value.
	*/

	rc |= read_vid(&VIDsafe);
#ifdef VID_VERBOSE
	printf("Sampled VIDsafe, 0x%x\n", VIDsafe);
#endif

	/*
	  Read the chip's optimal operating voltage set point code
	  (VIDrun) from the lookup table (vidLT[VC][ECID.svb_psro]).
	*/

	VIDrun = voltage->vidLT[svb_psro];

	if (0x5a > VIDsafe)
		VIDsafe = 0x5a;

	if (VIDsafe < VIDrun) {
		while (VIDsafe <= VIDrun) {
			write_vid(VIDsafe++);
			udelay(voltage->tvid);
		}
	} else if (VIDsafe > VIDrun) {
		while (VIDsafe >= VIDrun) {
			write_vid(VIDsafe--);
			udelay(voltage->tvid);
		}
	}

#ifdef VID_VERBOSE
	printf("Powering up in Common Core Supply Configuration.\n");
#endif

	return 0;
}

#endif

#ifdef CONFIG_AXXIA_25xx

/*
  ------------------------------------------------------------------------------
  pll_init_2500
*/

#define PARAMETER_MASK 0x3ffffff
#define CONTROL_MASK 0x1f300

static int
pll_init_2500(unsigned long offset, unsigned long *parameters)
{
	int timeout = 10000;
	unsigned long value;

	/*
	  Enable the PLL.
	*/

	dcr_write(0xe0000000, offset);
	mdelay(1);		/* Required in practice (if not in theory)... */

	/*
	  Set the parameter value and reset the PLL, with
	  bypass and bypass_select (in the control register),
	  set.
	*/

	dcr_write((parameters[1] & CONTROL_MASK) | 0xc00, (offset + 1));
	dcr_write((parameters[0] & PARAMETER_MASK) | 0xe0000000, offset);
	dcr_write((parameters[1] & CONTROL_MASK) | 0xc02, offset + 1);

	/*
	  Wait for pll_locked.
	*/

	do {
		value = dcr_read(offset + 0xa);
	} while ((0 == (value & 0x80000000) &&
		  (0 < --timeout)));

	if (0 == timeout)
		return -1;

	/*
	  Clear bypass.
	*/

	dcr_write((parameters[1] & CONTROL_MASK) | 0x402, offset + 1);

	/*
	  Update the mcgc.
	*/

	if (0 != parameters[2]) {
		value = dcr_read(0xd00);
		value |= (parameters[2] | 0x800);
		dcr_write(value, 0xd00);
		value &= ~0x800;
		dcr_write(value, 0xd00);
	}

	/*
	  Update the mcgc1.
	*/

	if (0 != parameters[2]) {
		value = dcr_read(0xd01);
		value |= (parameters[3] | 0x3);
		dcr_write(value, 0xd01);
		value &= ~0x3;
		dcr_write(value, 0xd01);
	}

	/*
	  Delay.
	*/

	udelay(parameters[4]);

	return 0;
}

/*
  ------------------------------------------------------------------------------
  clocks_init
*/

static int
clocks_init( void )
{
	unsigned long value;

#ifdef DISPLAY_PARAMETERS
	printf("-- -- Clocks\n"
	       "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n"
	       "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n"
	       "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n"
	       "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n"
	       "0x%lx 0x%lx\n",
	       clocks->syspll_prms, clocks->syspll_ctrl, clocks->syspll_mcgc,
	       clocks->syspll_mcgc1, clocks->syspll_psd, clocks->ppcpll_prms,
	       clocks->ppcpll_ctrl, clocks->ppcpll_mcgc, clocks->ppcpll_mcgc1,
	       clocks->ppcpll_psd, clocks->smpll_prms, clocks->smpll_ctrl,
	       clocks->smpll_mcgc, clocks->smpll_mcgc1, clocks->smpll_psd,
	       clocks->tmpll_prms, clocks->tmpll_ctrl, clocks->tmpll_mcgc,
	       clocks->tmpll_mcgc1, clocks->tmpll_psd, clocks->per_mcgc,
	       clocks->per_mcgc1);
#endif

	/*
	  Enable protected writes
	  ncpWrite   0x18d.0.0x0000000000 0x000000ab
	*/

	value = dcr_read(0xd00);
	value |= 0xab;
	dcr_write(value, 0xd00);

#ifdef PPCPLL_STEP_TEST
	pll_init_2500(0xd40, &clocks->syspll_prms); /* system */

	/*
	  PPC, Stepped...
	*/

	dcr_write(0xa0000000, 0xd50);
	mdelay(1);
	dcr_write(0x14f00, 0xd51);
	/*dcr_write(0xa0240551, 0xd50);*/
	dcr_write(0xa02405d2, 0xd50);
	dcr_write(0x14f02, 0xd51);

	do {
		value = dcr_read(0xd5a);
	} while (0 == (value & 0x80000000));

	value = dcr_read(0xd00);
	value |= 0x40000000;
	dcr_write(value, 0xd00);

	dcr_write(0x14702, 0xd51);
	mdelay(1000);

	value = dcr_read(0xd00);
	value &= 0xfffff7ff;
	dcr_write(value, 0xd00);

	value = dcr_read(0xd00);
	value |= 0x00100000;
	dcr_write(value, 0xd00);

	value = dcr_read(0xd00);
	value |= 0x00000800;
	dcr_write(value, 0xd00);

	mdelay(1000);

	value = dcr_read(0xd00);
	value &= 0xfffff7ff;
	dcr_write(value, 0xd00);

	value = dcr_read(0xd00);
	value |= 0x80000000;
	dcr_write(value, 0xd00);

	mdelay(1000);

	value = dcr_read(0xd00);
	value &= 0xff0fffff;
	dcr_write(value, 0xd00);

	value = dcr_read(0xd00);
	value |= 0x00000800;
	dcr_write(value, 0xd00);

	mdelay(1000);

	value = dcr_read(0xd00);
	value &= 0xfffff7ff;
	dcr_write(value, 0xd00);

	mdelay(1000);

	/*
	  Put TM in reset.

	  0x18e.0.0xc - Reset Module Register
	       tm_phy_io_rst and tm_clksync_rst
	*/

	dcr_write(0xc00000, 0x1703);

	pll_init_2500(0xd60, &clocks->tmpll_prms); /* tree memory */

	/*
	  Re-enable TM.
	*/

	dcr_write(0, 0x1703);

	/*
	  Put SM in reset.

	  0x18e.0.0xc - Reset Module Register
	       sm_phy_io_rst and sm_clksync_rst
	*/

	dcr_write(0xc0000, 0x1703);

	pll_init_2500(0xd70, &clocks->smpll_prms); /* system memory */

	/*
	  Re-enable SM.
	*/

	dcr_write(0, 0x1703);

	/*
	  -----------------------------------------------------------------------
	  Peripheral Clock Setup
	*/

	switch ((clocks->per_mcgc1 & 0xc000) >> 14) {
	case 0:
		/* Stay on the reference clock. */
		break;
	case 1:
		/* Use the PPC clock. */
		value = dcr_read(0xd00);
		value |= (clocks->per_mcgc | 0x800);
		dcr_write(value, 0xd00);
		value &= ~0x800;
		dcr_write(value, 0xd00);
		break;
	case 2:
	case 3:
		/* Use the SYS clock. */
		value = dcr_read(0xd01);
		value |= ((clocks->per_mcgc1 & 0xf00) | 0x2);
		dcr_write(value, 0xd01);
		value &= ~0x2;
		dcr_write(value, 0xd01);
		break;
	default:
		break;
	}

	value = dcr_read(0xd01);
	value |= (clocks->per_mcgc1 & 0xc000);
	dcr_write(value, 0xd01);
#else
	/*
	  -----------------------------------------------------------------------
	  PLL Setup
	  -----------------------------------------------------------------------
	*/

	pll_init_2500(0xd40, &clocks->syspll_prms); /* system */
	pll_init_2500(0xd50, &clocks->ppcpll_prms); /* ppc */

	/*
	  Put TM in reset.

	  0x18e.0.0xc - Reset Module Register
	       tm_phy_io_rst and tm_clksync_rst
	*/

	dcr_write(0xc00000, 0x1703);

	pll_init_2500(0xd60, &clocks->tmpll_prms); /* tree memory */

	/*
	  Re-enable TM.
	*/

	dcr_write(0, 0x1703);

	/*
	  Put SM in reset.

	  0x18e.0.0xc - Reset Module Register
	       sm_phy_io_rst and sm_clksync_rst
	*/

	dcr_write(0xc0000, 0x1703);

	pll_init_2500(0xd70, &clocks->smpll_prms); /* system memory */

	/*
	  Re-enable SM.
	*/

	dcr_write(0, 0x1703);

	/*
	  -----------------------------------------------------------------------
	  Peripheral Clock Setup
	*/

	switch ((clocks->per_mcgc1 & 0xc000) >> 14) {
	case 0:
		/* Stay on the reference clock. */
		break;
	case 1:
		/* Use the PPC clock. */
		value = dcr_read(0xd00);
		value |= (clocks->per_mcgc | 0x800);
		dcr_write(value, 0xd00);
		value &= ~0x800;
		dcr_write(value, 0xd00);
		break;
	case 2:
	case 3:
		/* Use the SYS clock. */
		value = dcr_read(0xd01);
		value |= ((clocks->per_mcgc1 & 0xf00) | 0x2);
		dcr_write(value, 0xd01);
		value &= ~0x2;
		dcr_write(value, 0xd01);
		break;
	default:
		break;
	}

	value = dcr_read(0xd01);
	value |= (clocks->per_mcgc1 & 0xc000);
	dcr_write(value, 0xd01);

	/*
	  Disable protected writes
	  ncpWrite   0x18d.0.0x0000000000 0
	*/

	value = dcr_read(0xd00);
	value &= ~0xff;
	dcr_write(value, 0xd00);
#endif

	return 0;
}

#else

/*
  -------------------------------------------------------------------------------
  clocks_ddr_init
*/

static int
clocks_ddr_init( unsigned long node, unsigned long value,
		 unsigned long tune1, unsigned long tune2, unsigned long tune3 )
{
	ncr_write32( NCP_REGION_ID( node, 0 ), 4, 0x18000000 );
	ncr_write32( NCP_REGION_ID( node, 0 ), 4, 0x98000000 );
	ncr_write32( NCP_REGION_ID( node, 0 ), 0, value );
	ncr_write32( NCP_REGION_ID( node, 0 ), 8, tune1 );
	ncr_write32( NCP_REGION_ID( node, 0 ), 0xc, tune2 );
	ncr_write32( NCP_REGION_ID( node, 0 ), 0x10, tune3 );
	ncr_write32( NCP_REGION_ID( node, 0 ), 4, 0x18000000 );

	do {
		ncr_read32( NCP_REGION_ID( node, 0 ), 0x1c, & value );
	} while( 1 != ( value & 1 ) );

	ncr_write32( NCP_REGION_ID( node, 0 ), 4, 0x10000000 );

	return 0;
}

/*
  ------------------------------------------------------------------------------
  clocks_init
*/

#define DDR_TUNE1 0x00000a00
#define DDR_TUNE2 0x00000050
#define DDR_TUNE3 0x00100000

#define PPCSYS_TUNE1 0x28000800
#define PPCSYS_TUNE2 0x00000000
#define PPCSYS_TUNE3 0x00000a00

static int
clocks_init( void )
{
	int type;
	unsigned long control;
	unsigned long tune1;
	unsigned long tune2;
	unsigned long tune3;

#ifdef DISPLAY_PARAMETERS
	printf("-- -- Clocks\n"
	       "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n"
	       "0x%lx 0x%lx 0x%lx 0x%lx 0x%lx\n"
	       "0x%lx 0x%lx\n"
	       "0x%lx 0x%lx 0x%lx 0x%lx\n",
	       clocks->sys_control,
	       clocks->sys_lftune_upper,
	       clocks->sys_lftune_lower,
	       clocks->sys_fftune,
	       clocks->sys_locktune,
	       clocks->ppc_control,
	       clocks->ppc_lftune_upper,
	       clocks->ppc_lftune_lower,
	       clocks->ppc_fftune,
	       clocks->ppc_locktune,
	       clocks->ddr0_control, clocks->ddr1_control,
	       clocks->ddr_lftune_upper,
	       clocks->ddr_lftune_lower,
	       clocks->ddr_fftune,
	       clocks->ddr_locktune);
#endif

	/* ppc and sys */
	for( type = 0; type <= 1; ++ type ) {
		unsigned long value;
		unsigned long field;

		if (0 == type)
			control = clocks->sys_control;
		else
			control = clocks->ppc_control;

		/* 0xd00: mcgc */
		value = dcr_read( 0xd00 );
		
		if( 0 == type ) {
			field = ( value >> 28 ) & 3;
		} else {
			field = ( value >> 30 ) & 3;
		}

		switch( field ) {
		case 1:
			if( 0 == type ) {
				value &= 0xefffffff;
			} else {
				value &= 0xb7ffffff;
			}
			/* 0xd00: mcgc */
			dcr_write( value, 0xd00 );
			break;
		case 2:
			if( 0 == type ) {
				value &= 0xdfffffff;
			} else {
				value &= 0x77ffffff;
			}
			/* 0xd00: mcgc */
			dcr_write( value, 0xd00 );
			break;
		case 3:
			if( 0 == type ) {
				value &= 0xefffffff;
			} else {
				value &= 0xb7ffffff;
			}
			/* 0xd00: mcgc */
			dcr_write( value, 0xd00 );
			if( 0 == type ) {
				value &= 0xdfffffff;
			} else {
				value &= 0x77ffffff;
			}
			/* 0xd00: mcgc */
			dcr_write( value, 0xd00 );
			break;
		}

		/* 0xd02: ppc_pll_ctrl */
		/* 0xd03: sys_pll_ctrl */
		dcr_write( control, ( 0 == type ) ? 0xd03 : 0xd02 );

		tune1 =	((clocks->sys_lftune_upper & 0x1ff) << 23) |
			((clocks->sys_lftune_lower & 0xfffffe00) >> 9);
		tune2 =	((clocks->sys_lftune_lower & 0x1ff) << 23);
		tune3 = ((clocks->sys_locktune & 0xffff) << 16) |
			(clocks->sys_fftune & 0xffff);
#ifdef DISPLAY_PARAMETERS
		serial_early_init();
		printf("%s Tune Values: 0x%08lx 0x%08lx 0x%08lx\n",
		       (0 == type) ? "SYS" : "PPC", tune1, tune2, tune3);
#endif

		/* 0xd04: ppc_pll_tune1 */
		/* 0xd07: sys_pll_tune1 */
		dcr_write( tune1, ( 0 == type ) ? 0xd07 : 0xd04 );
		/* 0xd05: ppc_pll_tune2 */
		/* 0xd08: sys_pll_tune2 */
		dcr_write( tune2, ( 0 == type ) ? 0xd08 : 0xd05 );
		/* 0xd06: ppc_pll_tune3 */
		/* 0xd09: sys_pll_tune3 */
		dcr_write( tune3, ( 0 == type ) ? 0xd09 : 0xd06 );
		/* 0xd02: ppc_pll_ctrl */
		/* 0xd03: sys_pll_ctrl */
		value = dcr_read( ( 0 == type ) ? 0xd03 : 0xd02 );
		value &= 0xfffffffe;
		dcr_write( value, ( 0 == type ) ? 0xd03 : 0xd02 );

		do {
			/* 0xd01: mcgs */
			value = dcr_read( 0xd01 );

			if( 0 == type ) {
				value = ( value >> 30 ) & 1;
			} else {
				value = ( value >> 31 ) & 1;
			}
		} while( 0 == value );

		/* 0xd00: mcgc */
		value = dcr_read( 0xd00 );

		if( 0 == type ) {
			value |= 0x10000000;
		} else {
			value |= 0x40000000;
		}

		/* 0xd00: mcgc */
		dcr_write( value, 0xd00 );

		/* Switch the per_clock from clk_ref0 to clk_pllb_ppc. */
		if( 1 == type ) {
			/*
			  If there are any problems with PLL locking,
			  swithcing the peripherals off of clk_ref0
			  will cause problems.
			*/
			/* 0xd00: mcgc */
			value = dcr_read( 0xd00 );
			value |= 0x08000000;
			/* 0xd00: mcgc */
			dcr_write( value, 0xd00 );
		}
	}

	tune1 = ((clocks->ddr_locktune & 0xffff) << 16) |
		(clocks->ddr_fftune & 0xffff);
	tune2 = clocks->ddr_lftune_upper & 0x1ff;
	tune3 = clocks->ddr_lftune_lower;

#ifdef DISPLAY_PARAMETERS
	serial_early_init();
	printf("DDR Tune Values: 0x%08lx 0x%08lx 0x%08lx\n",
	       tune1, tune2, tune3);
#endif
	clocks_ddr_init( 0x23, clocks->ddr0_control,
			 tune1, tune2, tune3 );
	clocks_ddr_init( 0x0d, clocks->ddr1_control,
			 tune1, tune2, tune3 );

	return 0;
}

#endif

#if defined(ACP_EMU)
#include "sysmem_emulation.c"
#else
#include "sysmem_asic_common.c"
#if defined (CONFIG_AXXIA_344X) || defined (CONFIG_AXXIA_342X)
#include "ncp_sysmem_init_ibmphy.c"
#endif
#if defined (CONFIG_AXXIA_25xx) 
#include "ncp_sysmem_init_lsiphy.c"
#endif
#endif

#if !defined(ACP_EMU)

#if defined(ACP_X1V1)
#define INT_STATUS_OFFSET 0x16c
#define BIST_COMPLETION 0x200
#define ECC_ERROR_MASK 0x3c
#elif defined(CONFIG_AXXIA_344X) || defined(CONFIG_AXXIA_342X)
#define INT_STATUS_OFFSET 0x16c
#define BIST_COMPLETION 0x400
#define ECC_ERROR_MASK 0x78
#elif defined(CONFIG_AXXIA_25xx)
#define INT_STATUS_OFFSET 0x410
#define BIST_COMPLETION 0x800
#define ECC_ERROR_MASK 0x78
#endif

/*
  ----------------------------------------------------------------------
  acp_sysmem_bist_failure
*/

static void
acp_sysmem_bist_failure(unsigned long region)
{
	int i;
	unsigned long value;
	unsigned long offsets [] =
	  { 0x248, 0x24c,
#ifdef CONFIG_AXXIA_25xx
	    0x418, 0x41c, 0x420, 0x424, 0x428, 0x42c, 0x430, 0x434,
	    0x438, 0x43c, 0x440, 0x444, 0x448, 0x44c, 0x450, 0x454
#else
	    0x2b0, 0x2b4, 0x2b8, 0x2bc, 0x2c0, 0x2c4, 0x2c8, 0x2cc
#endif
	  };

	for (i = 0; i < (sizeof(offsets) / sizeof(unsigned long)); ++i) {
		ncr_read32(region, offsets[i], &value);
		printf("\t\tRegion:0x%08lx Offset:0x%04lx Value:0x%08lx\n",
		       region, offsets[i], value);
	}

	return;
}

/*
  ----------------------------------------------------------------------
  acp_sysmem_asic_check_ecc
*/

static void
acp_sysmem_asic_check_ecc(unsigned long region)
{
	unsigned long value;

	ncr_read32(region, NCP_DENALI_CTL_20, &value);

	if (3 != ((ncp_denali_DENALI_CTL_20_t *)&value)->ctrl_raw) {
		printf("ECC is not enabled for node 0x%03lx\n",
		       NCP_NODE_ID(region));
		return;
	}

	ncr_read32(region, INT_STATUS_OFFSET, &value);

	if (0 == (value & ECC_ERROR_MASK)) {
		printf("No ECC Errors Detected on Node 0x%03lx.\n",
		       NCP_NODE_ID(region));
	} else {
		int i;
		unsigned long offsets [] = {
			0x0ac, 0x258, 0x260, 0x264, 0x288, 0x28c, 0x290, 0x294
		};

		printf("ECC Errors Detected on Node 0x%03lx: 0x%02lx\n",
		       NCP_NODE_ID(region), (value & ECC_ERROR_MASK));

		for (i = 0; i < (sizeof(offsets) / sizeof(unsigned long));
		     ++ i) {
			ncr_read32(region, offsets[i], &value);
			printf("\tRegion:0x%08lx Offset:0x%04lx Value:0x%08lx\n",
			       region, offsets[i], value);
		}
	}

	return;
}

/*
  ----------------------------------------------------------------------
  acp_sysmem_bist_start
*/

static void
acp_sysmem_bist_start( unsigned long region, int bits, int test )
{
	/* Disable BIST_GO parameter */
	ncr_and( region, 0x8, 0xfffffffe );

	/* Unset the previous data and address test modes. */
	ncr_and( region, 0x4, 0xfefeffff );

	/* Program to test either address or data. */
	if( 1 == test ) {
		ncr_or( region, 0x4, 0x00010000 );
	} else {
		ncr_or( region, 0x4, 0x01000000 );
	}

	/* Program the start address. */
	ncr_write32( region, 0x250, 0 );
	ncr_write32( region, 0x254, 0 );

	/* Program the data mask. */
#if defined(CONFIG_AXXIA_25xx)
	ncr_write32( region, 0x3f8, 0 );
	ncr_write32( region, 0x3fc, 0 );
	ncr_write32( region, 0x400, 0 );
	ncr_write32( region, 0x404, 0 );
#else
	ncr_write32( region, 0x280, 0 );
	ncr_write32( region, 0x284, 0 );
#endif

	/*
	  The end address is specified by the number
	  of address bits.
	*/
	if( 1 == test ) {
		/*
		  For address checking, the spec says
		  the register value should be one
		  less than the calculated value.
		*/
		ncr_or( region, 0xa4,	( ( bits - 1 ) << 24 ) );
	} else {
		/*
		  For data checking we just use the
		  value calculated above for the
		  specified memory size. (i.e. 2GB ==
		  31 bits).
		*/
		ncr_or( region, 0xa4,	( bits << 24 ) );
	}

	/* Erase the interrupt status from the previous run. */
#if defined(CONFIG_AXXIA_25xx)
	ncr_or( region, 0x164, 0x200 );
#else
	ncr_or( region, 0x164, 0x600 );
#endif

	/* Start the test. */
	ncr_or( region, 0x8, 0x1 );

	return;
}

/*
  ----------------------------------------------------------------------
  acp_sysmem_bist
*/

#define PARALLEL_BIST

static void
acp_sysmem_bist( void )
{
	unsigned long size =
		( 1 << ( sysmem_size - 20 ) ) / sysmem->num_interfaces;
	unsigned long bits = 20;
	int test;
	unsigned long result;
	int smid;
	unsigned long interrupt_status;

	printf("Running the Built In Self Test.\n");
	acp_sysmem_asic_check_ecc(NCP_REGION_ID(0x022, 0));

	if( 1 < sysmem->num_interfaces )
		acp_sysmem_asic_check_ecc(NCP_REGION_ID(0x00f, 0));

	while( 0 == ( size & 1 ) ) {
		++ bits;
		size = size >> 1;
	}

#ifdef PARALLEL_BIST
	for( test = 1; test >= 0; -- test ) {
		unsigned long smregion0 = NCP_REGION_ID(0x22, 0);
		unsigned long smregion1 = NCP_REGION_ID(0xf, 0);
		unsigned long delay_loops;

		if( 1 == test ) {
			printf( "ADDRESS Check MBIST on all nodes...\n" );
			delay_loops = 20000;
		} else {
			printf( "DATA Check MBIST on all nodes...\n" );
			delay_loops = 100000;
		}

#if defined(ACP_EMU)
		delay_loops *= 100;
#endif
		
		acp_sysmem_bist_start( smregion0, bits, test );

		if( 1 < sysmem->num_interfaces )
			acp_sysmem_bist_start( smregion1, bits, test );

		/* Poll for completion and get the results. */
		if( 0 != ncr_poll( smregion0, INT_STATUS_OFFSET,
				   BIST_COMPLETION, BIST_COMPLETION,
				   10000, delay_loops ) ) {
			printf( "SM Node 0x%lx Didn't Complete.\n",
				NCP_NODE_ID( smregion0 ) );
		} else {
			ncr_read32(smregion0, INT_STATUS_OFFSET,
				   &interrupt_status);
			ncr_write32(smregion0, 0x164, interrupt_status);
			ncr_read32( smregion0, 0x50, & result );

			if( result & ( 1 << test ) ) {
				printf( "\tSM Node 0 PASSED\n" );
			} else {
				printf( "\tSM Node 0 FAILED\n" );
				acp_sysmem_bist_failure( smregion0 );
			}
		}

		if( 1 < sysmem->num_interfaces ) {
			if( 0 != ncr_poll( smregion1, INT_STATUS_OFFSET,
					   BIST_COMPLETION, BIST_COMPLETION,
					   10000, delay_loops ) ) {
				printf( "SM Node 0x%lx Didn't Complete.\n",
					NCP_NODE_ID( smregion1 ) );
			} else {
				ncr_read32(smregion1, INT_STATUS_OFFSET,
					   &interrupt_status);
				ncr_write32(smregion1, 0x164, interrupt_status);
				ncr_read32( smregion1, 0x50, & result );
				
				if( result & ( 1 << test ) ) {
					printf( "\tSM Node 1 PASSED\n" );
				} else {
					printf( "\tSM Node 1 FAILED\n" );
					acp_sysmem_bist_failure( smregion1 );
				}
			}
		}

		/*
		  Make sure to disable before letting the
		  system access system memory.  Disabling
		  BIST_GO param.
		*/
		ncr_and( smregion0, 0x8, 0xfffffffe );

		do {
			ncr_read32(smregion0, 0x8, &result);
		} while (0 != (result & 1));

		if( 1 < sysmem->num_interfaces ) {
			ncr_and( smregion1, 0x8, 0xfffffffe );

			do {
				ncr_read32(smregion1, 0x8, &result);
			} while (0 != (result & 1));
		}
	}
#else
	for( smid = 0 ; smid < sysmem->num_interfaces ; ++ smid ) {
		unsigned long smregion;

		if( 0 == smid )
			smregion = NCP_REGION_ID( 34, 0);
		else
			smregion = NCP_REGION_ID( 15, 0);

		for( test = 1; test >= 0; -- test ) {
			if( 1 == test ) {
				printf( "SM Node %d ADDRESS Check... ",
					NCP_NODE_ID( smregion ) );
			} else {
				printf( "SM Node %d DATA Check... ",
					NCP_NODE_ID( smregion ) );
			}

			acp_sysmem_bist_start( smregion, bits, test );

			/* Poll for completion. */
			ncr_poll( smregion, INT_STATUS_OFFSET,
				  BIST_COMPLETION, BIST_COMPLETION,
				  1000000, 10000 );

			ncr_read32(smregion, INT_STATUS_OFFSET,
				   &interrupt_status);
			ncr_write32(smregion, 0x164, interrupt_status);

			/* Get the results. */
			ncr_read32(smregion, 0x50, & result);

			if( result & ( 1 << test ) ) {
				printf( "PASSED.\n" );
			} else {
				printf( "FAILED.\n" );
				acp_sysmem_bist_failure( smregion );
			}

			/*
			  Make sure to disable before letting the
			  system access system memory.  Disabling
			  BIST_GO param.
			 */
			ncr_and( smregion, 0x8, 0xfffffffe );
		}
	}
#endif

	acp_sysmem_asic_check_ecc(NCP_REGION_ID(0x022, 0));

	if( 1 < sysmem->num_interfaces )
		acp_sysmem_asic_check_ecc(NCP_REGION_ID(0x00f, 0));

	return;
}

#endif

/*
  ==============================================================================
  ==============================================================================
  Public Interface
  ==============================================================================
  ==============================================================================
*/

/*
  ------------------------------------------------------------------------------
  acp_init
*/

int
acp_init( void )
{
	int returnCode = 0;

	/*
	  Try LCM first, to allow for board repair when the serial
	  EEPROM contains a valid but incorrect (unusable) parameter
	  table.
	*/

	/* Verify that the paramater table is valid. */
	if( PARAMETERS_MAGIC != header->magic ) {
		/* Initialize the SEEPROM (device 0, read only). */
		ssp_init(0, 1);

		/* Copy the last 1K of the SEEPROM to the last 1K of LCM. */
		returnCode = ssp_read((void *)PARAMETERS_ADDRESS,
				      PARAMETERS_OFFSET, PARAMETERS_SIZE);

		if (0 != returnCode ||
		    PARAMETERS_MAGIC != header->magic) {
			/*
			  Set the PCIe/SRIO mode based on the
			  reset_config value since no parameters are
			  available.
			*/
			pciesrio_init(1);

			/* No parameters available, fail. */
			returnCode = -1;
			goto acp_init_return;
		}
	}

#ifdef DISPLAY_PARAMETERS
	printf("-- -- Header\n"
	       "0x%08lx 0x%08lx 0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n"
	       "0x%08lx 0x%08lx\n",
	       header->magic,
	       header->size,
	       header->checksum,
	       header->version,
	       header->globalOffset, header->globalSize,
	       header->pciesrioOffset, header->pciesrioSize,
	       header->voltageOffset, header->voltageSize,
	       header->clocksOffset, header->clocksSize,
	       header->sysmemOffset, header->sysmemSize);
#endif

	global = (parameters_global_t *)
		(PARAMETERS_ADDRESS + header->globalOffset);
	pciesrio = (parameters_pciesrio_t *)
		(PARAMETERS_ADDRESS + header->pciesrioOffset);
	voltage = (parameters_voltage_t *)
		(PARAMETERS_ADDRESS + header->voltageOffset);
	clocks = (parameters_clocks_t *)
		(PARAMETERS_ADDRESS + header->clocksOffset);
	sysmem = (parameters_sysmem_t *)
		(PARAMETERS_ADDRESS + header->sysmemOffset);

	if (crc32(0, (void *)PARAMETERS_ADDRESS, (header->size - 12)) !=
	    header->checksum ) {
		/*
		  Set the PCIe/SRIO mode based on the
		  reset_config value since no parameters are
		  available.
		*/
		pciesrio_init( 1 );

		printf( "Parameter table is corrupt. 0x%08lx!=0x%08x\n",
			header->checksum,
			crc32(0, (void *)PARAMETERS_ADDRESS,
			      (header->size - 12)));
		acp_failure( __FILE__, __FUNCTION__, __LINE__ );
	}

#ifdef DISPLAY_PARAMETERS
	printf("version=%lu flags=0x%lx\n",
	       global->version, global->flags);
#else
	printf("Parameter Table Version %lu\n", global->version);
#endif

	if (0 != (global->flags & PARAMETERS_GLOBAL_DISABLE_RESET))
		reset_enabled = 0;

#ifdef NCP_SM_PHY_REG_RESTORE
	ncp_sm_phy_reg_restore = 
		(global->flags & PARAMETERS_GLOBAL_SM_PHY_REG_RESTORE) ? 1 : 0;
#else 
	ncp_sm_phy_reg_restore = 0;
#endif

#ifdef NCP_SM_PHY_REG_DUMP
	ncp_sm_phy_reg_dump = 
		(global->flags & PARAMETERS_GLOBAL_SM_PHY_REG_DUMP) ? 1 : 0;
#else
	ncp_sm_phy_reg_dump = 0;
#endif

	/*
	  =======
	  Voltage
	  =======
	*/

#ifndef CONFIG_AXXIA_25xx
	if( 0 ==
	    ( global->flags & PARAMETERS_GLOBAL_IGNORE_VOLTAGE ) ) {
		if( 0 != ( returnCode = voltage_init( ) ) ) {
			/*
			  Set the PCIe/SRIO mode based on the
			  reset_config value since no parameters are
			  available.
			*/
			pciesrio_init( 1 );

			goto acp_init_return;
		}
	}
#endif

	/*
	  ======
	  Clocks
	  ======
	*/

	if( 0 ==
	    ( global->flags & PARAMETERS_GLOBAL_IGNORE_CLOCKS ) ) {
#ifndef DISPLAY_PARAMETERS
	  serial_exit(); /* Turn off the UART while updating the PLLs. */
#endif

		if( 0 != ( returnCode = clocks_init( ) ) ) {
			/*
			  Set the PCIe/SRIO mode based on the
			  reset_config value since no parameters are
			  available.
			*/	
			pciesrio_init( 1 );

			goto acp_init_return;
		}

#ifdef CLOCK_LOCK_VERIFY_EARLY
		mdelay( 500 );
#endif
		serial_early_init( );
	}

#if defined(CONFIG_AXXIA_25xx) && !defined(ACP_EMU)
	axm2500_pll_check_lock();
#endif

#ifdef CLOCK_LOCK_VERIFY_EARLY
	printf("LSI Version: %s\n", get_lsi_version());
	printf("Starting Lock Loop (Ctrl-C to Exit).\n");

	for (;;) {
		if (tstc()) {
			int c = getc();

			if (3 == c)
				break;

			acp_clock_lock_verify(0, 0);
		} else {
			acp_clock_lock_verify(0x10, 0x20000);
		}
	}
#endif

	/*
	  =========
	  PCIe/SRIO
	  =========
	*/

	if( 0 ==
	    ( global->flags & PARAMETERS_GLOBAL_IGNORE_PCIESRIO ) ) {
		if( 0 != ( returnCode = pciesrio_init( pciesrio->control ) ) ) {
			goto acp_init_return;
		}
	}

	/*
	  =============
	  System Memory
	  =============
	*/

	ncr_tracer_enable( );

	if( 0 ==
	    ( global->flags & PARAMETERS_GLOBAL_IGNORE_SYSMEM ) ) {
		/*
		  Create a UTLB entry for system memory.
		*/

		__asm__ __volatile__ ( "tlbwe %1,%0,0\n"			\
				       "tlbwe %2,%0,1\n"			\
				       "tlbwe %3,%0,2\n"			\
				       "isync\n"
				       : :
					 "r" (0x80000000),
					 "r" (0x00000bf0),
					 "r" (0x00000000),
					 "r" (0x00030207));

		if( 0 != ( returnCode = sysmem_init( ) ) ) {
			goto acp_init_return;
		}

#if !defined(ACP_EMU)
		if( 0 != ( global->flags &
			   PARAMETERS_GLOBAL_RUN_SYSMEM_BIST ) ) {
			acp_sysmem_bist( );
			fill_sysmem(0ULL, (1ULL << sysmem_size),
				    (sysmem->num_interfaces * 4));
		}
#endif
	}

 acp_init_return:
	ncr_tracer_disable( );

	/* set A2P */
#ifdef ACP_X1V1
	dcr_write( 0x1209, 0x1101 );
#else
	dcr_write( 0x120d, 0x1101 );
#endif

	/* 
	 * Workaround for bug 34037 
	 * Set the external AXIS MPIC1 GCF register, and do a
	 * sacrifical DCR read to some other non-MPIC address
	 */
	{
		int tmp;

		dcr_write(0x2000000e, 0xefc01020);
		tmp = dcr_read(0x1101);
	}

#ifdef SM_REG_DUMP
	/*check_for_failure(0, __FILE__, __LINE__);*/
#endif /* SM_REG_DUMP */

	/*
	  Force all speculative collisions to be true collisions (L2)
	*/
#if !defined(ACP_ISS) && defined(CONFIG_AXXIA_344X)
	if ((0 != (global->flags & PARAMETERS_GLOBAL_SET_L2_FTC)) ||
	    (0 != (global->flags & PARAMETERS_GLOBAL_SET_L2_DISABLE_SL))) {
		unsigned long mask = 0;
		unsigned long value;

		if (0 != (global->flags & PARAMETERS_GLOBAL_SET_L2_FTC))
			mask |= 0x10;

		if (0 != (global->flags & PARAMETERS_GLOBAL_SET_L2_DISABLE_SL))
			mask |= 0x1;

		/* L2[0] */
		dcr_write(0x14, 0x300);
		value = dcr_read(0x304);
		value |= mask;
		dcr_write(value, 0x304);
		/* L2[1] */
		dcr_write(0x14, 0x400);
		value = dcr_read(0x404);
		value |= mask;
		dcr_write(value, 0x404);
		/* L2[2] */
		dcr_write(0x14, 0x500);
		value = dcr_read(0x504);
		value |= mask;
		dcr_write(value, 0x504);
		/* L2[3] */
		dcr_write(0x14, 0x600);
		value = dcr_read(0x604);
		value |= mask;
		dcr_write(value, 0x604);
	}
#endif

#ifndef CONFIG_SPL_BUILD
	pci_init_board();
#endif

#ifdef SM_REG_DUMP
	/*check_for_failure(0, __FILE__, __LINE__);*/
#endif /* SM_REG_DUMP */

	return returnCode;
}
