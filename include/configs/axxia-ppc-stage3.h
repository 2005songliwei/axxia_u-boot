/*
 * (C) Copyright 2009
 * LSI <www.lsi.com>
 * John Jacques <john.jacques@lsi.com>
 * Configuration settings for the LSI Axxia development boards.
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

#ifndef __CONFIGS_ACP3_H
#define __CONFIGS_ACP3_H

/*
  ==============================================================================
  Even though the hardware type should be defined in axxia.h, the
  U-Boot config.mk file only looks here (for the 3rd stage) or in
  axxia-ppc-stage2.h (for the 2nd stage).  So, the hardware type is
  defined here.
*/

#define CONFIG_ACP3 1
#define CONFIG_ACP   1		/* LSI ACP Board             */
#define CONFIG_476FP 1		/* Specific PPC476FP Support */
#define CONFIG_470   1		/* ... PPC47x family         */
#define CONFIG_4xx   1		/* ... PPC4xx family         */
#define CONFIG_BOOKE 1

#define CONFIG_PCI 1
#define CONFIG_PCI_PNP 1                          /* do pci plug-and-play*/
#define CONFIG_CMD_PCI 1
#define CONFIG_PCI_SCAN_SHOW 1

#define ACP_PEI0 1
#define ACP_PEI1 1

#ifdef ACP_25xx
#define CONFIG_SYS_PCIE_NR_PORTS 2
#else
#define ACP_PEI2 1
#define CONFIG_SYS_PCIE_NR_PORTS 3
#endif

/* Board-specific PCI */
#define CONFIG_SYS_PCI_TARGET_INIT                 /* let board init pci target    */

#if defined(ACP_PEI0) && defined(ACP_PEI1) && defined(ACP_PEI2)
#define CONFIG_SYS_PCIE0_MEMBASE 0x90000000
#define CONFIG_SYS_PCIE1_MEMBASE 0xb0000000
#define CONFIG_SYS_PCIE2_MEMBASE 0xd0000000
#define CONFIG_PCIE0_PHY_START 0x80000000
#define CONFIG_PCIE1_PHY_START 0xa0000000
#define CONFIG_PCIE2_PHY_START 0xc0000000
#elif defined(ACP_PEI0) && defined(ACP_PEI1)
#define CONFIG_SYS_PCIE0_MEMBASE 0x90000000
#define CONFIG_SYS_PCIE1_MEMBASE 0xb0000000
#define CONFIG_SYS_PCIE2_MEMBASE 0x0
#define CONFIG_PCIE0_PHY_START 0x80000000
#define CONFIG_PCIE1_PHY_START 0xa0000000
#define CONFIG_PCIE2_PHY_START 0x0
#elif defined(ACP_PEI0)
#define CONFIG_SYS_PCIE0_MEMBASE 0x90000000
#define CONFIG_SYS_PCIE1_MEMBASE 0x0
#define CONFIG_SYS_PCIE2_MEMBASE 0x0
#define CONFIG_PCIE0_PHY_START 0x80000000
#define CONFIG_PCIE1_PHY_START 0x0
#define CONFIG_PCIE2_PHY_START 0x0
#endif

#define CONFIG_SYS_PCIE_MEMSIZE 0x100000

#define CONFIG_PCIE0_BUS_START 0xA0000000
#define CONFIG_PCIE1_BUS_START 0xB0000000
#define CONFIG_PCIE2_BUS_START 0xC0000000

#define CONFIG_SYS_PCIE0_CFGADDR PCIE0_CONFIG
#define CONFIG_SYS_PCIE1_CFGADDR PCIE1_CONFIG
#define CONFIG_SYS_PCIE2_CFGADDR PCIE2_CONFIG

#ifdef ACP_25xx
#define CONFIG_SYS_PCIE_NR_PORTS 2
#else
#define CONFIG_SYS_PCIE_NR_PORTS 3
#endif

#if !defined(USE_HOSTCC)
/* USB support */
#define CONFIG_CMD_USB 1
#define CONFIG_USB_EHCI 1
#define CONFIG_USB_STORAGE 1
#define CONFIG_CMD_USB_STORAGE 1
#define CONFIG_USB_ULPI 1
#define CONFIG_CMD_FAT 1
#define CONFIG_CMD_EXT2 1
#define CONFIG_DOS_PARTITION 1
#define CONFIG_SYS_PPC4XX_USB_ADDR (IO+0xA0000)
#define CONFIG_EHCI_IS_TDI 1
#define CONFIG_LSI_USB 1
#endif

/*
  ----------------------------------------------------------------------
*/

