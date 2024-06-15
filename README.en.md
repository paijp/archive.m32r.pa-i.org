
# Introduction (2006/12/09)

m32r.pa-i.org is a site that publishes software and hardware for free M32R/E processors created by pi.

The site is currently in provisional operation and is in the process of content development.

We are sorry, but we are not disclosing our contact information at the moment. Please note that we are preparing an inquiry form.

# Basic Monitor (2006/12/09)

This is a monitor for OAKS32R. By inputting commands from a serially connected terminal, memory operations, downloading to memory, program execution, and automatic program execution with jumper switches can be performed.

## Functions

## How to install

After building, use the downloader attached to OAKS32R to write to Flash ROM.

## How to use

Connect to the board via 115.2 kbps serial, open jumper 2 and turn on the power, you will see a prompt. Use "h" for help.

Shorting jumper 2 and turning on power will start execution without prompt.

# μITRON4.0 specification OS (2006/12/09)

µITRON for the OAKS-M32R, developed to run software on a DJ CD player without using SDRAM on the external bus.

## Functions

## How to install

Download using [monitor].

## How to use

The download file itself contains a test program because of its structure to be linked with the program.

## CD player for DJ (2006/12/09)

This is a twin CD player + mixer for DJ that uses the DSP function of the M32R to perform variable pitch playback.

# Tips (2006/12/09)

How to make the interrupt vector of M32R rewritable in RAM. This avoids the risk of failure to write the flash ROM, which would result in a boot failure.

When using the M32102 without ICE, there is a risk of losing the reset vector and becoming unbootable if the rewrite of the interrupt vector on the Flash ROM fails; by manipulating SDRAMC and assigning SDRAM to address 0, SDRAM access has priority over BSEL0. SDRAM accesses have priority over BSEL0, so the memory area of the interrupt vector can be overwritten with SDRAM. However, to do this, SDRAMCs must be stopped once, and the refresh will stop and the contents of SDRAM will no longer be guaranteed. Although we have not verified this, we believe that the flash ROM shadow is visible at the address after SDRAM, so it may be possible to copy from here to SDRAM.

# Tips (2006/12/09)

This is a method to ensure long setup time for 16bit external bus without using M32R's external wait.

When connecting an ATA to the M32102, the dynamic bus sizing, which splits a 32-bit access into two 16-bit accesses, can be successfully used to address the following issues with less hardware.

The first problem is that the long setup time required by PIO mode 0 of the ATA cannot be achieved with the bus controller configuration. In this system, when 32-bit accesses are made to BSEL4 for command access, CS is held without asserting RD and WR during the first divided access, and RD and WR are asserted during the second access. This ensures a long setup time. If PIO mode 2, which is the highest speed that does not require a wait signal, does not require a long setup time, then after switching to this mode, 16-bit accesses to odd addresses in BSEL4 will result in PIO mode 2 timing.

Another problem is that ATA is a 16-bit bus, so it is not efficient to transfer data to SDRAM by DMA. To address this, we simply read the 16-bit bus twice when BSEL5 for data access is used for 32-bit access. This makes it more efficient to transfer 32 bits by DMA, as 16 bits are read twice and written as 32 bits.

BSEL4 and BSEL5 differ only in the function of holding CS without asserting RD and WR, and the address mapping, etc. are identical.

# Main Board (2006/12/09)

Printed circuit board data of the main board of a DJ CD player.

# Power Supply Board (2006/12/09)

Printed circuit board data for power distribution and motor drive of CD player for DJ.

# For Display Module (2006/12/09)

Code for PIC16C84 for FL-tube display module of CD player for DJ.

# for key scan (2006/12/09)

Code for PIC16C74 for key scan, LED display, AD conversion, etc. for DJ CD players.

# For Scratch (2006/12/09)

Code for PIC12C508, which is a scratch controller for DJ CD players and drives the coil.
