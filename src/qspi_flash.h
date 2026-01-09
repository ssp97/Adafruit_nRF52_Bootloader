/*
 * QSPI Flash driver for Adafruit nRF52 Bootloader
 */

#ifndef QSPI_FLASH_H_
#define QSPI_FLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
 extern "C" {
#endif

// W25Q16 Flash commands
#define W25Q16_CMD_WRITE_ENABLE         0x06
#define W25Q16_CMD_WRITE_DISABLE        0x04
#define W25Q16_CMD_READ_STATUS_REG1     0x05
#define W25Q16_CMD_READ_STATUS_REG2     0x35
#define W25Q16_CMD_WRITE_STATUS_REG     0x01
#define W25Q16_CMD_PAGE_PROGRAM         0x02
#define W25Q16_CMD_QUAD_PAGE_PROGRAM    0x32
#define W25Q16_CMD_SECTOR_ERASE_4KB     0x20
#define W25Q16_CMD_BLOCK_ERASE_32KB     0x52
#define W25Q16_CMD_BLOCK_ERASE_64KB     0xD8
#define W25Q16_CMD_CHIP_ERASE           0xC7
#define W25Q16_CMD_ERASE_PROGRAM_SUSPEND 0x75
#define W25Q16_CMD_ERASE_PROGRAM_RESUME 0x7A
#define W25Q16_CMD_POWER_DOWN           0xB9
#define W25Q16_CMD_RELEASE_POWER_DOWN   0xAB
#define W25Q16_CMD_MANUFACTURER_DEVICE_ID 0x90
#define W25Q16_CMD_READ_UNIQUE_ID       0x4B
#define W25Q16_CMD_JEDEC_ID             0x9F
#define W25Q16_CMD_READ_DATA            0x03
#define W25Q16_CMD_FAST_READ            0x0B
#define W25Q16_CMD_FAST_READ_DUAL_OUTPUT 0x3B
#define W25Q16_CMD_FAST_READ_DUAL_IO    0xBB
#define W25Q16_CMD_FAST_READ_QUAD_OUTPUT 0x6B
#define W25Q16_CMD_FAST_READ_QUAD_IO    0xEB

// W25Q16 Flash parameters
#define W25Q16_PAGE_SIZE                256
#define W25Q16_SECTOR_SIZE              4096
#define W25Q16_BLOCK_SIZE_32KB          (32 * 1024)
#define W25Q16_BLOCK_SIZE_64KB          (64 * 1024)
#define W25Q16_TOTAL_SIZE               (2 * 1024 * 1024) // 2MB

// Status register bits
#define W25Q16_STATUS_BUSY              (1 << 0)
#define W25Q16_STATUS_WRITE_ENABLE      (1 << 1)
#define W25Q16_STATUS_BLOCK_PROTECT0    (1 << 2)
#define W25Q16_STATUS_BLOCK_PROTECT1    (1 << 3)
#define W25Q16_STATUS_BLOCK_PROTECT2    (1 << 4)
#define W25Q16_STATUS_BLOCK_PROTECT3    (1 << 5)
#define W25Q16_STATUS_TOP_BOTTOM        (1 << 6)
#define W25Q16_STATUS_STATUS_PROTECT    (1 << 7)

// QSPI Flash status
typedef enum {
    QSPI_FLASH_STATUS_SUCCESS,
    QSPI_FLASH_STATUS_BUSY,
    QSPI_FLASH_STATUS_ERROR,
    QSPI_FLASH_STATUS_TIMEOUT
} qspi_flash_status_t;

// Initialize QSPI Flash
qspi_flash_status_t qspi_flash_init(void);

// Deinitialize QSPI Flash
void qspi_flash_deinit(void);

// Read data from QSPI Flash
qspi_flash_status_t qspi_flash_read(uint32_t address, uint8_t *data, size_t length);

// Write data to QSPI Flash
qspi_flash_status_t qspi_flash_write(uint32_t address, const uint8_t *data, size_t length);

// Erase sector in QSPI Flash
qspi_flash_status_t qspi_flash_erase_sector(uint32_t address);

// Erase block in QSPI Flash
qspi_flash_status_t qspi_flash_erase_block(uint32_t address, size_t size);

// Erase entire QSPI Flash
qspi_flash_status_t qspi_flash_chip_erase(void);

// Check if QSPI Flash is busy
bool qspi_flash_is_busy(void);

// Get QSPI Flash status register
uint8_t qspi_flash_get_status(void);

// Set QSPI Flash XIPOFFSET
void qspi_flash_set_xip_offset(uint32_t offset);

// Configure W25Q16 for Quad mode (internal function)
static qspi_flash_status_t qspi_flash_configure_quad_mode(void);

#ifdef __cplusplus
}
#endif

#endif /* QSPI_FLASH_H_ */