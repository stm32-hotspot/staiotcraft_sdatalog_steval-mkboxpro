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
/* Includes ------------------------------------------------------------------*/
#include "custom_motion_sensors.h"
#include "main.h"
#include "lsm6dsv16x_reg.h"
#include "stm32u5xx.h"
#include "lsm6dsv16x_gyro.h"
#include "simple_serial_tl.h"
#include "vdatalog_conf.h"
#include "vdatalog.h"
#include "timestamp.h"
#include "fsm.h"
#include "App_model_Lsm6dsv16x_Gyro.h"
#include "App_model_Lsm6dsv16x_Mlc.h"
#include "lsm6dsv16x_acc.h"
#include <stdbool.h>
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define VD_BUFFER_SIZE_GYRO   (ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE + TIMESTAMP_SIZE + SAMPLES_PER_TIME_STAMP_GYRO*BYTES_PER_SAMPLE_GYRO)
extern void *MotionCompObj[CUSTOM_MOTION_INSTANCES_NBR];
extern SimpleSerialTL_t *pSstl;
extern volatile double GYROTimeStamp;
extern uint8_t lsm6dsv16x_gyro_get_output_st_stream__channel_specification_stream_id(int32_t *value);
extern uint32_t led10_timer;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t txbuffer_s2[VD_BUFFER_SIZE_GYRO] __attribute__((aligned(4)));
/* allocate the FSM obj */
FSM GyroFsm;

/** Define the FSM state transitions LUT
     current FSM state,             Input event,        next FSM state,            FSM state action func **/
Transition GyroTransitions[] = {
    {FSM_STATE_DISABLED,       FSM_EVT_ENABLE,      FSM_STATE_ENABLED,         GyroDisabledStateActionEnable},

    {FSM_STATE_ENABLED,        FSM_EVT_DISABLE,     FSM_STATE_DISABLED,        GyroEnabledStateActionDisable},
    {FSM_STATE_ENABLED,        FSM_EVT_START_LOG,   FSM_STATE_LOGGING_DATA,    GyroEnabledStateActionStartLog},    

    {FSM_STATE_LOGGING_DATA,   FSM_EVT_INT,         FSM_STATE_LOGGING_DATA,    GyroLoggingDataStateActionInt},
    {FSM_STATE_LOGGING_DATA,   FSM_EVT_STOP_LOG,    FSM_STATE_ENABLED,         GyroLoggingDataStateActionStopLog},
};

unsigned int sizeof_Gyro_transitions = sizeof(GyroTransitions) / sizeof(Transition);

/* Private function prototypes -----------------------------------------------*/

