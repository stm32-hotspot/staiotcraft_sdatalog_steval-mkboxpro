/**
  ******************************************************************************
  * @file    finish_good_hal.c
  * @brief   Finish-good detection implementation using HAL I2C2 and ST25DV APIs.
  ******************************************************************************
  */

#include "finish_good_hal.h"

#include "i2c.h"
#include "st25dv.h"
#include "PnPLCompManager_conf.h"
#include <string.h>

static ST25DV_Object_t g_st25_obj;

static int32_t FG_I2C2_Init(void)
{
  return (NFC_I2C2_Init() == HAL_OK) ? NFCTAG_OK : NFCTAG_ERROR;
}

static int32_t FG_I2C2_DeInit(void)
{
  NFC_I2C2_DeInit();
  return NFCTAG_OK;
}

static int32_t FG_I2C2_IsReady(uint16_t dev_addr, const uint32_t trials)
{
  return (HAL_I2C_IsDeviceReady(&hi2c2, dev_addr, trials, ST25_I2C_POLL_TIMEOUT) == HAL_OK) ? NFCTAG_OK : NFCTAG_ERROR;
}

static int32_t FG_I2C2_Write(uint16_t dev_addr, uint16_t reg_addr, const uint8_t *data, uint16_t length)
{
  HAL_StatusTypeDef status;

  status = HAL_I2C_Mem_Write(&hi2c2, dev_addr, reg_addr, I2C_MEMADD_SIZE_16BIT, (uint8_t *)data, length, ST25_I2C_POLL_TIMEOUT);
  return (status == HAL_OK) ? NFCTAG_OK : NFCTAG_ERROR;
}

static int32_t FG_I2C2_Read(uint16_t dev_addr, uint16_t reg_addr, uint8_t *data, uint16_t length)
{
  HAL_StatusTypeDef status;

  status = HAL_I2C_Mem_Read(&hi2c2, dev_addr, reg_addr, I2C_MEMADD_SIZE_16BIT, data, length, ST25_I2C_POLL_TIMEOUT);
  return (status == HAL_OK) ? NFCTAG_OK : NFCTAG_ERROR;
}

static uint32_t FG_GetTick(void)
{
  return HAL_GetTick();
}

HAL_StatusTypeDef NFC_I2C2_Init(void)
{
  MX_I2C2_Init();
  return HAL_OK;
}

void NFC_I2C2_DeInit(void)
{
  MX_I2C2_DeInit();
}

HAL_StatusTypeDef NFC_I2C2_WaitFlag(uint32_t flag_mask, uint32_t timeout_ms)
{
  uint32_t tick_start = HAL_GetTick();

  (void)flag_mask;

  while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY)
  {
    if ((HAL_GetTick() - tick_start) >= timeout_ms)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef NFC_I2C2_ReadReg16(uint16_t dev_addr, uint16_t reg_addr, uint8_t *data, uint16_t length)
{
  if ((data == NULL) || (length == 0U))
  {
    return HAL_ERROR;
  }

  if (HAL_I2C_Mem_Read(&hi2c2, dev_addr, reg_addr, I2C_MEMADD_SIZE_16BIT, data, length, ST25_I2C_POLL_TIMEOUT) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

uint8_t get_fw_id_from_finish_good(ai_type_t ai_type, FinishGood_TypeDef finish_good)
{
  if (finish_good == FINISHB)
  {
    return FW_ID_C;
  }
  return FW_ID_A;
}

FinishGood_TypeDef BSP_CheckFinishGood(void)
{
  FinishGood_TypeDef detected_finish_good = FINISH_ERROR;
  uint8_t nfctag_id = 0U;
  ST25DV_IO_t io_ctx;

  memset(&g_st25_obj, 0, sizeof(g_st25_obj));

  io_ctx.Init = FG_I2C2_Init;
  io_ctx.DeInit = FG_I2C2_DeInit;
  io_ctx.IsReady = FG_I2C2_IsReady;
  io_ctx.Write = FG_I2C2_Write;
  io_ctx.Read = FG_I2C2_Read;
  io_ctx.GetTick = FG_GetTick;

  if (ST25DV_RegisterBusIO(&g_st25_obj, &io_ctx) != NFCTAG_OK)
  {
    return FINISH_ERROR;
  }

  if (St25Dv_Drv.ReadID(&g_st25_obj, &nfctag_id) == NFCTAG_OK)
  {
    if ((nfctag_id == IAM_ST25DV04KC) || (nfctag_id == IAM_ST25DV64KC))
    {
      detected_finish_good = FINISHB;
    }
    else if ((nfctag_id == IAM_ST25DV04) || (nfctag_id == IAM_ST25DV64))
    {
      detected_finish_good = FINISHA;
    }
  }

  NFC_I2C2_DeInit();

  return detected_finish_good;
}
