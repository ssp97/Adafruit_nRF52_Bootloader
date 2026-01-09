/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ha Thach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>
#include "nrf_sdm.h"
#include "flash_nrf5x.h"
#include "boards.h"
#include "usb/uf2/uf2cfg.h"

#ifdef ENABLE_QSPI_FLASH
#include "qspi_flash.h"
#endif

#define FLASH_PAGE_SIZE           4096
#define FLASH_CACHE_INVALID_ADDR  0xffffffff

static uint32_t _fl_addr = FLASH_CACHE_INVALID_ADDR;
static uint8_t _fl_buf[FLASH_PAGE_SIZE] __attribute__((aligned(4)));

#ifdef ENABLE_QSPI_FLASH
// Cache to track which QSPI sectors have been erased to avoid repeated erasures
static uint32_t _qspi_erased_sector = 0xFFFFFFFF; // Track last erased sector
#endif

void flash_nrf5x_flush (bool need_erase)
{
  if ( _fl_addr == FLASH_CACHE_INVALID_ADDR ) return;

  // skip the write if contents matches
  if ( memcmp(_fl_buf, (void *) _fl_addr, FLASH_PAGE_SIZE) != 0 )
  {
    // - nRF52832 dfu via uart can miss incoming byte when erasing because cpu is blocked for > 2ms.
    // Since dfu_prepare_func_app_erase() already erase the page for us, we can skip it here.
    // - nRF52840 dfu serial/uf2 are USB-based which are DMA and should have no problems.
    //
    // Note: MSC uf2 does not erase page in advance like dfu serial
    if ( need_erase )
    {
      PRINTF("Erase and ");
      nrfx_nvmc_page_erase(_fl_addr);
    }

    PRINTF("Write 0x%08lX\r\n", _fl_addr);
    nrfx_nvmc_words_write(_fl_addr, (uint32_t *) _fl_buf, FLASH_PAGE_SIZE / 4);
  }

  _fl_addr = FLASH_CACHE_INVALID_ADDR;
}

#ifdef ENABLE_QSPI_FLASH
// Reset the QSPI sector erase cache - useful when starting a new write operation
void flash_nrf5x_reset_qspi_erase_cache(void)
{
  _qspi_erased_sector = 0xFFFFFFFF;
}
#endif

void flash_nrf5x_write (uint32_t dst, void const *src, int len, bool need_erase)
{
  uint32_t newAddr = dst & ~(FLASH_PAGE_SIZE - 1);

#ifdef ENABLE_QSPI_FLASH
  // Check if address is in QSPI Flash range
  if (dst >= CFG_UF2_QSPI_XIP_OFFSET && dst < (CFG_UF2_QSPI_XIP_OFFSET + CFG_UF2_QSPI_FLASH_SIZE))
  {
    // Initialize QSPI Flash if not already done
    static bool qspi_initialized = false;
    if (!qspi_initialized)
    {
      if (qspi_flash_init() == QSPI_FLASH_STATUS_SUCCESS)
      {
        qspi_initialized = true;
        PRINTF("QSPI Flash initialized successfully\r\n");
      }
      else
      {
        PRINTF("Failed to initialize QSPI Flash\r\n");
        return;
      }
    }
    
    // For QSPI Flash, we need to erase the sector before writing if need_erase is true
    if (need_erase)
    {
      // Align address to sector boundary
      uint32_t sector_addr = (dst - CFG_UF2_QSPI_XIP_OFFSET) & ~(W25Q16_SECTOR_SIZE - 1);
      
      // Avoid repeated erasure of the same sector
      if (sector_addr != _qspi_erased_sector)
      {
        PRINTF("Erasing QSPI Flash sector at 0x%08lX\r\n", sector_addr);
        
        qspi_flash_status_t erase_status = qspi_flash_erase_sector(sector_addr);
        if (erase_status != QSPI_FLASH_STATUS_SUCCESS)
        {
          PRINTF("Failed to erase QSPI Flash sector: status=%d\r\n", erase_status);
          return;
        }
        
        // Update the cache to track the last erased sector
        _qspi_erased_sector = sector_addr;
      }
      else
      {
        PRINTF("Skipping erase of already erased sector at 0x%08lX\r\n", sector_addr);
      }
    }
    
    // Write to QSPI Flash
    qspi_flash_status_t status = qspi_flash_write(dst - CFG_UF2_QSPI_XIP_OFFSET, (uint8_t*)src, len);
    if (status != QSPI_FLASH_STATUS_SUCCESS)
    {
      PRINTF("Failed to write to QSPI Flash: status=%d\r\n", status);
    }
    return;
  }
#endif

  if ( newAddr != _fl_addr )
  {
    flash_nrf5x_flush(need_erase);
    _fl_addr = newAddr;
    memcpy(_fl_buf, (void *) newAddr, FLASH_PAGE_SIZE);
  }
  memcpy(_fl_buf + (dst & (FLASH_PAGE_SIZE - 1)), src, len);
}
