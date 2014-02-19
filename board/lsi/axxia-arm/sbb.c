/*
 * sbb.c
 *
 * Interface to the SBB (Secure Boot Block)
 *
 * Copyright (C) 2014 LSI Corporation
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
  Local
  ==============================================================================
  ==============================================================================
*/

#define ECM_BASE (IO+0x10031800)
#define SBB_BASE (IO+0x10160000)

/*
  ------------------------------------------------------------------------------
  read_ecm
*/

static int
read_ecm(int bit_address, int number_of_bits, int pe_means_zero,
	 unsigned long *destination)
{
	unsigned long value;
	int i;

	/*
	  Verify that ReadEFuse is 0 (bit 0 of the control register).
	*/

	value = readl(ECM_BASE + 0x1300);

	if (0 != (value & 1)) {
		printf("ECM Read Error: ReadEFuse is not 0.\n");
		return -1;
	}

	/*
	  Set ClearReadData and PwrDown.  Clear PadEnb.
	*/

	writel(0x8080, ECM_BASE + 0x1300);

	/*
	  Set ReadAddr 11:0 to the starting bit address.
	*/

	value = readl(ECM_BASE + 0x1305);
	value &= ~0x1fff;
	value |= (bit_address & 0x1fff);
	writel(value, ECM_BASE + 0x1305);

	/*
	  Clear PwrDown and PadEnb.
	*/

	writel(0, ECM_BASE + 0x1300);

	/*
	  Wait at least 900 ns.
	*/

	udelay(1);

	/*
	  Clear the status bits.
	*/

	writel(0x1fbc0000, ECM_BASE + 0x1301);

	/*
	  Update ReadDataSize (number of bits to read).  Clear PwrDown and
	  PadEnb.  Set ReadEFuse.
	*/

	value = readl(ECM_BASE + 0x1300);
	value &= ~0xff000000;
	value |= (number_of_bits & 0xff) << 24;
	value &= ~(1 << 7);
	value &= ~(1 << 6);
	value |= 1;
	writel(value, ECM_BASE + 0x1300);

	/*
	  Wait for LocalReadDone to be set.
	*/

	while (0 == (readl(ECM_BASE + 0x1301) & 0x20000000)) {
		udelay(1);
	}

	/*
	  Set PwrDown and clear PadEnb.
	*/

	writel(0x80, ECM_BASE + 0x1300);

	/*
	  Clear LocalReadDone.
	*/

	writel(0x20000000, ECM_BASE + 0x1301);

	/*
	  Check the status.
	*/

	value = (0x1fbc0000 & readl(ECM_BASE + 0x1301));

	if (0 != value) {
		printf("ECM Read Error: Status is 0x%08lx.\n", value);
		return -2;
	} else {
		/*
		  Read Values.
		*/

		for (i = 0; i <= (number_of_bits / 32); ++i)
			if (3 == number_of_bits)
				*destination++ =
					readl(ECM_BASE + (0x1310 + i));
			else
				*destination++ =
					swab32(readl(ECM_BASE + (0x1310 + i)));
	}

	return 0;
}

/*
  ------------------------------------------------------------------------------
  verify_sbb_enabled
*/

static int
verify_sbb_enabled(void)
{
	unsigned long fuses[8];

	/* Export Security Disable Bit. */
	memset((void *)fuses, 0, sizeof(unsigned long) * 8);

	if (0 != read_ecm(0x8, 0x3, 1, (void *)fuses)) {
		printf("read_ecm() failed!\n");
		return -1;
	}

	if (0 != (fuses[0] & 0x8)) {
		printf("Export Security Disable is SET!\n");
		return -1;
	}

	/* Secure Boot Enable Bit. */
	memset((void *)fuses, 0, sizeof(unsigned long) * 8);

	if (0 != read_ecm(0x18, 0x3, 0, (void *)fuses)) {
		printf("read_ecm() failed!\n");
		return -1;
	}

	if (0 == (fuses[0] & 0x8)) {
		printf("Secure Boot Enable is NOT SET!\n");
		return -1;
	}

#if 0
	/* Master Key Expect. */
	memset((void *)fuses, 0, sizeof(unsigned long) * 8);

	if (0 != read_ecm(0x518, 0x7f, 0, (void *)fuses)) {
		printf("read_ecm() failed!\n");
		return -1;
	}

	{
		int i;

		puts("Master Key Expect is\n\t");

		for (i = 0; i < 4; ++i) {
			printf("%08lx", fuses[i]);
		}

		puts("\n");
	}

	/* Authentication Key Expect. */
	memset((void *)fuses, 0, sizeof(unsigned long) * 8);

	if (0 != read_ecm(0x6b8, 0x7f, 0, (void *)fuses)) {
		printf("read_ecm() failed!\n");
		return -1;
	}

	{
		int i;

		puts("Authentication Key Expect is\n\t");

		for (i = 0; i < 4; ++i) {
			printf("%08lx", fuses[i]);
		}

		puts("\n");
	}

	/* Public Key */
	memset((void *)fuses, 0, sizeof(unsigned long) * 8);

	if (0 != read_ecm(0x808, 0xff, 0, (void *)fuses)) {
		printf("read_ecm() failed!\n");
		return -1;
	}

	{
		int i;

		puts("Public Key is\n\t");

		for (i = 0; i < 8; ++i) {
			printf("%08lx", fuses[i]);
		}

		puts("\n");
	}
#endif

	return 0;
}

