/*
 * (C) Copyright 2013
 * LSI <www.lsi.com>
 * John Jacques <john.jacques@lsi.com>
 * Configuration settings for the LSI ACP development boards.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIGS_AXXIA_55XX_EMU_H
#define __CONFIGS_AXXIA_55XX_EMU_H

#define CONFIG_AXXIA_55XX
#define CONFIG_AXXIA_EMU

/*
  ===============================================================================
  ===============================================================================
  Constants
  ===============================================================================
  ===============================================================================
*/

#define DEFAULT_SDCR_VALUE 0x00080000
#define CONFIG_BAUDRATE    9600


/********** FEMAC PHY ADDRESS *************/
#define CONFIG_AXXIA_PHY_ADDRESS 0x3


/* HACK: Define it until cache coherency is figured out */
/* #define USE_CACHE_SYNC */

/*
  ================================================================================
  ================================================================================
  Non-Volatile Storage
  ================================================================================
  ================================================================================
*/

#define CONFIG_AXXIA_SERIAL_FLASH /* Include support for SPI flash. */

#define CONFIG_AXXIA_SERIAL_FLASH_ENV
#define CONFIG_ENV_IS_IN_SERIAL_FLASH

#define CONFIG_PARAMETER_OFFSET          (256 * 1024)
#define CONFIG_PARAMETER_SIZE            (64 * 1024)
#define CONFIG_PARAMETER_RANGE           (64 * 1024)
#define CONFIG_PARAMETER_OFFSET_REDUND \
	(CONFIG_PARAMETER_OFFSET + CONFIG_PARAMETER_RANGE)
#define CONFIG_PARAMETER_SIZE_REDUND     CONFIG_PARAMETER_SIZE
#define CONFIG_PARAMETER_RANGE_REDUND    CONFIG_PARAMETER_RANGE

#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_OFFSET \
	(CONFIG_PARAMETER_OFFSET_REDUND + CONFIG_PARAMETER_RANGE_REDUND)
#define CONFIG_ENV_SIZE                  (64 * 1024)
#define CONFIG_ENV_RANGE                 (64 * 1024)
#define CONFIG_ENV_OFFSET_REDUND         (CONFIG_ENV_OFFSET + CONFIG_ENV_RANGE)
#define CONFIG_ENV_SIZE_REDUND            CONFIG_ENV_SIZE
#define CONFIG_ENV_RANGE_REDUND           CONFIG_ENV_RANGE

/*
  ===============================================================================
  ===============================================================================
  Features.
  ===============================================================================
  ===============================================================================
*/

#define V_MIN   795
#define V_SAFE  940
#define V_MAX  1050

#define CONFIG_MEMORY_RETENTION

#include <configs/axxia-arm.h>

#endif	/* __CONFIGS_AXXIA_55XX_EMU_H */
