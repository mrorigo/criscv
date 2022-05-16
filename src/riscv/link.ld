SECTIONS
{
  . = 0x00010000;
  .text           :
  {
    *(.text)
    *(.text.unlikely .text.*_unlikely .text.unlikely.*)
    *(.text.exit .text.exit.*)
    *(.text.startup .text.startup.*)
    *(.text.hot .text.hot.*)
    *(.stub .text.* .gnu.linkonce.t.*)
    *(.gnu.warning)
  }

  .data           :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    SORT(CONSTRUCTORS)
  }

  .sdata          :
  {
    __global_pointer$ = . + 0x800;
    *(.srodata.cst16) *(.srodata.cst8) *(.srodata.cst4) *(.srodata.cst2) *(.srodata .srodata.*)
    *(.sdata .sdata.* .gnu.linkonce.s.*)
  }

  _edata = .; PROVIDE (edata = .);

  . = ALIGN(32 / 8);
  _end = .; PROVIDE (end = .);
}
