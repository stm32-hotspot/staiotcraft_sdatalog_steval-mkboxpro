/**
  ******************************************************************************
  * @file           : lsm6dsv16x_gyro.c
  * @brief          : Application code for lsm6dsv16x_gyro
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LSM6DSV16X_GYRO_H
#define __LSM6DSV16X_GYRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "custom_motion_sensors.h"
#include "custom_mems_conf.h"
#include "MEMS_integration.h"
#include "fsm.h"

/* Private includes ----------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
extern FSM 			    GyroFsm;
extern Event 		    GyroEvent;
extern Event 		    GyroEventInt;
extern Transition 	GyroTransitions[];
extern unsigned int sizeof_Gyro_transitions;

/* User Defined QVAR specific FSM event handling functions */
extern void GyroDisabledStateActionEnable(Event event, void* args);

extern void GyroEnabledStateActionDisable(Event event, void* args);

extern void GyroEnabledStateActionStartLog(Event event, void* args);

extern void GyroLoggingDataStateActionInt(Event event, void* args);

extern void GyroLoggingDataStateActionStopLog(Event event, void* args);
/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __LSM6DSV16X_GYRO_H */
