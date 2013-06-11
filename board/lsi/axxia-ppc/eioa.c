/*
 *  Copyright (C) 2011 LSI Corporation
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

/*
  Notes:

  - Virtual registers (0.769.0.OFFSET) are described in
    rte/api/nca/include/ncp_dev_config_regs.h.
*/

/*#define LSI_DEBUG*/
#define LSI_WARN
/*#define LSI_LOGIO*/
#include <config.h>
#include <common.h>
#include <malloc.h>

/*#define TRACE_ALLOCATION*/

#define NCP_TASKIO_LITE
#define NCP_TASKIO_UBOOT_ENV
#define NCP_TASKIO_LITE_UBOOT_ENV_DEFINE_GLOBALS

#include "ncp_task_lite_nca_regs.h"
#include "ncp_task_lite_nca_reg_defines.h"
#include "ncp_task_lite_basetypes.h"
#include "ncp_task_lite.h"

DECLARE_GLOBAL_DATA_PTR;

/*
  ==============================================================================
  ==============================================================================
  Private Interface
  ==============================================================================
  ==============================================================================
*/

static int initialized = 0;
static ncp_task_lite_hdl_t taskLiteHdl = NULL;
static ncp_task_lite_rxQ_hdl_t taskLiteRxHdl = NULL;
static int queueSetId = 0;
static int ncaQueueId = 0;
static int eioaPort = 0;
static int loopback = 0;
static int rxtest = 0;

extern int dumprx;
extern int dumptx;

static int port_by_index[] = {1, 2, 3, 4, 9, 10, 11, 12};
#ifdef ACP_25xx
static int phy_by_index[] = {0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10};
#else
static int phy_by_index[] = {0x10, 0x11, 0x12, 0x13, 0x18, 0x19, 0x1a, 0x1b};
#endif
static int index_by_port[] = {-1, 0, 1, 2, 3, -1, -1, -1, -1, 4, 5, 6, 7};

#define NUMBER_OF_RX_BUFFERS 15
static void *rx_buffers[NUMBER_OF_RX_BUFFERS];

#undef TRACE
#define TRACE
#ifdef TRACE
#define NCR_CALL(x) { \
if (0 != (rc = (x))) { \
printf("%s:%s:%d - FAILED\n", __FILE__, __FUNCTION__, __LINE__); \
goto ncr_exit; \
} \
}
#else
#define NCR_CALL(x) { \
if (0 != (rc = (x))) { \
goto ncr_exit; \
} \
}
#endif

typedef struct {
	unsigned long region;
	unsigned long offset;
} ncr_location_t;

typedef enum {
	NCR_COMMAND_NULL,
	NCR_COMMAND_WRITE,
	NCR_COMMAND_READ,
	NCR_COMMAND_MODIFY,
	NCR_COMMAND_USLEEP,
	NCR_COMMAND_POLL
} ncr_command_code_t;

typedef struct {
	ncr_command_code_t command;
	unsigned long region;
	unsigned long offset;
	unsigned long value;
	unsigned long mask;
} ncr_command_t;

#if defined(ACP_X1V2)
#include "EIOA344x/mmb.c"
#include "EIOA344x/vp.c"
#include "EIOA344x/nca.c"
#include "EIOA344x/eioa.c"
#elif defined(ACP_X2V1)
#include "EIOA342x/mmb.c"
#include "EIOA342x/vp.c"
#include "EIOA342x/nca.c"
#include "EIOA342x/eioa.c"
#elif defined(ACP_25xx)
#include "EIOA25xx/mmb.c"
#include "EIOA25xx/vp.c"
#include "EIOA25xx/nca.c"
#include "EIOA25xx/timer.c"
#include "EIOA25xx/tdmioa.c"
#include "EIOA25xx/eioa.c"
#elif defined(AXM_35xx)
#include "EIOA35xx/mmb.c"
#include "EIOA35xx/vp.c"
#include "EIOA35xx/nca.c"
#include "EIOA35xx/eioa.c"
#else
#error "EIOA is not defined for this architecture!"
#endif

#ifdef ACP_25xx
/*
  ------------------------------------------------------------------------------
  ncp_1d1_read
*/