#define CONFIG_SMP
#define CFG_PROMPT		"ACP3=> "
#define CONFIG_SYS_PROMPT	"ACP3=> "
#define CFG_CBSIZE		1024
#define CONFIG_SYS_CBSIZE	1024
#define CFG_PBSIZE		(CFG_CBSIZE+sizeof(CFG_PROMPT)+16)
#define CONFIG_SYS_PBSIZE	(CONFIG_SYS_CBSIZE+sizeof(CONFIG_SYS_PROMPT)+16)
#define CFG_MAXARGS		64
#define CONFIG_SYS_MAXARGS	64
#define CFG_BARGSIZE		CFG_CBSIZE
#define CONFIG_SYS_BARGSIZE	CONFIG_SYS_CBSIZE

/*
  ----------------------------------------------------------------------
  Memory Layout

  U-Boot won't get moved to the top of RAM in this case. There will be
  two RAM segments.  1 Mb at address 0 (U-Boot text) and 3 Mbyte at
  0x100000.

  Note that CFG_MALLOC_BASE is the top of the heap.
*/

/* Use the last 2M of the first 4M of system memory */
#define CFG_MALLOC_BASE     0x00400000
#ifdef ACP_ISS
#define CFG_MALLOC_LEN      ((2 * 1024 * 1024) - (64 * 1024))
#else
#define CFG_MALLOC_LEN      (2 * 1024 * 1024)
#endif
#define CFG_STACK_BASE      0x00200000
#define CFG_INIT_RAM_END    0x4000 /* This is really the size... */
#define CFG_INIT_RAM_ADDR0  (CFG_STACK_BASE - 0x0000)
#define CFG_INIT_RAM_ADDR1  (CFG_STACK_BASE - 0x4000)
#define CFG_INIT_RAM_ADDR2  (CFG_STACK_BASE - 0x8000)
#define CFG_INIT_RAM_ADDR3  (CFG_STACK_BASE - 0xc000)

/*
  ----------------------------------------------------------------------
  Commands
*/

/*
  ======================================================================
  ======================================================================
  NAND
  ======================================================================
  ======================================================================
*/

#define ACP_NAND_4BIT_ECC
#define CONFIG_JFFS2_NAND
#define CONFIG_JFFS2_CMDLINE
#define CONFIG_CMD_SETGETDCR
#define CONFIG_CMD_JFFS2
#define CONFIG_CMD_ELF

/*
  ======================================================================
  ======================================================================
  NAND
  ======================================================================
  ======================================================================
*/

#if !defined(CONFIG_SPD)
#define CONFIG_HARD_I2C         /* Hardware I2C */
#define CONFIG_CMD_I2C
#define CONFIG_SYS_I2C_MULTI_EEPROMS
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN 1
#define CONFIG_CMD_EEPROM
#if defined(ACP_25xx)
#define CONFIG_I2C_MULTI_BUS
#define CONFIG_SYS_MAX_I2C_BUS 2
#endif
#endif



/*
  ======================================================================
  ======================================================================
  Device Tree...
  ======================================================================
  ======================================================================
*/

#define CONFIG_OF_LIBFDT
#define CONFIG_FIT
#ifndef __ASSEMBLY__
unsigned long *get_acp_fdt(int);
#endif

/*
  ======================================================================
  ======================================================================
  SMP/AMP Stuff
  ======================================================================
  ======================================================================
*/

#define ACP_MAX_CORES 4
#if defined(ACP_X2V1) || defined(ACP_25xx)
#define ACP_NR_CORES 2
#define OS_GROUP0_DEFAULT "0xd31:0:0x100"
#else
#define ACP_NR_CORES 4
#define OS_GROUP0_DEFAULT "0xdf1:0:0x100"
#endif
#define OS_MEMORY_DEFAULT 0x100

#ifndef __ASSEMBLY__

typedef struct {
	volatile unsigned int lock;
} acp_spinlock_t;

#define ACP_DEFINE_SPINLOCK(lock) acp_spinlock_t lock = { 0 }

static inline unsigned long
__acp_spin_trylock(acp_spinlock_t *lock)
{
	unsigned long tmp;
	unsigned long token;

	token = 1;

	__asm__ __volatile__ ("1:  lwarx    %0,0,%2\n" \
			      "    cmpwi    0,%0,0\n"  \
			      "    bne-     2f\n"      \
			      "    stwcx.   %1,0,%2\n" \
                              "    bne-     1b\n"      \
			      "2:           \n"
			      : "=&r" (tmp)
			      : "r" (token), "r" (&lock->lock)
			      : "cr0", "memory");

	return tmp;
}

