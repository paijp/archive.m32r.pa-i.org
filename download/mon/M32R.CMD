/*
 * m32r.cmd -- Linker script for M32R & MTM
 * This script is based on `m32r-elf/lib/ldscripts/m32relf.x'.
 * @(#) m32r.cmd 1.2@(#) 99/03/16
 * $Id: m32r.cmd,v 1.1.1.1 2003/02/04 07:17:49 inui Exp $
 */

/*[System Calls(You selected)]*/
/*
 * Users are not allowed to remove and modify the below
 * blocks because MTM should handle these blocks:
 *   - MTM_INPUT
 *   - MTM_SEARCH_DIR
 *   - MTM_GROUP
 */
/*[MTM_INPUT_TOP]*/
/*[MTM_INPUT_END]*/
/*[MTM_SEARCH_DIR_TOP]*/

/*[MTM_SEARCH_DIR_END]*/
/*[MTM_GROUP_TOP]*/
/*[MTM_GROUP_END]*/


OUTPUT_FORMAT("elf32-m32r", "elf32-m32r", "elf32-m32r")
OUTPUT_ARCH(m32r)
ENTRY(__START)
/* Do we need any of these for elf?
   __DYNAMIC = 0;    */

/* Memory Layout
 *   This layout is just example.  Applying this example, it
 *   is able to check a linking process exactly.
 *
 */
MEMORY
{
	in_rom   : org = 0, len = 4M
	in_ram1  : org = 0xf00000, len = 64K
	out_ram1 : org = 0x1000000, len = 4M
}

/* Section Layout
 *
 *   <section name>	<placed in>	<placed in while executing>
 *   .reset		ROM		ROM
 *   .EIT_Vector	ROM		ROM
 *   .Int_Vector	ROM		ROM
 *   .text		ROM		ROM
 *   .rodata		ROM		ROM
 *   .data		ROM		RAM
 *   .sdata		ROM		RAM
 *   .sbss		RAM		RAM
 *   .bss		RAM		RAM
 *   .heap		RAM		RAM
 *   .spu_stack		RAM		RAM
 *   .spi_stack		RAM		RAM
 */
SECTIONS
{
  RESET = 0;
  START_OF_MWIF = 0xF00000;
  START_OF_OS = 0x40100;
  START_OF_EXT_RAM = 0x1000000;
  START_OF_CONFIG = 0x40000;

  /* reset vector sections */
/*
  .STARTUP (START_OF_ROM) : AT(START_OF_ROM)
  {
    *(.STARTUP)
  }
*/

  .CONFIG( START_OF_CONFIG ) : AT( START_OF_CONFIG )
  {
    *(.CONFIG)
  }

  .OS_START( START_OF_OS ) : AT( START_OF_OS )
  {
    *(.OS_START)
  }

  .text (START_OF_EXT_RAM) : AT(LOADADDR(.OS_START) + SIZEOF(.OS_START))
  {
    __text_start = .;
    *(.MR_KERNEL)
    *(.mwif)
    *(.text)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
    __text_end = .;
  } =0
  __rom_text = LOADADDR(.text);
  _etext = .;
  PROVIDE (etext = .);


  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */

  .data (ADDR(.text)+SIZEOF(.text)) : AT( LOADADDR(.text) + SIZEOF(.text) )
  {
    __data_start = .;
    *(.data)
    *(.rodata)
    *(.gnu.linkonce.d*)
    CONSTRUCTORS
    __data_end = .;
  }
  __rom_data = LOADADDR(.data);

  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */

  .sdata (ADDR(.data)+SIZEOF(.data)) : AT( LOADADDR(.data) + SIZEOF(.data) )
  {
    __sdata_start = .;
    *(.sdata)
    __sdata_end = .;
  }
  __rom_sdata = LOADADDR(.sdata);

  _edata  =  .;
  PROVIDE (edata = .);

  . = ADDR(.sdata)+SIZEOF(.sdata);

  .sbss (.) : 
     {
	__sbss_start = .;
	*(.sbss)
	*(.scommon)
	__sbss_end = .;
     }
  .bss (.) : 
     {
	__bss_start = .;
	*(.dynbss)
	*(.bss)
	*(COMMON)
	__bss_end = .;
     }
  _end = . ;
  PROVIDE (end = .);

  /* Stabs debugging sections.  */
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
}