void ReadGyro()
{
uint8_t Status=0;
LSM6DSV16X_AxesRaw_t Value;
  
  /* if acc is enabled it needs to check if the INT2 is from acc or gyro data ready */
  if (FsmCheckState(&AccFsm, FSM_STATE_LOGGING_DATA)) {
    LSM6DSV16X_GYRO_Get_DRDY_Status((LSM6DSV16X_Object_t *)MotionCompObj[SENSOR_0], &Status);
    if (Status == 0) return; /* if gyro data is not ready, it means is a acc data INT: return and wait for next INT */
  }

  //HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_SET);                    

    /* Get time stamp*/
  GYROTimeStamp = TS_VD_GetTimeStamp();  
    
    /* read the gyro axes */
  #ifndef DUMMY_DATA_GYRO    
    LSM6DSV16X_GYRO_GetAxesRaw((LSM6DSV16X_Object_t *)MotionCompObj[SENSOR_0], &Value);
  #else
    Value.x=rand() % 2000 - 1000; // generate random value between -1000 and 1000
    Value.y=rand() % 2000 - 1000; // generate random value between -1000 and 1000
    Value.z=rand() % 2000 - 1000; // generate random value between -1000 and 1000
  #endif
  #ifndef NO_PRINTF_USB
    printf("G: %d %d %d\r\n", Value.x, Value.y, Value.z);
  #endif
    /* prepare the ptr to msg data field */
  LSM6DSV16X_AxesRaw_t *p_tx_datard_sensorGYRO = (LSM6DSV16X_AxesRaw_t *)&txbuffer_s2[ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE + TIMESTAMP_SIZE];    

    /* fill msg data field */
	memcpy (p_tx_datard_sensorGYRO, &Value, sizeof(LSM6DSV16X_AxesRaw_t));

	  /* this works only in case of SAMPLES_PER_TIME_STAMP_QVAR = 1 otherwise data needs to be accumulated */
	  /* because of alignement constraint timestamp must be copied byte by byte */
    /*  fill the msg time stamp data field */
  for (size_t i = 0; i < sizeof(double); i++) {
        txbuffer_s2[ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE  + i] = ((uint8_t *)&GYROTimeStamp)[i];
  }
	  /* send "Datalog Payload" (ByteCounter(4) | QvarData(2) | TimeStamp(8)) to GUI */
  int32_t id;  
  lsm6dsv16x_gyro_get_output_st_stream__channel_specification_stream_id(&id); 

  if (SSTL_TxAsync(pSstl, (uint8_t)id, SSTL_ASYNC_PAYLOAD_TYPE_CUSTOM, &txbuffer_s2[ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE], TIMESTAMP_SIZE + SAMPLES_PER_TIME_STAMP_GYRO*BYTES_PER_SAMPLE_GYRO, SSTL_NON_BLOCKING) != SSTL_OK )
//    if (SSTL_TxAsync(pSstl, (uint8_t)id, SSTL_ASYNC_PAYLOAD_TYPE_CUSTOM, &txbuffer_s2[ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE], TIMESTAMP_SIZE + SAMPLES_PER_TIME_STAMP_GYRO*BYTES_PER_SAMPLE_GYRO, SSTL_BLOCKING) != SSTL_OK )
  {
    return;
  } 
// 	  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_RESET); 
}

void GyroDisabledStateActionEnable(Event event, void* args)
{
  (void)event;
  (void)args;
  CUSTOM_MOTION_SENSOR_Enable(SENSOR_0, MOTION_GYRO);  
}

void GyroEnabledStateActionDisable(Event event, void* args)
{
  (void)event;
  (void)args;
  bool value;
  lsm6dsv16x_mlc_get_enable(&value);
  if(!value) {
    /* if MLC is NOT enabled, set Gyro EN, ODR, FS to their default value */    
    CUSTOM_MOTION_SENSOR_Disable(SENSOR_0, MOTION_GYRO);
    lsm6dsv16x_gyro_set_odr( DEFAULT_GYRO_ODR_HZ);
    lsm6dsv16x_gyro_set_fs(DEFAULT_GYRO_FS_DPS);  
  }/* if MLC is enabled, keep the ODR and FS as they are because they can be shared between Gyro and MLC */
}

/* The activation function from off state */
void GyroEnabledStateActionStartLog(Event event, void* args)
{
  (void)event;
  (void)args;
  led10_timer = LED10_FAST_TIMER; /* change blue led timer to fast */

  MY_CUSTOM_MOTION_SENSOR_GYRO_Enable_DRDY_On_INT2 (SENSOR_0);
  
  /* first dummy read to unlatch INT */
  LSM6DSV16X_AxesRaw_t Value;  
  LSM6DSV16X_GYRO_GetAxesRaw((LSM6DSV16X_Object_t *)MotionCompObj[SENSOR_0], &Value);
}

void GyroLoggingDataStateActionInt(Event event, void* args)
{
  (void)event;
  (void)args;
  /* read Gyro data */
  	ReadGyro();
}

void GyroLoggingDataStateActionStopLog(Event event, void* args)
{
  (void)event;
  (void)args;
  /* slow the LED timer */
	led10_timer = LED10_SLOW_TIMER; /* change blue led blink timer to slow */

	MY_CUSTOM_MOTION_SENSOR_GYRO_Disable_DRDY_On_INT2 (SENSOR_0);

}
