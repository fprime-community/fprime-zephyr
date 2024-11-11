// ======================================================================
// \title  FprimeZephyr.hpp
// \author ethancheez
// \brief  Undefinitions of Zephyr-specific macros
// ======================================================================
#ifndef FPRIMEZEPHYR_HPP
#define FPRIMEZEPHYR_HPP

// These includes are needed for `k_tid_t`. Remarks:
//   1) Including <zephyr/kernel.h> doesn't work out of the box because it depends on
//      compile-time generated `syscall_list.h` which is why the cherrypicked headers
//      are below are used.
//   2) #undef CRC is needed because the CRC defined in `stm32h723xx.h` HAL header
//      conflicts with `CmdSequencerImpl` internal CRC struct.
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/kernel/thread.h>
#undef CRC
typedef k_tid_t TaskIdRepr;

#include <zephyr/sys/util_macro.h>
#undef EMPTY

#endif  // FPRIMEZEPHYR_HPP
