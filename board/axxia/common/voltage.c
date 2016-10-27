/*
 *  Copyright (C) 2016 Intel Corporation
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

#include <config.h>
#include <common.h>
#include <asm/io.h>

/*
  ==============================================================================
  ==============================================================================
  Public Interface
  ==============================================================================
  ==============================================================================
*/

/*
  ------------------------------------------------------------------------------
  calc_chip_vrun

  Returns Vrun in mV.
*/

unsigned int
calc_chip_vrun(void)
{
#if defined(CONFIG_AXXIA_ANY_XLF)
	return V_SAFE;
#else
	unsigned int bin;
	unsigned int voltage = V_SAFE;

	/* Get the bin; slow, medium, or fast. */
	ncr_read32(NCP_REGION_ID(0x155, 0xb), 0x15c, &bin);
	bin = ((bin >> 16) & 0xff);

	switch (bin) {
	case 1:
		/* slow bin */
		ncr_read32(NCP_REGION_ID(0x155, 0xb), 0x148, &voltage);
		voltage = ((voltage >> 16) & 0xff) + 700;

		if (V_MIN_1 > voltage)
			voltage = V_SAFE;

		break;
	case 2:
		/* fast bin */
		ncr_read32(NCP_REGION_ID(0x155, 0xb), 0x148, &voltage);
		voltage = ((voltage >> 24) & 0xff) + 700;

		if (V_MIN_2 > voltage)
			voltage = V_SAFE;

		break;
	default:
		break;
	}

	if (V_MAX < voltage)
		voltage = V_SAFE;

	return voltage;
#endif
}

/*
  ------------------------------------------------------------------------------
  set_vrm_to_vrun

  The Intel implementation.  May be replaced by a board specific function.
*/

int __weak
set_vrm_to_vrun(unsigned int v_run)
{
	printf("To set the voltage to %u mV, implement set_vrm_to_vrun().\n",
	       v_run);

	return 0;
}

/*
  ------------------------------------------------------------------------------
  voltage_init
*/

int
voltage_init(void)
{
	if (0 != set_vrm_to_vrun(calc_chip_vrun()))
		return -1;

	return 0;
}
