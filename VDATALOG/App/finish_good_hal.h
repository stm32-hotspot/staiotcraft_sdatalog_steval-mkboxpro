/**
  ******************************************************************************
  * @file    finish_good_hal.h
  * @brief   Finish-good detection APIs based on HAL I2C2 and ST25DV driver APIs.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#ifndef __FINISH_GOOD_HAL_H
#define __FINISH_GOOD_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define ST25_ADDR_DATA_I2C                  ((uint16_t)0xAE)
#define ST25_ICREF_REG                      ((uint16_t)0x0017)
#define ST25_I2C_POLL_TIMEOUT               (1000U)

#define IAM_ST25DV04KC                      (0x50U)
#define IAM_ST25DV64KC                      (0x51U)
#define IAM_ST25DV04                        (0x24U)
#define IAM_ST25DV64                        (0x26U)

typedef enum {
  FINISHA = 0,
  FINISHB = 1,
  FINISH_ERROR = 2
} FinishGood_TypeDef;

typedef enum {
  MCU = 0,
  LSM6DSV16X_MLC = 1,
  AI_TYPE_UNKNOWN = 2
} ai_type_t;

HAL_StatusTypeDef NFC_I2C2_Init(void);
void NFC_I2C2_DeInit(void);
HAL_StatusTypeDef NFC_I2C2_ReadReg16(uint16_t dev_addr, uint16_t reg_addr, uint8_t *data, uint16_t length);
HAL_StatusTypeDef NFC_I2C2_WaitFlag(uint32_t flag_mask, uint32_t timeout_ms);
uint8_t get_fw_id_from_finish_good(ai_type_t ai_type, FinishGood_TypeDef finish_good);
FinishGood_TypeDef BSP_CheckFinishGood(void);

#ifdef __cplusplus
}
#endif

#endif /* __FINISH_GOOD_HAL_H */
