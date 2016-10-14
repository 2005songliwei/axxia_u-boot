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
#include <serial.h>
#include "ncp_sysmem_ext.h"

/*
==============================================================================
==============================================================================
Local
==============================================================================
==============================================================================
*/

unsigned *phyRegs;

/*
==============================================================================
==============================================================================
Public Interface
==============================================================================
==============================================================================
*/
ncp_st_t
ncp_elm_init(ncp_dev_hdl_t, ncp_sm_parms_t *);

void dickens_init(void);

/*
------------------------------------------------------------------------------
axxia_initialize
*/

int
axxia_initialize(void)
{
	int rc;

	/*
	  ===============
	  Read Parameters
	  ===============
	*/

	rc = read_parameters();

#if !defined(CONFIG_AXXIA_SIM)
	if (0 != rc)
		acp_failure(__FILE__, __FUNCTION__, __LINE__);
#else
	/*
	  For now, in simulation, don't require a parameter file.  If
	  there is no parameter file, simply return success.
	*/

	if (0 != rc)
		return 0;
#endif

	/*
	  =======
	  Voltage
	  =======
	*/

#if !defined(CONFIG_TARGET_EMULATION)
	if (0 != (global->flags & PARAMETERS_GLOBAL_SET_VOLTAGE))
		if (0 != voltage_init())
			acp_failure(__FILE__, __FUNCTION__, __LINE__);
#endif

	/*
	  ======
	  Clocks
	  ======
	*/

#if !defined(CONFIG_TARGET_EMULATION)
	if (0 != (global->flags & PARAMETERS_GLOBAL_SET_CLOCKS))
		if (0 != clocks_init(sysmem->ddrRecovery))
			acp_failure(__FILE__, __FUNCTION__, __LINE__);
#endif

	serial_initialize();
	serial_init();
	(void)sysmem_size();

	/*
	  =============
	  System Memory
	  =============
	*/

#if !defined(SYSCACHE_ONLY_MODE) && !defined(CONFIG_TARGET_EMULATION)
	if (0 != (global->flags & PARAMETERS_GLOBAL_SET_SMEM)) {
		if (0 == sysmem->ddrRecovery) {
			ncr_tracer_enable();

			if (0 != sysmem_init())
				acp_failure(__FILE__, __FUNCTION__, __LINE__);

			ncr_tracer_disable();
		} else {
			ncp_uint32_t version_save;

			printf("initializing ELMs for ddrRecovery\n");

			/* Initialize Dickens and the ELMs */
			version_save = sysmem->version;
#if defined(CONFIG_AXXIA_56XX) || defined(CONFIG_AXXIA_56XX_SIM)
			sysmem->version = NCP_CHIP_ACP56xx;
#elif defined(CONFIG_AXXIA_XLF) || defined(CONFIG_AXXIA_XLF_SIM)
			sysmem->version = NCP_CHIP_ACPXLF;
#else
#error "Invalid Chip Type!"
#endif

            dickens_init();

			rc = ncp_elm_init(NULL, sysmem);

			sysmem->version = version_save;

			if (NCP_ST_SUCCESS != rc) {
				printf("Initializing ELMs Failed!\n");

				return -1;
			}
		}
	}
#endif

	/*
	  ===========
	  Tree Memory
	  ===========
	*/

#if !defined(CONFIG_AXXIA_EMU)
	if ((0 != (global->flags & PARAMETERS_GLOBAL_SET_CMEM))) {
		ncr_tracer_enable();

		if (0 != cmem_init())
			acp_failure(__FILE__, __FUNCTION__, __LINE__);

		ncr_tracer_disable();
	}
#endif

	/*
	  =========
	  PCIe/SRIO
	  =========
	*/

#if defined(CONFIG_AXXIA_PCI) && !defined(PEI_SETUP_IN_LINUX)
	if (0 != (global->flags & PARAMETERS_GLOBAL_SET_PEI))
		if (0 != pciesrio_init(pciesrio->control))
			acp_failure(__FILE__, __FUNCTION__, __LINE__);
#endif

	return 0;
}
