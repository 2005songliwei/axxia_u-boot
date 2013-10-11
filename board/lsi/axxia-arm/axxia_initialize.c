/*
 *  Copyright (C) 2013 LSI Corporation
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

#include "../common/ncp_sysmem_ext.h"
#include "../common/ncp_sysmem_lsiphy.h"

/*
  ===============================================================================
  ===============================================================================
  Local
  ===============================================================================
  ===============================================================================
*/

unsigned long sysmem_size = 1;
unsigned long reset_enabled = 1;
unsigned long ncp_sm_phy_reg_restore = 0;
unsigned long ncp_sm_phy_reg_dump = 0;
unsigned long *phyRegs;
                                                                                
#if defined (CONFIG_AXXIA_EMU)
#include "sysmem_emulation.c"
#else                                                                           
#include "../common/sysmem_asic_common.c"
#include "../common/ncp_sysmem_init_lsiphy.c"
#include "../common/ncp_elm.c"
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
  axxia_initialize
*/

int
axxia_initialize(void)
{
	int i;
    unsigned long value;

	if (0 != read_parameters())
		acp_failure(__FILE__, __FUNCTION__, __LINE__);


	/*
	  ============
	  Reset Reason
	  ============
	*/

#ifndef CONFIG_AXXIA_EMU
    /* read and clear reset status (write one to clear) */
    ncr_read32(NCP_REGION_ID(0x156, 0x00), 0x100, &value);
    printf("Reset Status = 0x%08x\n", value);
    ncr_write32(NCP_REGION_ID(0x156, 0x00), 0x100, value);

    /*
     * if this is a power-up/pin reset then initialize
     * persistent registers 
     */
    if ( (value & 0x00000001) ) {
        printf("PowerUp/Pin Reset detected - initializing persistent registers\n");
        for (i = 0; i < 9; i++) {
            ncr_write32(NCP_REGION_ID(0x156, 0x00), (0xdc + (4 * i)), 0);
        }
    }
#endif

	/*
	  =======
	  Voltage
	  =======
	*/

#ifndef CONFIG_AXXIA_EMU
	if (0 == (global->flags & PARAMETERS_GLOBAL_IGNORE_VOLTAGE))
		if (0 != voltage_init())
			acp_failure(__FILE__, __FUNCTION__, __LINE__);
#endif

	/*
	  =========
	  PCIe/SRIO
	  =========
	*/

#ifndef CONFIG_AXXIA_EMU
	if (0 == (global->flags & PARAMETERS_GLOBAL_IGNORE_PCIESRIO))
		if (0 != pciesrio_init(pciesrio->control))
			acp_failure(__FILE__, __FUNCTION__, __LINE__);
#endif

	/*
	  ======
	  Clocks
	  ======
	*/
#ifndef CONFIG_AXXIA_EMU
	if (0 == (global->flags & PARAMETERS_GLOBAL_IGNORE_CLOCKS))
		if (0 != clocks_init())
			acp_failure(__FILE__, __FUNCTION__, __LINE__);
#endif
		
	serial_initialize();
	serial_init();

	/*
	  =============
	  System Memory
	  =============
	*/

#ifdef SYSCACHE_ONLY_MODE
	ncr_l3tags();
#else
	if (0 == (global->flags & PARAMETERS_GLOBAL_IGNORE_SYSMEM)) {
		ncr_tracer_enable();

		if (0 != sysmem_init())
			acp_failure(__FILE__, __FUNCTION__, __LINE__);

		ncr_tracer_disable();
	}
#endif

	return 0;
}
