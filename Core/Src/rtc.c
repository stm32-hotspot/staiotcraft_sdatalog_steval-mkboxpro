/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    rtc.c
  * @brief   This file provides code for the configuration
  *          of the RTC instances.
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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"

/* USER CODE BEGIN 0 */

#define RTC_SMOOTH_CALIB_MAX_MINUS_PULSES 511U

/* 1 pulse over a 32 s window is 1/(32768*32) s, i.e. about 0.953674316 ppm. */
static const float rtc_calib_step_ppm = 0.953674316f;

static float absf_local(float x)
{
  return (x < 0.0f) ? -x : x;
}

static void RTC_SelectBestSmoothCalibration(float correction_ppm, uint32_t *plus_pulses, uint32_t *minus_pulses)
{
  uint32_t best_plus = RTC_SMOOTHCALIB_PLUSPULSES_RESET;
  uint32_t best_minus = 0U;
  float best_err = 1.0e9f;

  for (uint32_t plus = 0U; plus <= 1U; plus++)
  {
    for (uint32_t minus = 0U; minus <= RTC_SMOOTH_CALIB_MAX_MINUS_PULSES; minus++)
    {
      float effective_ppm = ((plus ? 512.0f : 0.0f) - (float)minus) * rtc_calib_step_ppm;
      float err = absf_local(correction_ppm - effective_ppm);

      if (err < best_err)
      {
        best_err = err;
        best_plus = plus ? RTC_SMOOTHCALIB_PLUSPULSES_SET : RTC_SMOOTHCALIB_PLUSPULSES_RESET;
        best_minus = minus;
      }
    }
  }

  *plus_pulses = best_plus;
  *minus_pulses = best_minus;
}

HAL_StatusTypeDef RTC_ApplySmoothCalibrationPpm(float correction_ppm)
{
  uint32_t plus_pulses;
  uint32_t minus_pulses;

  RTC_SelectBestSmoothCalibration(correction_ppm, &plus_pulses, &minus_pulses);

  return HAL_RTCEx_SetSmoothCalib(&hrtc,
                                  RTC_SMOOTHCALIB_PERIOD_32SEC,
                                  plus_pulses,
                                  minus_pulses);
}

HAL_StatusTypeDef RTC_ApplySmoothCalibrationFromDrift(float rtc_minus_ref_seconds, uint32_t measurement_seconds)
{
  float correction_ppm;

  if (measurement_seconds == 0U)
  {
    return HAL_ERROR;
  }

  /* Positive drift means RTC is fast, so apply a negative correction. */
  correction_ppm = -(rtc_minus_ref_seconds * 1000000.0f) / (float)measurement_seconds;

  return RTC_ApplySmoothCalibrationPpm(correction_ppm);
}

/* USER CODE END 0 */

RTC_HandleTypeDef hrtc;

/* RTC init function */
void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_PrivilegeStateTypeDef privilegeState = {0};
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  hrtc.Init.BinMode = RTC_BINARY_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  privilegeState.rtcPrivilegeFull = RTC_PRIVILEGE_FULL_NO;
  privilegeState.backupRegisterPrivZone = RTC_PRIVILEGE_BKUP_ZONE_NONE;
  privilegeState.backupRegisterStartZone2 = RTC_BKP_DR0;
  privilegeState.backupRegisterStartZone3 = RTC_BKP_DR0;
  if (HAL_RTCEx_PrivilegeModeSet(&hrtc, &privilegeState) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0;
  sTime.Minutes = 0;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 1;
  sDate.Year = 0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{

  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspInit 0 */

  /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    /* RTC clock enable */
    __HAL_RCC_RTC_ENABLE();
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    __HAL_RCC_RTCAPB_CLKAM_ENABLE();
  /* USER CODE BEGIN RTC_MspInit 1 */

  /* USER CODE END RTC_MspInit 1 */
  }
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* rtcHandle)
{

  if(rtcHandle->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
    __HAL_RCC_RTCAPB_CLK_DISABLE();
    __HAL_RCC_RTCAPB_CLKAM_DISABLE();
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