static inline void
acp_spin_lock(acp_spinlock_t *lock)
{
	if (0 == __acp_spin_trylock(lock))
		return;

	while (0 != __acp_spin_trylock(lock))
		;
}

static inline void
acp_spin_unlock(acp_spinlock_t *lock)
{
	__asm__ __volatile__ ("lwsync" : : : "memory");
	lock->lock = 0;
}

void acp_lock_stage3(void);
void acp_unlock_stage3(void);

typedef struct {
	unsigned long long entry_address;
	unsigned long long r3_value;
	unsigned long rsvd1_value;
	unsigned long pir_value;
	unsigned long ccr0_value;
	unsigned long ccr1_value;
	unsigned long ccr2_value;
} acp_spintable_t;

extern acp_spintable_t * acp_spintable [ ACP_NR_CORES ];

int acp_spintable_init(int, int, unsigned long);
void acp_spintable_spin( void );
void acp_spintable_jump( unsigned long, unsigned long );

typedef enum {
	ACP_OSG_SPINTABLE_NOTAVAIL,
	ACP_OSG_SPINTABLE_AVAILABLE
} acp_osg_spintable_state_t;

typedef enum {
	ACP_OSG_CORE_STATE_UNKNOWN,
	ACP_OSG_CORE_STATE_ASLEEP,
	ACP_OSG_CORE_STATE_WAITING_FOR_SPINTABLE,
	ACP_OSG_CORE_STATE_SPINNING,
	ACP_OSG_CORE_STATE_RUNNING
} acp_osg_core_state_t;

acp_osg_spintable_state_t acp_osg_get_spintable_state(int);
void acp_osg_set_spintable_state(int, acp_osg_spintable_state_t);

acp_osg_core_state_t acp_osg_get_core_state(int);
void acp_osg_set_core_state(int, acp_osg_core_state_t);

#define ACP_MAX_OS_GROUPS ACP_MAX_CORES

int acp_osg_readenv(void);

typedef enum {
	ACP_OS_BOOT_CORE, ACP_OS_CORES, ACP_OS_BASE, ACP_OS_SIZE,
	ACP_OS_UART0, ACP_OS_UART1, ACP_OS_NAND, ACP_OS_FEMAC,
	ACP_OS_PCIE0, ACP_OS_PCIE1, ACP_OS_PCIE2, ACP_OS_SRIO,
	ACP_OS_USB, ACP_OS_PLX, ACP_OS_SBB, ACP_OS_SSP,	ACP_OS_I2C,
	ACP_OS_FDT
} acp_osg_group_res_t;

unsigned long long acp_osg_group_get_res(int, acp_osg_group_res_t);
void acp_osg_group_set_res(int, acp_osg_group_res_t, unsigned long);
int acp_osg_get_group(int);
int acp_osg_is_group_boot_core(int);
int acp_osg_is_boot_core(int);
int acp_osg_set_os(int, void (*)(unsigned long, unsigned long, unsigned long,
				 unsigned long, unsigned long),
		   unsigned long, unsigned long, unsigned long, unsigned long,
		   unsigned long);
int acp_osg_jump_to_os(int);
int acp_osg_dump(int);
int acp_osg_get_current(void);
void acp_osg_set_current(int);
int acp_osg_initialize(void);
int acp_osg_map(int);

#endif

/*
  ======================================================================
  ======================================================================
  OSE
  ======================================================================
  ======================================================================
*/

#ifndef __ASSEMBLY__
extern unsigned long _ose_strings_0;
extern unsigned long _ose_strings_1;
extern unsigned long _ose_strings_2;
extern unsigned long _ose_strings_3;
#define OSE_STRINGS_SIZE 0x400
int ose_init( void );
int ose_add_string( int core, const char * string );
#endif /* __ASSEMBLY */

/*
  ======================================================================
  ======================================================================
  OS Access
  ======================================================================
  ======================================================================
*/

#ifndef __ASSEMBLY__
int os_access_init( void );
void * os_access_get_address( void );
#endif /* __ASSEMBLY */

/*
  ======================================================================
  ======================================================================
  Clocks
  ======================================================================
  ======================================================================
*/

#ifndef __ASSEMBLY__
int acp_clock_update_dt(void);
#endif /* __ASSEMBLY__ */

/*
  ======================================================================
  ======================================================================
  EIOA Ethernet
  ======================================================================
  ======================================================================
*/

#define CONFIG_LSI_EIOA

#ifndef __ASSEMBLY__
int eioa_ethernet_configure(void);
#endif /* __ASSEMBLY__ */