static int
ncp_1d1_read(ncr_command_t *command, unsigned long *data)
{
	unsigned long target;
	unsigned long base;
	unsigned long timeout = 0x100000;
	unsigned long value;

	target = NCP_TARGET_ID(command->region);
	base = IO + 0x3000 + (target * 0x10);

	WRITEL(value, base);

	if (5 == target || 7 == target || 9 == target) {
		/* 32 bit */
		WRITEL((0x81400000 + command->offset), (base + 4));
	} else if (4 == target || 6 == target || 8 == target) {
		/* 16 bit */
		WRITEL((0x80c00000 + command->offset), (base + 4));
	}

	do {
		--timeout;
		value = READL(base + 4);
	} while (0 != (value & 0x80000000) && 0 < timeout);

	if (0 == timeout) {
		printf("ncp_1d1_read() timed out!\n");
		return -1;
	}

	value = READL(base + 0xc);

	if (0 != value) {
		printf("ncp_1d1_read() error!\n");
		return -1;
	}

	*data = READL(base + 0x8);

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_1d1_write
*/

static int
ncp_1d1_write(ncr_command_t *command)
{
	unsigned long target;
	unsigned long base;
	unsigned long timeout = 0x100000;
	unsigned long value;

	target = NCP_TARGET_ID(command->region);
	base = IO + 0x3000 + (target * 0x10);

	WRITEL(command->value, base);

	if (5 == target || 7 == target || 9 == target) {
		/* 32 bit */
		WRITEL((0xc1400000 + command->offset), (base + 4));
	} else if (4 == target || 6 == target || 8 == target) {
		/* 16 bit */
		WRITEL((0xc0c00000 + command->offset), (base + 4));
	}

	do {
		--timeout;
		value = READL(base + 4);
	} while (0 != (value & 0x80000000) && 0 < timeout);

	if (0 == timeout) {
		printf("ncp_1d1_write() timed out!\n");
		return -1;
	}

	value = READL(base + 0xc);

	if (0 != value) {
		printf("ncp_1d1_write() error!\n");
		return -1;
	}

	return 0;
}

#endif

/*
  ------------------------------------------------------------------------------
  ncp_dev_reset
*/

static int
ncp_dev_reset(void)
{
	unsigned long value;

	DEBUG_PRINT("\n");

	/*
	  Reset Modules
	*/

#if defined(ACP_X1V2)
	dcr_write(0xcc700000, (DCR_RESET_BASE + 2));
	udelay(10000);
	dcr_write(0, (DCR_RESET_BASE + 2));
	udelay(10000);
#elif defined(ACP_X2V1)
	dcr_write(0xcc500000, (DCR_RESET_BASE + 2));
	udelay(10000);
	dcr_write(0, (DCR_RESET_BASE + 2));
	udelay(10000);
#elif defined(ACP_25xx)
	/*
	  Enable protected writes.
	*/

	value = dcr_read(0xd00);
	value |= 0xab;
	dcr_write(value, 0xd00);

	/*
	  asp_rst, tmgr_nca_rst, tmc_rst, eioa_io_rst, xfi_phy_rst,
	  eioa_phy1_rst, eioa_phy0_rst, tm_clksync_rst, tm_phy_io_rst,
	  enet_ser_rst, nrcp_rst
	*/

	dcr_write(0x0ac01c1b, 0x1703);
	udelay(10000);
	dcr_write(0, 0x1703);
	udelay(10000);

	/*
	  Disable protected writes.
	*/

	value = dcr_read(0xd00);
	value &= ~0xab;
	dcr_write(value, 0xd00);
#endif

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_do_read
*/

static int
ncp_dev_do_read(ncr_command_t *command, unsigned long *value)
{
	if (NCP_REGION_ID(512, 1) == command->region) {
		*value = readl(command->offset);

		return 0;
#ifdef ACP_25xx
	} else if(0x1d1 == NCP_NODE_ID(command->region)) {
		return ncp_1d1_read(command, value);
#endif
	}

	if (0x100 <= NCP_NODE_ID(command->region)) {
		static int last_node = -1;

		if (-1 == last_node ||
		    NCP_NODE_ID(command->region) != last_node) {
			last_node = NCP_NODE_ID(command->region);
			DEBUG_PRINT("READ IGNORED: n=0x%lx t=0x%lx o=0x%lx\n",
				    NCP_NODE_ID(command->region),
				    NCP_TARGET_ID(command->region),
				    command->offset);
		}

		return 0;
	}

	if (0 != ncr_read32(command->region, command->offset, value)) {
		ERROR_PRINT("READ ERROR: n=0x%lx t=0x%lx o=0x%lx\n",
			    NCP_NODE_ID(command->region),
			    NCP_TARGET_ID(command->region), command->offset);
		return -1;
	}

	DEBUG_PRINT("Read 0x%08lx from n=0x%lx t=0x%lx o=0x%lx\n",
		    *value, NCP_NODE_ID(command->region),
		    NCP_TARGET_ID(command->region),
		    command->offset);

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_do_modify
*/

static int
ncp_dev_do_modify(ncr_command_t *command)
{
	if (0x100 <= NCP_NODE_ID(command->region)) {
		static int last_node = -1;

		if (-1 == last_node ||
		    NCP_NODE_ID(command->region) != last_node) {
			last_node = NCP_NODE_ID(command->region);
			DEBUG_PRINT("MODIFY IGNORED: n=0x%lx t=0x%lx o=0x%lx\n",
				    NCP_NODE_ID(command->region),
				    NCP_TARGET_ID(command->region),
				    command->offset);
		}

		return 0;
	}

	if (0 != ncr_modify32(command->region, command->offset,
			      command->mask, command->value)) {
		ERROR_PRINT("MODIFY ERROR: n=0x%lx t=0x%lx o=0x%lx m=0x%lx "
			    "v=0x%lx\n",
			    NCP_NODE_ID(command->region),
			    NCP_TARGET_ID(command->region), command->offset,
			    command->mask, command->value);

		return -1;
	} else {
		DEBUG_PRINT("MODIFY: r=0x%lx o=0x%lx m=0x%lx v=0x%lx\n",
			    command->region, command->offset,
			    command->mask, command->value);
	}

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_do_write
*/

static int
ncp_dev_do_write(ncr_command_t *command)
{
	DEBUG_PRINT(" WRITE: r=0x%lx o=0x%lx v=0x%lx\n",
		    command->region, command->offset, command->value);

	if (NCP_REGION_ID(0x200, 1) == command->region) {
		out_be32((volatile unsigned *)command->offset, command->value);
		flush_cache(command->offset, 4);
#ifndef ACP_25xx
	} else if (NCP_REGION_ID(0x148, 0) == command->region) {
		out_le32((volatile unsigned *)(APB2RC + command->offset),
			 command->value);
		flush_cache((APB2RC + command->offset), 4);
#else
	} else if (0x1d1 == NCP_NODE_ID(command->region)) {
		return ncp_1d1_write(command);
#endif
	} else if (0x100 > NCP_NODE_ID(command->region)) {
		if (NCP_REGION_ID(0x17, 0x11) == command->region &&
		    0x11c == command->offset) {
			return 0;
		}

		if (0 != ncr_write32(command->region, command->offset,
				     command->value)) {
			ERROR_PRINT("WRITE ERROR: n=0x%lx t=0x%lx o=0x%lx "
				    "v=0x%lx\n",
				    NCP_NODE_ID(command->region),
				    NCP_TARGET_ID(command->region),
				    command->offset, command->value);

			return -1;
		}
	} else {
		static int last_node = -1;

		if (-1 == last_node ||
		    NCP_NODE_ID(command->region) != last_node) {
			last_node = NCP_NODE_ID(command->region);
			DEBUG_PRINT("WRITE IGNORED: n=0x%lx t=0x%lx o=0x%lx "
				    "v=0x%lx\n",
				    NCP_NODE_ID(command->region),
				    NCP_TARGET_ID(command->region),
				    command->offset,
				    command->value);
		}

		return 0;
	}

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_do_poll
*/

static int
ncp_dev_do_poll(ncr_command_t *command)
{
	int timeout = 1000;
	int delay = 1000;
	unsigned long value;

	do {
		udelay(delay);

		if (0 != ncp_dev_do_read(command, &value)) {
			printf("ncp_dev_do_read() failed!\n");
			return -1;
		}
	} while (((value & command->mask) != command->value) &&
		 0 < --timeout);

	if (0 == timeout) {
		printf("ncp_dev_do_poll() timed out!\n");
		return -1;
	}

	return 0;
}

/*
  ------------------------------------------------------------------------------
  ncp_dev_configure
*/

static int
ncp_dev_configure(ncr_command_t *commands) {
	int rc = 0;
	unsigned long value;

	while (NCR_COMMAND_NULL != commands->command) {
		switch (commands->command) {
		case NCR_COMMAND_WRITE:
			rc = ncp_dev_do_write(commands);
			break;
		case NCR_COMMAND_READ:
			rc = ncp_dev_do_read(commands, &value);
			break;
		case NCR_COMMAND_MODIFY:
			rc = ncp_dev_do_modify(commands);
			break;
		case NCR_COMMAND_USLEEP:
			DEBUG_PRINT("USLEEP: v=0x%lx\n", commands->value);
			udelay(commands->value);
			break;
		case NCR_COMMAND_POLL:
			rc = ncp_dev_do_poll(commands);
			break;
		default:
			ERROR_PRINT("Unknown Command: 0x%x\n",
				    commands->command);
			rc = -1;
			break;
		}

		if (ctrlc()) {
			printf("Canceling configuration.\n");
			break;
		}

		++commands;
	}

	return rc;
}

/*
  ------------------------------------------------------------------------------
  alloc_128B_aligned
*/

static void *
alloc_128B_aligned(int size)
{
	void *free_address;
	void *aligned_address;

	if (NULL == (free_address = malloc(size + 128 + sizeof(void *)))) {
		ERROR_PRINT("Failed to allocate %d bytes!\n", size);
		return NULL;
	}

#ifdef TRACE_ALLOCATION
	printf("128B_ALLOC: 0x%08x bytes at 0x%p\n",
	       (size + 128 + sizeof(void *)), free_address);
#endif

	aligned_address =
		(void *)((unsigned long)free_address + 128UL + sizeof(void *));
	aligned_address =
		(void *)((unsigned long)aligned_address -
			 ((unsigned long)aligned_address % 128UL));
	*((unsigned long *)((unsigned long)aligned_address - sizeof(void *)))
		= (unsigned long)free_address;
	flush_cache((unsigned long)free_address, (size + 128 + sizeof(void *)));

	return aligned_address;
}

/*
  ------------------------------------------------------------------------------
  free_128B_aligned
*/

static void
free_128B_aligned(void *address)
{
	void *free_address;

	free_address =
		(void *)(*((unsigned long *)
			   ((unsigned long)address - sizeof(void *))));
	free(free_address);

#ifdef TRACE_ALLOCATION
	printf(" 128B_FREE: at 0x%p\n", free_address);
#endif

	return;
}

/*
  ------------------------------------------------------------------------------
  client_free
*/

static void
client_free(void *argument, void *buffer)
{
	free_128B_aligned(argument);

	return;
}

/*
  ------------------------------------------------------------------------------
  alloc_nvm
*/

static void *
alloc_nvm(int size)
{
#ifdef TRACE_ALLOCATION
	void *address;

	address = malloc(size);
	printf(" NVM_ALLOC: 0x%08x bytes at 0x%p\n", size, address);

	return address;
#else
	return malloc(size);
#endif
}

/*
  ------------------------------------------------------------------------------
  free_nvm
*/

static void
free_nvm(void *address)
{
	free(address);
#ifdef TRACE_ALLOCATION
	printf("  NVM_FREE: at 0x%p\n", address);
#endif
}

/*
  ------------------------------------------------------------------------------
  va2pa
*/

static ncp_uint32_t
va2pa(void *virtual_address)
{
	return (ncp_uint32_t)virtual_address;
}

/*
  ------------------------------------------------------------------------------
  task_lite_send
*/

static int
task_lite_send(ncp_task_lite_send_meta_t * taskLiteMetaData)
{
	ncp_st_t ncpStatus;

	do {
		flush_cache((unsigned long)taskLiteMetaData->taskPduData,
			    taskLiteMetaData->dataSize);
		ncpStatus =
			ncp_task_lite_send(taskLiteHdl, taskLiteMetaData, TRUE);

		if (NCP_ST_SUCCESS == ncpStatus)
			ncp_task_lite_free_send_buffers(taskLiteHdl);
	} while (NCP_ST_TASK_SEND_QUEUE_FULL == ncpStatus);

	if (NCP_ST_SUCCESS != ncpStatus)
		return 0;

	return taskLiteMetaData->dataSize;
}

/*
  ------------------------------------------------------------------------------
  tlb_entries
*/

static void
tlb_entries(int add)
{
	unsigned long offset;
	unsigned long size;
	unsigned long word0;
	unsigned long word1;
	unsigned long word2;

	/*
	  Be default, only the first 256M of system memory is mapped.
	  Add TLB entries for the next 3 256M chunks (first 1G).
	*/

	offset = 0x10000000UL;
	size = 0x30000000UL;

	while (0 < size) {
		if (add) {
			word0 = offset | 0x9f0UL;
			word1 = offset;
			word2 = 0x00030207UL;
		} else {
			word0 = offset;
			word1 = 0;
			word2 = 0;
		}

		__asm__ __volatile__ ( "tlbwe %1,%0,0\n"               \
				       "tlbwe %2,%0,1\n"               \
				       "tlbwe %3,%0,2\n"               \
				       "isync\n" : :
				       "r" (0),
				       "r" (word0),
				       "r" (word1),
				       "r" (word2) :
				       "memory" );

		offset += 0x10000000UL;
		size -= 0x10000000UL;
	}


	

	return;
}

/*
  ------------------------------------------------------------------------------
  line_renegotiate
*/

#define DELAY() udelay(5000)

static int
line_renegotiate(int index)
{
	int rc;
	unsigned long eioaRegion;
	unsigned long gmacRegion;
	unsigned long gmacPortOffset;
	char *envstring;
	unsigned short ad_value;
	unsigned short ge_ad_value;
	unsigned short control;

	/* Set the region and offset. */
	if (4 > index) {
		eioaRegion = NCP_REGION_ID(31, 16); /* 0x1f.0x10 */
		gmacRegion = NCP_REGION_ID(31, 18); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * index;
	} else {
		eioaRegion = NCP_REGION_ID(23, 16); /* 0x17.0x10 */
		gmacRegion = NCP_REGION_ID(23, 18); /* 0x17.0x12 */
		gmacPortOffset = 0xc0 * (index - 4);
	}

	/* Disable stuff. */
	NCR_CALL(ncr_modify32(gmacRegion, 0x330 + gmacPortOffset, 0x3f, 0));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x8, 0x0));
	NCR_CALL(ncr_modify32(eioaRegion, 0x70, 0x11000000, 0x0));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x4, 0x0));

	/* Check for "macspeed".  If set, ignore the PHYs... */
	envstring = getenv("macspeed");

	if (NULL == envstring) {
		/* Get ad_value and ge_ad_value from the environment. */
		envstring = getenv("ad_value");

		if (NULL == envstring) {
			ad_value = 0x1e1;
		} else {
			ad_value = simple_strtoul(envstring, NULL, 0);
		}

		envstring = getenv("ge_ad_value");

		if (NULL == envstring) {
			ge_ad_value = 0x300;
		} else {
			ge_ad_value = simple_strtoul(envstring, NULL, 0);
		}

		/* Set the AN advertise values. */
		mdio_write(phy_by_index[index], 4, ad_value);
		mdio_write(phy_by_index[index], 9, ge_ad_value);

		/* Force re-negotiation. */
		control = mdio_read(phy_by_index[index], 0);
		control |= 0x200;
		mdio_write(phy_by_index[index], 0, control);
	}

	return 0;

 ncr_exit:

	return -1;
}

/*
  ------------------------------------------------------------------------------
  line_setup
*/

static int
line_setup(int index)
{
	int rc;
	int retries = 100000;
	unsigned long eioaRegion;
	unsigned long gmacRegion;
	unsigned long gmacPortOffset;
	unsigned long eioaPortOffset;
	unsigned long ncr_status;
	char *envstring;
	unsigned short status;
	unsigned long top;
	unsigned long bottom;
	unsigned short ad_value;
	unsigned short ge_ad_value;
	unsigned short control;

	/* Set the region and offset. */
	if (4 > index) {
		eioaRegion = NCP_REGION_ID(31, 16); /* 0x1f.0x10 */
		eioaPortOffset = 8 * index;
		gmacRegion = NCP_REGION_ID(31, 18); /* 0x1f.0x12 */
		gmacPortOffset = 0xc0 * index;
	} else {
		eioaRegion = NCP_REGION_ID(23, 16); /* 0x17.0x10 */
		eioaPortOffset = 8 * (index - 4);
		gmacRegion = NCP_REGION_ID(23, 18); /* 0x17.0x12 */
		gmacPortOffset = 0xc0 * (index - 4);
	}

	/* Disable stuff. */
	NCR_CALL(ncr_modify32(gmacRegion, 0x330 + gmacPortOffset, 0x3f, 0));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x8, 0x0));
	NCR_CALL(ncr_modify32(eioaRegion, 0x70, 0x11000000, 0x0));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x4, 0x0));

	/* Check for "macspeed".  If set, ignore the PHYs... */
	envstring = getenv("macspeed");

	if (NULL != envstring) {
		printf("Overriding MAC Speed Settings; Ignoring PHY(s)!"
		       " %s\n", envstring);

		NCR_CALL(ncr_read32(gmacRegion, 0x324 + gmacPortOffset,
				    &ncr_status));
		ncr_status &= ~0x3c;
		ncr_status |= 0x08; /* Force Link Up */

		if (0 == strcmp("10MH", envstring)) {
		} else if (0 == strcmp("10MF", envstring)) {
			ncr_status |= 0x04;
		} else if (0 == strcmp("100MH", envstring)) {
			ncr_status |= 0x10;
		} else if (0 == strcmp("100MF", envstring)) {
			ncr_status |= 0x14;
		} else if (0 == strcmp("1G", envstring)) {
			ncr_status |= 0x24;
		} else {
			printf("macspeed must be set to 10MH, 10MF, 100MH, "
			       "100MF, or 1G\n");
			return -1;
		}

		NCR_CALL(ncr_write32(gmacRegion, 0x324 + gmacPortOffset,
				     ncr_status));
	} else {
		/* Get ad_value and ge_ad_value from the environment. */
		envstring = getenv("ad_value");

		if (NULL == envstring) {
			ad_value = 0x1e1;
		} else {
			ad_value = simple_strtoul(envstring, NULL, 0);
		}

		envstring = getenv("ge_ad_value");

		if (NULL == envstring) {
			ge_ad_value = 0x300;
		} else {
			ge_ad_value = simple_strtoul(envstring, NULL, 0);
		}

		/* Set the AN advertise values. */
		mdio_write(phy_by_index[index], 4, ad_value);
		mdio_write(phy_by_index[index], 9, ge_ad_value);

		/* Force re-negotiation. */
		control = mdio_read(phy_by_index[index], 0);
		control |= 0x200;
		mdio_write(phy_by_index[index], 0, control);

		DELAY();

		/* Wait for AN complete. */
		for (;;) {
			status = mdio_read(phy_by_index[index], 1);

			if (0 != (status & 0x20))
				break;

			if (0 == retries--) {
				ERROR_PRINT("GMAC%d: AN Timed Out.\n",
					    port_by_index[index]);
				return -1;
			}

			DELAY();
		}

		if (0 == (status & 0x4)) {
			ERROR_PRINT("GMAC%d: LINK is Down.\n",
				    port_by_index[index]);

			if (0 != eioaPort)
				return -1; /* Don't Error Out in AUTO Mode. */
		} else {
			status = mdio_read(phy_by_index[index], 0x1c);
			printf("GMAC%02d: ", port_by_index[index]);

			switch ((status & 0x18) >> 3) {
			case 0:
				puts("10M");
				break;
			case 1:
				puts("100M");
				break;
			case 2:
				puts("1G");
				break;
			default:
				puts("UNKNOWN");
				break;
			}

			printf(" %s\n",
			       (0 == (status & 0x20)) ?
			       "Half Duplex" : "Full Duplex");
			DELAY();

			/* Make the MAC match. */

			NCR_CALL(ncr_read32(gmacRegion, 0x324 + gmacPortOffset,
					    &ncr_status));
			ncr_status &= ~0x3c;
			ncr_status |= 0x08;	/* Force Link Up */

			if (0 != (status & 0x20))
				ncr_status |= 0x04; /* Force Full Duplex */

			/* Set the Speed */
			ncr_status |= (((status & 0x18) >> 3) << 4);

			NCR_CALL(ncr_write32(gmacRegion, 0x324 + gmacPortOffset,
					     ncr_status));
		}
	}

	/*
	  Set the Ethernet addresses...
	*/

	top = (ethernet_address[0] << 8) | ethernet_address[1];
	bottom = (ethernet_address[2] << 24) | (ethernet_address[3] << 16) |
	  (ethernet_address[4] << 8) | (ethernet_address[5]);

	/* - EIOA */
	NCR_CALL(ncr_write32(eioaRegion, 0xa0 + eioaPortOffset, bottom));
	NCR_CALL(ncr_write32(eioaRegion, 0xa4 + eioaPortOffset, top));

	/* - Source */
	NCR_CALL(ncr_write32(gmacRegion, 0x304 + gmacPortOffset, bottom));
	NCR_CALL(ncr_write32(gmacRegion, 0x308 + gmacPortOffset, top));

	/* - Unicast */
	NCR_CALL(ncr_write32(gmacRegion, 0x344 + gmacPortOffset, bottom));
	NCR_CALL(ncr_write32(gmacRegion, 0x348 + gmacPortOffset, bottom));
	NCR_CALL(ncr_write32(gmacRegion, 0x34c + gmacPortOffset,
			     ((top & 0xffff) << 16) | (top & 0xffff)));
	NCR_CALL(ncr_write32(gmacRegion, 0x350 + gmacPortOffset, bottom));
	NCR_CALL(ncr_write32(gmacRegion, 0x354 + gmacPortOffset, top));

	/* Enable stuff. */
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x8, 0x8));
	NCR_CALL(ncr_modify32(eioaRegion, 0x70, 0x11000000, 0x11000000));
	NCR_CALL(ncr_modify32(gmacRegion, 0x300 + gmacPortOffset, 0x4, 0x4));

	/* Unicast Filtering based on the Ethernet Address set above. */
	if (0 == rxtest && 0 == loopback)
		NCR_CALL(ncr_modify32(gmacRegion, 0x330 + gmacPortOffset,
				      0x3f, 0x09));

