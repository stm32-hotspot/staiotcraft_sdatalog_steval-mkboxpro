/**
  ******************************************************************************
  * @file           : lsm6dsv16x_qvar.c
  * @brief          : Application code for lsm6dsv16x_qvar
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
#include "main.h"
#include "lsm6dsv16x_reg.h"
#include "stm32u5xx.h"
#include "lsm6dsv16x_acc.h"
#include "simple_serial_tl.h"
#include "vdatalog_conf.h"
#include "vdatalog.h"
#include "timestamp.h"
#include "fsm.h"
#include "App_model_Lsm6dsv16x_Acc.h"
#include "App_model_Lsm6dsv16x_Mlc.h"
#include "lsm6dsv16x_gyro.h"
#include <stdbool.h>
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define VD_BUFFER_SIZE_ACC   (ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE + TIMESTAMP_SIZE + SAMPLES_PER_TIME_STAMP_ACC*BYTES_PER_SAMPLE_ACC)
extern void *MotionCompObj[CUSTOM_MOTION_INSTANCES_NBR];
extern SimpleSerialTL_t *pSstl;
extern volatile double ACCTimeStamp;
extern uint8_t lsm6dsv16x_acc_get_output_st_stream__channel_specification_stream_id(int32_t *value);
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t txbuffer_s1[VD_BUFFER_SIZE_ACC] __attribute__((aligned(4)));
/* allocate the FSM obj */
FSM AccFsm;

/** Define the FSM state transitions LUT
     current FSM state,             Input event,    next FSM state,                 FSM state action func **/
Transition AccTransitions[] = {
    {FSM_STATE_DISABLED,       FSM_EVT_ENABLE,    FSM_STATE_ENABLED,        AccDisabledStateActionEnable},

    {FSM_STATE_ENABLED,        FSM_EVT_START_LOG, FSM_STATE_LOGGING_DATA,   AccEnabledStateActionLogRun},    
    {FSM_STATE_ENABLED,        FSM_EVT_DISABLE,   FSM_STATE_DISABLED,       AccEnabledStateActionDisable},        

    {FSM_STATE_LOGGING_DATA,   FSM_EVT_INT,       FSM_STATE_LOGGING_DATA,   AccLoggingDataStateActionInt},
    {FSM_STATE_LOGGING_DATA,   FSM_EVT_STOP_LOG,  FSM_STATE_ENABLED,        AccLoggingDataStateActionLogStop},
};
unsigned int sizeof_Acc_transitions = sizeof(AccTransitions) / sizeof(Transition);

/* Private function prototypes -----------------------------------------------*/

void ReadAcc()
{
  uint8_t Status=0;
  LSM6DSV16X_AxesRaw_t Value = {0, 0, 0};
  //bool gyro_enabled;
  
  /* if gyro is logging it needs to check if the INT2 is from acc or gyro data ready */
  if (FsmCheckState(&GyroFsm, FSM_STATE_LOGGING_DATA)) {
    LSM6DSV16X_ACC_Get_DRDY_Status((LSM6DSV16X_Object_t *)MotionCompObj[SENSOR_0], &Status);
    if (Status == 0) return; /* if acc data is not ready, it means is a gyro data INT: return and wait for next INT */
  }

  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_SET);  
    /* get time stamp */ 
  ACCTimeStamp = TS_VD_GetTimeStamp();

  /* read the ACC axes */
  #ifndef DUMMY_DATA_ACC
    LSM6DSV16X_ACC_GetAxesRaw((LSM6DSV16X_Object_t *)MotionCompObj[SENSOR_0], &Value);
  #else
    Value.x=rand() % 2000 - 1000; // generate random value between -1000 and 1000
    Value.y=rand() % 2000 - 1000; // generate random value between -1000 and 1000
    Value.z=rand() % 2000 - 1000; // generate random value between -1000 and 1000
  #endif

  #ifndef NO_PRINTF_USB
    printf("A: %d %d %d\r\n", Value.x, Value.y, Value.z);
  #endif
    /* prepare the ptr to msg data field */
  LSM6DSV16X_AxesRaw_t *p_tx_datard_sensorACC = (LSM6DSV16X_AxesRaw_t *)&txbuffer_s1[ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE + TIMESTAMP_SIZE];    

    /* fill msg data field */
	memcpy (p_tx_datard_sensorACC, &Value, sizeof(LSM6DSV16X_AxesRaw_t));

	  /* this works only in case of SAMPLES_PER_TIME_STAMP_QVAR = 1 otherwise data needs to be accumulated */
	  /* because of alignement constraint timestamp must be copied byte by byte */
    /*  fill the msg time stamp data field */
  for (size_t i = 0; i < sizeof(double); i++) {
      txbuffer_s1[ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE + i] = ((uint8_t *)&ACCTimeStamp)[i];
  }
	/* send "Datalog Payload" (TimeStamp(8) | AccData(2*3) | ) to GUI */
    
  int32_t id;
  lsm6dsv16x_acc_get_output_st_stream__channel_specification_stream_id(&id);