/*
  ======================================================================
  ======================================================================
  VxWorks
  ======================================================================
  ======================================================================
*/

#define CONFIG_SYS_VXWORKS_BOOT_ADDR 0x1000
#define CONFIG_SYS_VXWORKS_BOOT_DEVICE "lsiMac(0,0)"

/*
  ======================================================================
  ======================================================================
  Non-Volatile Storage...
  ======================================================================
  ======================================================================
*/

/*
  By default, the 3rd stage always supports NAND flash.  To turn off
  NAND flash support, comment out the following.
*/

#define CONFIG_LSI_NAND
#define ACP_NAND_4BIT_ECC

/*
  By default, the 3rd stage always supports serial flash.  To turn off
  NAND flash support, comment out the following.
*/

#define CONFIG_LSI_SERIAL_FLASH

/*
  By default, the U-Boot environment is stored in NAND for all targets
  except AXM2500.  To switch to serial flash, defined
  CONFIG_LIS_SERIAL_FLASH_ENV.
*/

#ifdef ACP_25xx
#define CONFIG_LSI_SERIAL_FLASH_ENV
#endif

#ifndef CONFIG_LSI_SERIAL_FLASH_ENV
#define CONFIG_LSI_NAND_ENV
#endif

/*
  ======================================================================
  ======================================================================
  Network
  ======================================================================
  ======================================================================
*/

#define CONFIG_LSI_NET

#if defined(ACP_ISS) || defined(NCR_TRACER) || defined(ACP2_SYSMEM_TEST)
#undef CONFIG_LSI_NET
#endif

#if defined(CONFIG_LSI_NET)
#define CONFIG_CMD_NET
#define CONFIG_CMD_DHCP
#define APP3XXNIC_RX_BASE  (IO+0x80000)
#define APP3XXNIC_TX_BASE  (IO+0x81000)
#define APP3XXNIC_DMA_BASE (IO+0x82000)
#endif

/*
  ======================================================================
  ======================================================================
  Task I/O Lite
  ======================================================================
  ======================================================================
*/

#define NCP_SYSMEM_BARRIER()  __asm__ __volatile__(".long 0x7c0004ac")
#define NULL ((void *)0)
#define FALSE 0
#define TRUE (!FALSE)
#define NCP_TASKIO_UBOOT_ENV

/*
  ======================================================================
  ======================================================================
  Errata
  ======================================================================
  ======================================================================
*/

#if 0
#define CONFIG_SATA_SIL3114
#define CONFIG_SYS_SATA_MAX_DEVICE 1
#define CONFIG_CMD_SATA
#define CONFIG_DOS_PARTITION
#define CONFIG_CMD_FAT
#define CONFIG_SUPPORT_VFAT
#define CONFIG_CMD_EXT2
#define CONFIG_LIBATA
#endif

#define CONFIG_LSI_OSG 1
#define CONFIG_MTD_DEVICE 1
#define CONFIG_CMD_MTDPARTS 1
#define CONFIG_LSI_TEST 1
#define CONFIG_CMD_DBG 1

#define CFG_LONGHELP
#define CONFIG_SYS_LONGHELP
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_MTEST
#define CFG_MEMTEST_START 0x4000000
#define CONFIG_SYS_MEMTEST_START 0x4000000
#define CFG_MEMTEST_END   0x5000000
#define CONFIG_SYS_MEMTEST_END   0x5000000
#define CONFIG_SYS_ALT_MEMTEST

#define CONFIG_CMD_BDI
#define CONFIG_CMD_LOADB
#define CONFIG_SYS_BOOTM_LEN 0x4000000

#define CONFIG_EXTRA_ENV_SETTINGS \
"bootargs3=root=/dev/issblk3 rw lpj=15990784\0" \
"bootcmd3=bootm c00000\0" \
"osgroup0=0xf1:0x100:0x100\0"

#ifndef __ASSEMBLY__
extern unsigned long _core_copy_beginning;
extern unsigned long _core_copy_end;
extern unsigned long _core_copy_core0;
extern unsigned long _core_copy_core1;
extern unsigned long _core_copy_core2;
extern unsigned long _core_copy_core3;

extern unsigned long __acp_osg_cores;
extern unsigned long __acp_osg_groups;
extern unsigned long __acp_system_section;
#endif

#define CONFIG_CMD_SOURCE 1

/*
  ----------------------------------------------------------------------
  Include the ACP common header.
*/

#include <configs/axxia-ppc.h>

#endif	/* __CONFIGS_ACP3_H */