#ifdef ACP_25xx
	mdio_write(phy_by_index[index], 0x17, 0x2000);
	mdio_write(phy_by_index[index], 0x1b, 0x7000);
	mdio_write(phy_by_index[index], 0x00, 0x8000);

	udelay(10);
#endif

	return 0;

 ncr_exit:

	return -1;
}

/*
  -------------------------------------------------------------------------------
  initialize_task_lite
*/

static int
initialize_task_lite(void)
{
	ncp_st_t ncpStatus = NCP_ST_SUCCESS;
	char *eioaport;
	int i;

	/* Set the port. */
	eioaport = getenv("eioaport");

	if (NULL != eioaport) {
		if (0 == strcmp(eioaport, "AUTO")) {
			eioaPort = 0;
		} else if (0 == strcmp(eioaport, "GMAC1")) {
			eioaPort = 1;
		} else if (0 == strcmp(eioaport, "GMAC2")) {
			eioaPort = 2;
		} else if (0 == strcmp(eioaport, "GMAC3")) {
			eioaPort = 3;
		} else if (0 == strcmp(eioaport, "GMAC4")) {
			eioaPort = 4;
		} else if (0 == strcmp(eioaport, "GMAC9")) {
			eioaPort = 9;
		} else if (0 == strcmp(eioaport, "GMAC10")) {
			eioaPort = 10;
		} else if (0 == strcmp(eioaport, "GMAC11")) {
			eioaPort = 11;
		} else if (0 == strcmp(eioaport, "GMAC12")) {
			eioaPort = 12;
		} else {
			WARN_PRINT("If set, eioaport must be AUTO, "
				   "GMAC1...GMAC4, or GMAC9...GMAC12\n");
			return -1;
		}
	} else {
		eioaPort = 0;
	}

	if (0 != ncp_dev_reset()) {
		WARN_PRINT("Reset Failed\n");
		return -1;
	}

	tlb_entries(1);

	if (0 != ncp_dev_configure(mmb)) {
		WARN_PRINT("MMB Configuration Failed\n");
		return -1;
	}

	if (0 != ncp_dev_configure(vp)) {
		WARN_PRINT("Virtual Pipeline Configuration Failed\n");
		return -1;
	}

	if (0 != ncp_dev_configure(nca)) {
		WARN_PRINT("NCA Configuration Failed\n");
		return -1;
	}

	NCP_CALL_LITE(
		ncp_task_lite_uboot_register_allocators(alloc_128B_aligned,
							free_128B_aligned,
							alloc_nvm,
							free_nvm,
							va2pa));
	NCP_CALL_LITE(ncp_task_lite_uboot_config(NULL));
	NCP_CALL_LITE(ncp_task_lite_hdl_create(0,
					       (ncp_uint8_t)queueSetId,
					       &taskLiteHdl));
	NCP_CALL_LITE(ncp_task_lite_recv_hdl_create(taskLiteHdl,
						    (ncp_uint8_t)ncaQueueId,
						    &taskLiteRxHdl));

	/*
	  Add receive buffers.
	*/

	for (i = 0; i < NUMBER_OF_RX_BUFFERS; ++i) {
		if (NULL == (rx_buffers[i] = alloc_128B_aligned(2048))) {
			ERROR_PRINT("RX Buffer Allocation Failed.\n");
			return -1;
		}

		ncp_task_lite_supply_rxbuffer_to_nca(taskLiteHdl,
						     rx_buffers[i], 3);
	}

#ifdef ACP_25xx
	if (0 != ncp_dev_configure(timer)) {
		WARN_PRINT("TIMER Configuration Failed\n");
		return -1;
	}

	if (0 != ncp_dev_configure(tdmioa)) {
		WARN_PRINT("TDMIOA Configuration Failed\n");
		return -1;
	}
#endif

	if (0 != ncp_dev_configure(eioa)) {
		WARN_PRINT("EIOA Configuration Failed\n");
		return -1;
	}

 ncp_return:

	if (NCP_ST_SUCCESS != ncpStatus) {
		acp_eioa_eth_halt();

		return -1;
	}

	/*
	  Make sure the network is connected.
	*/

	if (0 == eioaPort) {
		int i;
		/* Use all ports. */
		for (i = 0; i < 8; ++i) {
			if (0 != line_setup(i)) {
				return -1;
			}
		}
	} else {
		if (0 != line_setup(index_by_port[eioaPort])) {
			return -1;
		}
	}

	initialized = 1;

	return 0;
}