//    if (SSTL_TxAsync(pSstl, (uint8_t)id, SSTL_ASYNC_PAYLOAD_TYPE_CUSTOM, &txbuffer_s1[ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE], TIMESTAMP_SIZE + SAMPLES_PER_TIME_STAMP_ACC*BYTES_PER_SAMPLE_ACC, SSTL_BLOCKING) != SSTL_OK )
  if (SSTL_TxAsync(pSstl, (uint8_t)id, SSTL_ASYNC_PAYLOAD_TYPE_CUSTOM, &txbuffer_s1[ASPEP_HEADER_SIZE + SSTL_HEADER_SIZE], TIMESTAMP_SIZE + SAMPLES_PER_TIME_STAMP_ACC*BYTES_PER_SAMPLE_ACC, SSTL_NON_BLOCKING/*SSTL_BLOCKING*/) != SSTL_OK )
  {
	  return;
  }
#ifndef NO_PRINTF_USB  
//  printf("ACC Data: X=%d, Y=%d, Z=%d", Value.x, Value.y, Value.z);
//  printf("\r\n");
#endif
  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_RESET);  
}

extern uint32_t led10_timer;

void AccDisabledStateActionEnable(Event event, void* args)
{
  (void)event;
  (void)args;
  CUSTOM_MOTION_SENSOR_Enable(SENSOR_0, MOTION_ACCELERO);  
}

void AccEnabledStateActionDisable(Event event, void* args)
{
  (void)event;
  (void)args;
  /* set Acc EN, ODR, FS to dft */
  bool value;
  lsm6dsv16x_mlc_get_enable(&value);
  if (!value) {
    /* if MLC is NOT enabled, set ODR and FS to their default value */    
    CUSTOM_MOTION_SENSOR_Disable(SENSOR_0, MOTION_ACCELERO);
    lsm6dsv16x_acc_set_odr( DEFAULT_ACC_ODR_HZ);
    lsm6dsv16x_acc_set_fs(DEFAULT_ACC_FS_G);
  } /* if MLC is enabled, keep the ODR and FS as they are because they can be shared between Acc and MLC */
}

/* The activation function from enabled state */
void AccEnabledStateActionLogRun(Event event, void* args)
{
  (void)event;
  (void)args;
	led10_timer = LED10_FAST_TIMER; /* change blue led timer to fast */

  MY_CUSTOM_MOTION_SENSOR_ACC_Enable_DRDY_On_INT2 (SENSOR_0);

  /* retrieve the sensor device context */
//  stmdev_ctx_t *dev_ctx = &(((LSM6DSV16X_Object_t *)MotionCompObj[SENSOR_0])->Ctx);
  /* first dummy read to unlatch INT */
  LSM6DSV16X_AxesRaw_t Value;  
  LSM6DSV16X_ACC_GetAxesRaw((LSM6DSV16X_Object_t *)MotionCompObj[SENSOR_0], &Value);
}

void AccLoggingDataStateActionInt(Event event, void* args)
{
  (void)event;
  (void)args;
	/* read Accelero data */
	  ReadAcc();
}

void AccLoggingDataStateActionLogStop(Event event, void* args)
{
  (void)event;
  (void)args;
	/* slow the LED timer */
	led10_timer = LED10_SLOW_TIMER; /* change blue led blink timer to slow */

	MY_CUSTOM_MOTION_SENSOR_ACC_Disable_DRDY_On_INT2 (SENSOR_0);
}