/*
  ------------------------------------------------------------------------------
  lock_sbb
*/

static int
lock_sbb(void)
{
	unsigned long value;

	value = readl(SBB_BASE + 0x800);

	if (0 == value)
		return -1;

	return 0;
}

/*
  ------------------------------------------------------------------------------
  unlock_sbb
*/

static void
unlock_sbb(void)
{
	writel(0xffffffff, (SBB_BASE + 0x800));
}

static int
report_errors(unsigned long sbb_interrupt_status)
{
	int rc = 0;

	/*
	  In all other cases, display the error string(s).
	*/

	if (0 != (sbb_interrupt_status & 0x4)) {
		printf("SBB Error: HW Error\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x8)) {
		printf("SBB Error: Invalid Master Key\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x10)) {
		printf("SBB Error: Invalid Authentication Key\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x20)) {
		printf("SBB Error: Secure Boot is Disabled\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x40)) {
		printf("SBB Error: Invalid Function\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x80)) {
		printf("SBB Error: Invalid ID\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x100)) {
		printf("SBB Error: Invalid Length\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x200)) {
		printf("SBB Error: Invalid Padding\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x400)) {
		printf("SBB Error: Unaligned Data\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x800)) {
		printf("SBB Error: SDO Authentication Fails\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x1000)) {
		printf("SBB Error: Invalid Boot Image\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x2000)) {
		printf("SBB Error: ECDSA Initialization Failed\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x4000)) {
		printf("SBB Error: Invalid Version Number\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x8000)) {
		printf("SBB Error: Unaligned Address\n");
		rc = -1;
	}

	if (0 != (sbb_interrupt_status & 0x20000)) {
		printf("SBB Error: Self-Test Failed\n");
		rc = -1;
	}

	return rc;
}

/*
  ------------------------------------------------------------------------------
  run_sbb_function
*/

static int
run_sbb_function(int function,
		 unsigned long *parameters, int number_of_parameters)
{
	int i;
	unsigned long value;

	/* Make sure SBB is enabled (check the fuses). */
	if (0 != verify_sbb_enabled()) {
		return -1;
	}

	/* Get the semaphore. */
	if (0 != lock_sbb())
		return -1;

	/* Write the input parameters. */
	for (i = 0; i < number_of_parameters; ++i) {
		writel(parameters[i], (SBB_BASE + 0x838 + (i * 4)));
	}

	/* Clear the interrupt status registers. */
	writel(0xffffffff, (SBB_BASE + 0xa04));

	/* Write the function. */
	writel(function, (SBB_BASE + 0x804));

	if (function != readl(SBB_BASE + 0x804))
		return -1;

	/* Wait for the "done" bit. */
	value = readl(SBB_BASE + 0xa04);

	while (0 == (value & 1)) {
		udelay(1000);

		if (ctrlc()) {
			unlock_sbb();
			return -1;
		}

		value = readl(SBB_BASE + 0xa04);
	}

	/* Release the semaphore. */
	unlock_sbb();

	/* Check for errors. */
	if (0 != report_errors(value))
		return -1;

	return 0;
}

/*
  ==============================================================================
  ==============================================================================
  Public
  ==============================================================================
  ==============================================================================
*/

/*
  ------------------------------------------------------------------------------
  sbb_verify_image
*/

int
sbb_verify_image(void *source, void *destination, int safe)
{
	unsigned long parameters[4];

	/* Set up the parameters. */
	parameters[0] = 0;
	parameters[1] = (unsigned long)source;
	parameters[2] = 0;
	parameters[3] = (unsigned long)destination;

	/* Run the function. */
	if (0 != run_sbb_function((0 == safe) ? 1 : 11, parameters, 4)) {
		memset((void *)destination, 0, 16);
		return -1;
	}

	return 0;
}