/*
  -------------------------------------------------------------------------------
  finalize_task_lite
*/

void
finalize_task_lite(void)
{
	int i;
	unsigned long value;

	DEBUG_PRINT("\n");

	/*
	  Stop the queue.
	*/

	/* Disable EIOA NEMACs. */
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0x3c0, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0x480, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x1f, 0x12), 0x540, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0x300, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0x3c0, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0x480, 0x0000000c, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x12), 0x540, 0x0000000c, 0x0);

	/* Disable the MACs. */
	ncr_or(NCP_REGION_ID(0x1f, 0x12), 0x20, 0xf);
	ncr_or(NCP_REGION_ID(0x17, 0x12), 0x20, 0xf);

	/* Disable EIOA Cores. */
	ncr_modify32(NCP_REGION_ID(0x1f, 0x10), 0x70, 0xff000000, 0x0);
	ncr_modify32(NCP_REGION_ID(0x17, 0x10), 0x70, 0xff000000, 0x0);

	/* Disable PCQ 0 (only queue 0 is used) */
	value = readl(NCA + 0x800);
	value &= ~0x00008000;
	writel(value, (NCA + 0x800));

	/*
	  Shut down task lite.
	*/

	if (taskLiteRxHdl)
		ncp_task_lite_recv_hdl_remove(taskLiteHdl, taskLiteRxHdl);

	for (i = 0; i < NUMBER_OF_RX_BUFFERS; ++i) {
		free_128B_aligned(rx_buffers[i]);
	}

	if (taskLiteHdl) {
		ncp_task_lite_free_send_buffers(taskLiteHdl);
		ncp_task_lite_hdl_remove(taskLiteHdl);
	}

	ncp_task_lite_uboot_unconfig();

	/*
	  Remove TLB entries.
	*/

	tlb_entries(0);

	initialized = 0;

	return;
}

