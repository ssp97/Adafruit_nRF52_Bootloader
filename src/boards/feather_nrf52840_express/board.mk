MCU_SUB_VARIANT = nrf52840

# QSPI Flash Configuration
# Enable QSPI Flash support
ENABLE_QSPI_FLASH ?= 0

# QSPI Flash size in bytes (W25Q16 = 2MB)
QSPI_FLASH_SIZE ?= 2097152

# QSPI Flash XIPOFFSET (start address after internal flash)
QSPI_XIP_OFFSET ?= 0x100000

# QSPI Flash pins configuration
QSPI_SCK_PIN ?= 13
QSPI_CSN_PIN ?= 14
QSPI_IO0_PIN ?= 15
QSPI_IO1_PIN ?= 16
QSPI_IO2_PIN ?= 17
QSPI_IO3_PIN ?= 18
