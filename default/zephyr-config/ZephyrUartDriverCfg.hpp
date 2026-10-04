// ======================================================================
// \title  ZephyrUartDriverCfg.hpp
// \brief  Compile-time configuration for Zephyr::ZephyrUartDriver
//
// Projects may override these values by supplying their own copy of this
// header in a config directory that takes precedence over
// default/zephyr-config (standard F Prime configuration override).
// ======================================================================
#ifndef ZEPHYR_UART_DRIVER_CFG_HPP
#define ZEPHYR_UART_DRIVER_CFG_HPP

#include <Fw/FPrimeBasicTypes.hpp>

namespace Zephyr {
namespace ZephyrUartDriverCfg {

//! Size in bytes of the software RX ring buffer filled by the UART ISR.
//! When full, the driver disables the RX interrupt (back-pressure) instead of
//! discarding bytes; it is re-enabled once the ring has been drained.
static constexpr FwSizeType RX_RING_SIZE = 1024;

//! Maximum size in bytes of a single Fw::Buffer requested from the allocator
//! and delivered on the `recv` port per drain iteration. Must be <= RX_RING_SIZE
//! and <= the smallest buffer the connected allocator can provide.
static constexpr FwSizeType RX_CHUNK_SIZE = 256;

//! Period in milliseconds after which the optional RX task wakes even when
//! no interrupt has signalled it (defensive safety net against a missed wake).
static constexpr U32 RX_TASK_WAKE_TIMEOUT_MS = 100;

static_assert(RX_CHUNK_SIZE > 0, "RX_CHUNK_SIZE must be positive");
static_assert(RX_CHUNK_SIZE <= RX_RING_SIZE, "RX_CHUNK_SIZE must not exceed RX_RING_SIZE");

}  // namespace ZephyrUartDriverCfg
}  // namespace Zephyr

#endif  // ZEPHYR_UART_DRIVER_CFG_HPP