/*
  ==============================================================================
  ==============================================================================
  Public Interface
  ==============================================================================
  ==============================================================================
*/

/*
  -------------------------------------------------------------------------------
  acp_eioae_eth_halt
*/

void
acp_eioa_eth_halt(void)
{
	if (0 != initialized)
		finalize_task_lite();

	return;
}

/*
  ----------------------------------------------------------------------
  acp_eioa_eth_init
*/

int
acp_eioa_eth_init(bd_t *bd)
{
	if (0 == initialized)
		if (0 != initialize_task_lite()) {
			printf("Failed to Initialize TaskIO Lite!\n");
			return -1;
		}

	return 0;
}

/*
  -------------------------------------------------------------------------------
  acp_eioa_eth_send
*/

int
acp_eioa_eth_send(volatile void *packet, int length)
{
	int bytes_sent;
	void *buffer;
	int i;

	DEBUG_PRINT("\n");

	if (0 == eioaPort) {
		ncp_task_lite_send_meta_t *taskLiteMetaData;

		/* Broadcast to all ports. */
		/* TODO: Is there an argument to broadcast to all ports? */

		bytes_sent = 0;

		for (i = 0; i < 8; ++i) {
			taskLiteMetaData =
				alloc_128B_aligned(sizeof(ncp_task_lite_send_meta_t));

			if (NULL == taskLiteMetaData) {
				ERROR_PRINT("Couldn't allocate send buffer "
					    "meta data.\n");
				return 0;
			}

			buffer = alloc_128B_aligned(length);

			if (NULL == buffer) {
				free_128B_aligned(taskLiteMetaData);
				ERROR_PRINT("Couldn't allocate send buffer.\n");
				return 0;
			}

			memcpy(buffer, (void *)packet, length);
			flush_cache((unsigned long)buffer, length);
			memset(taskLiteMetaData, 0,
			       sizeof(ncp_task_lite_send_meta_t));
			taskLiteMetaData->dataSize        = length;
			taskLiteMetaData->taskBuffer      = buffer;
			taskLiteMetaData->taskPduData     = buffer;
			taskLiteMetaData->clientFreeFn    = client_free;
			taskLiteMetaData->clientFreeFnArg = buffer;
			taskLiteMetaData->taskParms[0]    = port_by_index[i];
			flush_cache((unsigned long)taskLiteMetaData,
				    sizeof(ncp_task_lite_send_meta_t));

			if (length != task_lite_send(taskLiteMetaData)) {
				printf("Send Failed on Port %d\n",
				       port_by_index[i]);
			} else {
				bytes_sent = length;
			}
		}
	} else {
		ncp_task_lite_send_meta_t taskLiteMetaData;

		if (NULL == (buffer = alloc_128B_aligned(length))) {
			ERROR_PRINT("Couldn't allocate send buffer.\n");
			return 0;
		}

		memcpy(buffer, (void *)packet, length);
		flush_cache((unsigned long)buffer, length);
		memset(&taskLiteMetaData, 0, sizeof(ncp_task_lite_send_meta_t));
		taskLiteMetaData.dataSize        = length;
		taskLiteMetaData.taskBuffer      = buffer;
		taskLiteMetaData.taskPduData     = buffer;
		taskLiteMetaData.clientFreeFn    = client_free;
		taskLiteMetaData.clientFreeFnArg = buffer;
		flush_cache((unsigned long)&taskLiteMetaData,
			    sizeof(ncp_task_lite_send_meta_t));

		if (0 != dumptx)
			dump_packet("TX", buffer, length);

		taskLiteMetaData.taskParms[0] = eioaPort;
		if (length != (bytes_sent = task_lite_send(&taskLiteMetaData))) {
			printf("Send Failed on Port %d\n", eioaPort);
		}
	}

	return bytes_sent;
}

