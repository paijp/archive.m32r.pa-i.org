
OUTPUT_FORMAT("elf32-m32r", "elf32-m32r", "elf32-m32r")
OUTPUT_ARCH(m32r)
ENTRY(__START)

MEMORY
{
	in_ram1  : org = 0xf00000, len = 64K
}

SECTIONS
{
  START_OF_RAM = 0xf00200;

  .startup (START_OF_RAM) : AT(START_OF_RAM)
  {
    *(.__startup);
  }

  .text (.) : 
  {
    __text_start = .;
    __START = .;
    *(.text)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
    __text_end = .;
  } =0

  .data (.) : 
  {
    __data_start = .;
    *(.data)
    *(.rodata)
    *(.gnu.linkonce.d*)
    CONSTRUCTORS
    
    *(.sdata)
    __data_end = .;
  }

  .bss (.) : 
     {
	__bss_start = .;
	*(.sbss)
	*(.scommon)
	
	*(.dynbss)
	*(.bss)
	*(COMMON)
	__bss_end = .;
     }
}