/*
  -------------------------------------------------------------------------------
  acp_eioa_eth_rx
*/

int
acp_eioa_eth_rx(void)
{
	ncp_st_t ncpStatus;
	ncp_task_lite_recv_buf_t *task;
	void *taskData;
	int bytes_received = 0;

	DEBUG_PRINT("\n");
	ncpStatus = ncp_task_lite_recv(taskLiteHdl, taskLiteRxHdl,
				       &task, &taskData, FALSE);

	if (NCP_ST_TASK_RECV_QUEUE_EMPTY == ncpStatus) {
		return 0;
	}

	if (NCP_ST_SUCCESS != ncpStatus) {
		NCP_CALL_LITE(ncpStatus);
	}

	if (task->taskErr ||
	    task->bufferSizeErr ||
	    ((0 != eioaPort) && (task->parms[0] != eioaPort))) {
		NCP_CALL_LITE(
			ncp_task_lite_supply_rxbuffer_to_nca(taskLiteHdl,
							     task,
							     task->bufferSize));
		return 0;
	}

	if (0 == eioaPort) {
		eioaPort = task->parms[0];
		printf("Selecting EIOA Port GMAC%02d\n", eioaPort);
	}

	bytes_received = task->dataSz;
	memcpy((void *)NetRxPackets[0], (void *)taskData, bytes_received);

	if (0 != dumprx)
		dump_packet("RX", (void *)NetRxPackets[0], bytes_received);

	if (0 == loopback && 0 == rxtest)
		NetReceive(NetRxPackets[0], bytes_received);

	NCP_CALL_LITE(
		ncp_task_lite_supply_rxbuffer_to_nca(taskLiteHdl,
						     task,
						     task->bufferSize));

 ncp_return:

	if (NCP_ST_SUCCESS != ncpStatus) {
		printf("%s:%d - NCP_CALL_LITE() Failed: 0x%08x\n",
		       __FILE__, __LINE__, ncpStatus);
		return 0;
	}

	return bytes_received;
}

/*
  -------------------------------------------------------------------------------
  acp_eioa_receive_test
*/

void
acp_eioa_receive_test(void)
{
	int save_dumprx;
	int packets_received = 0;
	bd_t *bd = gd->bd;

	DEBUG_PRINT("\n");
	save_dumprx = dumprx;
	dumprx = 1;
	rxtest = 1;
	eth_halt();

	if (0 != eth_init(bd)) {
		eth_halt();
		dumprx = save_dumprx;
		rxtest = 0;
		return;
	}

	for (;;) {
		if (0 != eth_rx())
			++packets_received;

		if (ctrlc())
			break;
	}

	eth_halt();
	dumprx = save_dumprx;
	rxtest = 0;
	printf("EIOA Receive Test Interrupted.  Received %d packets.\n",
	       packets_received);

	return;
}

/*
  -------------------------------------------------------------------------------
  acp_eioa_loopback_test
*/

void
acp_eioa_loopback_test(void)
{
	bd_t *bd = gd->bd;
	int bytes_received;
	int packets_looped = 0;

	DEBUG_PRINT("\n");
	loopback = 1;
	eth_halt();

	if (0 != eth_init(bd)) {
		eth_halt();
		return;
	}

	for (;;) {
		if (0 != (bytes_received = eth_rx())) {
			if (bytes_received !=
			    eth_send((void *)NetRxPackets[0], bytes_received)) {
				ERROR_PRINT("acp_eioa_eth_send() failed.\n");
			} else {
				++packets_looped;
			}
		}

		if (ctrlc())
			break;
	}

	loopback = 0;
	eth_halt();
	printf("EIOA Loopback Test Interrupted.  Looped back %d packets.\n",
	       packets_looped);

	return;
}
