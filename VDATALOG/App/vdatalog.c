/**
 ******************************************************************************
 * @file    vdatalog.c
 * @brief   Platform independent application code for Vanilla Datalog
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
#include "vdatalog.h"
#include "fsm.h"
#include "vdatalog_conf.h"
#include "App_model_Lsm6dsv16x_Mlc.h"
#include "custom_motion_sensors.h"
#include "custom_motion_sensors_ex.h"
#include "lsm6dsv16x_reg.h"
#include "main.h"
#include "stdlib.h"
#include "simple_serial_tl.h"
#include "PnPL_init.h"
#include "PnPLCompManager.h"
#include "PnPLCompManager_conf.h"
#include "finish_good_hal.h"
#include "stm32u5xx_hal_cortex.h"
#include "timestamp.h"
#include "App_model.h"
#include "lsm6dsv16x_acc.h"
#include "lsm6dsv16x_gyro.h"
#include "lsm6dsv16x_mlc_app.h"
#include "flash_memory.h"
#include "lsm6dsv16x_mlc.h"
#include "ntp_client.h"
#include "rtc.h"
#ifndef NO_PRINTF_USB
#include "usbd_cdc_if.h"
#endif
#include <stdbool.h>
#include <stdio.h>

#ifdef NO_VDATALOG_PERIODIC_RTC_TASKS
#define VDATALOG_PERIODIC_RTC_TASKS_ENABLED 0
#define VDATALOG_PERIODIC_RTC_CALIB_ENABLED 0
#define VDATALOG_PERIODIC_RTC_NTP_ENABLED 0
#else
#define VDATALOG_PERIODIC_RTC_TASKS_ENABLED (RTC_PERIODIC_CALIB_ENABLE || RTC_NTP_DRIFT_DYNAMIC_ENABLE)
#define VDATALOG_PERIODIC_RTC_CALIB_ENABLED RTC_PERIODIC_CALIB_ENABLE
#define VDATALOG_PERIODIC_RTC_NTP_ENABLED RTC_NTP_DRIFT_DYNAMIC_ENABLE
#endif

typedef struct
{
  uint32_t rtc_epoch_s;
  uint32_t ntp_epoch_s;
} RTC_NtpSyncPoint_t;

/* Private typedef -----------------------------------------------------------*/
extern bool is_MLC_configuration_in_flash(void);
extern void MLC_init_flash_memory(void);

/* Private define ------------------------------------------------------------*/
stmdev_ctx_t dev_ctx;
/* Private variables ---------------------------------------------------------*/
static uint32_t led1_start_time 	= 0;
static uint32_t led10_start_time  	= 0;
uint32_t 		led10_timer  		= LED10_SLOW_TIMER;

/* Communication protocol */
SimpleSerialTL_t *pSstl;
volatile double ACCTimeStamp;
volatile double GYROTimeStamp;

volatile bool command_received;                        /* Flag when command is received from USB. */
uint32_t command_buffer_size;                         /* Size of the command received from USB. */
char command_buffer_static[APP_RX_DATA_SIZE];         /* Buffer containing the data received from USB. */
char *command_buffer_ptr;                              /* Pointer to the beginning of the command buffer. */
char *command_buffer_write_ptr;                        /* Pointer to the current write position in the command buffer. */

#ifndef NO_USB_PRINTF 
extern PCD_HandleTypeDef  hpcd_USB_OTG_FS;
#endif
extern UART_HandleTypeDef huart4;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim3;

#ifndef NO_PRINTF_USB
/* printf redirection over USB CDC */
int _write(int file, char *ptr, int len)
{
    CDC_Transmit_FS((uint8_t*)ptr, len);
    return len;
}
#endif

#if VDATALOG_PERIODIC_RTC_TASKS_ENABLED
static void RTC_PeriodicCalibrationInit(void);
#endif
#if VDATALOG_PERIODIC_RTC_CALIB_ENABLED
static void RTC_PeriodicCalibrationTask(void);
#endif
#if VDATALOG_PERIODIC_RTC_NTP_ENABLED
static void NTP_PeriodicSyncTask(void);
#endif

__attribute__((weak)) int32_t NTP_Backend_UdpSend(void *user_ctx,
                                                   const char *server_name,
                                                   uint16_t server_port,
                                                   const uint8_t *payload,
                                                   uint16_t payload_len)
{
  (void)user_ctx;
  (void)server_name;
  (void)server_port;
  (void)payload;
  (void)payload_len;
  return -1;
}

__attribute__((weak)) int32_t NTP_Backend_UdpRecv(void *user_ctx,
                                                   uint8_t *payload,
                                                   uint16_t payload_max_len,
                                                   uint32_t timeout_ms)
{
  (void)user_ctx;
  (void)payload;
  (void)payload_max_len;
  (void)timeout_ms;
  return -1;
}

#if VDATALOG_PERIODIC_RTC_TASKS_ENABLED
static uint32_t 			    rtc_calib_last_ms; 
static HAL_StatusTypeDef 	rtc_calib_last_status;
static uint32_t ntp_sync_last_ms;
static bool ntp_client_initialized;
static NTP_Client_t ntp_client;
static RTC_NtpSyncPoint_t rtc_ntp_points[RTC_NTP_SAMPLE_COUNT];
static uint32_t rtc_ntp_last_sync_ms;
static uint8_t rtc_ntp_count;
static uint8_t rtc_ntp_head;
static bool rtc_ntp_anchor_valid;
static uint32_t rtc_ntp_anchor_rtc_s;
static uint32_t rtc_ntp_anchor_ntp_s;
static bool rtc_ntp_calibration_ready;
static float rtc_ntp_drift_seconds;
static uint32_t rtc_ntp_measurement_seconds;
#endif

#if VDATALOG_PERIODIC_RTC_NTP_ENABLED
static bool RTC_GetEpochSeconds(uint32_t *epoch_seconds);
static bool RTC_IsLeapYear(uint32_t year);
static uint32_t RTC_DaysBeforeMonth(uint32_t year, uint32_t month);
#endif


/**
 * @brief  Main function for Vanilla datalog application
 *         This function will never return
 * @param  handle: pointer to the HW interface for sensors
 */
void vdatalog_init(void *handle)
{
  (void)handle;

  /* Timestamp Timer initialization */
  TS_TIM_VD_Init();
  TS_TIM_MLC_Init();

  /* Serial Protocol (SSTL) initialization
   * Incoming configuration packets (PnPL) are managed in ProtocolScheduler(void)
   **/
  pSstl = SSTL_Alloc();
  SSTL_Init(pSstl);

  /* Frame format on UART
   * Max length of Datalog Payload buffer is defined by SSTL_MAX_ASYNC_PAYLOAD macro
   * and it depends on the ASPEP capabilities.
   * +--------------+-------------+------------------------------------+
   * | ASPEP Header | SSTL Header |       Datalog Payload buffer       |
   * +--------------+-------------+------------------------------------+
   * |    4 Bytes   |   4 Bytes   |               N Bytes              |
   * +--------------+-------------+------------------------------------+
   **/

   
  /* Initialize custom motion sensors abstraction layer */
  CUSTOM_MOTION_SENSOR_Init(SENSOR_0, MOTION_ACCELERO | MOTION_GYRO);

  /* reset sensor registrs to dft values */
  MY_CUSTOM_MOTION_SENSOR_Reset (SENSOR_0);

  /* set the default INT data ready mode, FIXME consider it can be changed at runtime by the MLC program */
  #ifdef DATA_READY_PULSED
  uint8_t Data;
  CUSTOM_MOTION_SENSOR_Read_Register(SENSOR_0, LSM6DSV16X_CTRL4, &Data);
  Data = Data | 0x02; /* set drdy pulsed */
  CUSTOM_MOTION_SENSOR_Write_Register(SENSOR_0, LSM6DSV16X_CTRL4, Data);
  #else
  uint8_t Data;
  CUSTOM_MOTION_SENSOR_Read_Register(SENSOR_0, LSM6DSV16X_CTRL4, &Data);
  Data = Data & ~0x02; /* set drdy latched */
  CUSTOM_MOTION_SENSOR_Write_Register(SENSOR_0, LSM6DSV16X_CTRL4, Data);
  #endif
 
  /* PnPL components initialization */
  PnPLSetAllocationFunctions(malloc, free);
  json_set_escape_slashes(0);
  PnPL_Components_Alloc();
  PnPL_Components_Init();

  {
    FinishGood_TypeDef finish_good = BSP_CheckFinishGood();

    PnPLSetBOARDID(finish_good == FINISHB ? BOARD_ID_C : BOARD_ID_A);
    PnPLSetFWID(get_fw_id_from_finish_good(LSM6DSV16X_MLC, finish_good));
  }

  /* Sensors FSMs initialization*/
  FsmInit(&AccFsm,  FSM_STATE_DISABLED, AccTransitions,  sizeof_Acc_transitions,  EXTI11_IRQn);    // Acc INT2
  FsmInit(&GyroFsm, FSM_STATE_DISABLED, GyroTransitions, sizeof_Gyro_transitions, EXTI11_IRQn);    // Gyro INT2
  FsmInit(&MlcFsm,  FSM_STATE_DISABLED, MlcTransitions,  sizeof_Mlc_transitions,  EXTI4_IRQn);     // MLC INT1
#if 0  
/* this code is to load MLC configuration from rom array at startup just for debugging */
#ifndef NO_FLASH_MEM  
  /* check if MLC configuration is already loaded in flash */
  if (!is_MLC_configuration_in_flash())
  {
    /* init in flash MLC magic number, fname and model file with default one */
    MLC_init_flash_memory();
  }
#endif
#endif

	memset((char *) command_buffer_static, 0, APP_RX_DATA_SIZE);
 	command_buffer_ptr = &command_buffer_static[0];
	command_buffer_write_ptr = &command_buffer_static[0];
	command_buffer_size = 0;
	command_received = false;

#ifndef NO_PRINTF_USB 
  /* USB Device initialization */
	MX_USB_Device_Init();

	/* USB initialization timeout (required to stabilize the USB peripheral voltage and to open the terminal). */
	HAL_Delay(500);

        printf ("Vanilla Datalog initialized.\r\n"); // printf over USB CDC
#endif

// for debug purposes uncomment to enable by default acc, gyro or mlc
//      lsm6dsv16x_acc_set_enable(true);  // uncomment to enable ACC by default 
//      lsm6dsv16x_acc_set_odr(pnpl_lsm6dsv16x_acc_odr_hz3840);
//      lsm6dsv16x_gyro_set_enable(false);  // uncomment to enable gyro by default
//      lsm6dsv16x_mlc_set_enable(true);  // uncomment to enable mlc by default      
//      load_lsm6dsv16x_mlc_configuration(lsm6dsv16x_mlc_conf_0, MEMS_CONF_ARRAY_LEN(lsm6dsv16x_mlc_conf_0)); // uncomment to load dft mlc program at startup
//      log_controller_start_log(0);  // uncomment start logging at startup by default

#if VDATALOG_PERIODIC_RTC_TASKS_ENABLED
  RTC_PeriodicCalibrationInit();
#endif
}

void vdatalog_main()
{
  /* Infinite loop */
  while(1)
  {
	/* Run the Protocol Scheduler to manage incoming messages */
    ProtocolScheduler();

    /* handle Start/Stop Log evts from PnPL cmds and from INT */
	  FsmHandleEvent(&AccFsm);
	  FsmHandleEvent(&GyroFsm);
    FsmHandleEvent(&MlcFsm);

#if VDATALOG_PERIODIC_RTC_NTP_ENABLED
  NTP_PeriodicSyncTask();
#endif
#if VDATALOG_PERIODIC_RTC_CALIB_ENABLED
  RTC_PeriodicCalibrationTask();
#endif

#ifndef NO_PRINTF_USB
  /* dummy test of USB CDC COM sending back echo of each received data terminated with \CR\LF */
  	if (command_received) {         /* Check if command is received. */
 	  	command_received = false;
	  	printf("%s", command_buffer_ptr);               /* Send back on USB the command received. */
        printf ("\r\n");  /* send Carriage Return and Line Feed to terminate the line on terminal */
		/* Resetting command buffer. */
		command_buffer_size = command_buffer_write_ptr - command_buffer_ptr;    /* Size of the last command received. */
		memset((char *) command_buffer_ptr, 0, command_buffer_size);            /* Resetting the command buffer. */
		command_buffer_write_ptr = command_buffer_ptr;                          /* Move the write pointer to point to the initial position of the command buffer. */
		command_buffer_size = 0; 
	  }
#endif
    /* Go to sleep. The system wakes up on any IRQ (Systick as well) */
//    __WFI();
  }
}

/**
 * @brief  ProtocolScheduler
 *
 * In bare metal applications this function should be called periodically to check PnPL incoming msgs.
 * In an event driven application with a RTOS it could be invoked in a task when a new packet is received.
 */
void ProtocolScheduler(void)
{
  static char *SerializedResponse;
  uint32_t rx_status;

  if (pSstl == NULL)
  {
    return;
  }
  /* Green LED flash once at GUI connection */
  if (HAL_GetTick() - led1_start_time >= LED1_TIMER)
  {
	  led1_start_time = 0;
	  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
  }
  /* Blue LED blinking slow indicate prog is running */
  if (HAL_GetTick() - led10_start_time >= led10_timer)
  {
	  led10_start_time = HAL_GetTick();
      HAL_GPIO_TogglePin(USB_ENUM_LED_GPIO_Port, USB_ENUM_LED_Pin);
  }

  rx_status = SSTL_ProcessRxFrame(pSstl, SSTL_NON_BLOCKING);
  if (rx_status == SSTL_NEW_PACKET_AVAILABLE)
  {
    uint8_t *p_rx_packet = NULL;
    uint32_t size = 0;
    if (SSTL_GetRxPacket(pSstl, &p_rx_packet, &size) == SSTL_OK)
    {
      /* PnPLCommand object declaration */
      PnPLCommand_t PnPLCommand;

      /* Parse received input message. (received_msg is the received input message to parse)
       * PnPLCommand object is filled by the PnPLParseCommand function.
       */
      PnPLParseCommand((char *) p_rx_packet, &PnPLCommand);

      /* Check PnPLCommand type:
       * - If it is a GET Message, generate a response with complete or partial status
       * - PNPL_CMD_SYSTEM_INFO is for board identification only (FW_ID, BOARD_ID)
       */
      if (PnPLCommand.comm_type == PNPL_CMD_GET || PnPLCommand.comm_type == PNPL_CMD_SYSTEM_INFO)
      {
        /* Declare Serialized JSON response message and size */
        uint32_t size;

        /* Serialize message response */
        PnPLSerializeResponse(&PnPLCommand, &SerializedResponse, &size, 0);

        /* Set buffer that needs to be sent and its size */
        SSTL_SetResponse(pSstl, (uint8_t *) SerializedResponse, size);

        /* Start the transmission of the buffer that was previously set */
        if (SSTL_TxResponse(pSstl, SSTL_BLOCKING) != SSTL_PARTIAL_PACKET)
        {
          /* Free the json response unless the packet was segmented */
          pnpl_free(SerializedResponse);
          SerializedResponse = NULL;
        } 
      }
      else if(PnPLCommand.comm_type == PNPL_CMD_SET || PnPLCommand.comm_type == PNPL_CMD_COMMAND)
      {
          SSTL_TxAck(pSstl, SSTL_BLOCKING);
      }
    }
  }
  else if (rx_status == SSTL_ACK_RECEIVED) /* An ACK was received. */
  {
    /* Call again SSTL_SetResponse to send another frame of the segmented packet */
    if (SSTL_TxResponse(pSstl, SSTL_BLOCKING) != SSTL_PARTIAL_PACKET)
    {
      pnpl_free(SerializedResponse);
      SerializedResponse = NULL;
    }
  }
  else if (rx_status == SSTL_PARTIAL_PACKET)
  {
    // Send ACK to host for each partial packet received
    SSTL_TxAck(pSstl, SSTL_BLOCKING);
  }    
}

/**
 * @brief  EXTI line detection callback.
 * @param  GPIO_Pin Specifies the port pin connected to corresponding EXTI line.
 * @retval None
 */
#if defined(STM32U5) || defined(STM32H5) || defined(STM32U0)
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
#elif defined(STM32F4) || defined(STM32L4) || defined(STM32G4)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
#else
#warning "Add support for the current MCU family"
#endif
{
  switch(GPIO_Pin)
  {
/* INT1 carry the MLC  */
    case VD_SENSOR_INT1_PIN:
    {
//   	  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_SET);
      FsmSetEventFromINT(&MlcFsm, FSM_EVT_INT);  
//      HAL_GPIO_WritePin(GPIOG, GPIO_PIN_3, GPIO_PIN_RESET);    
      break;
    }
/* INT2 carry the Acc and Gyro int */
    case VD_SENSOR_INT2_PIN:
    { 
 	  HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_SET);      
      FsmSetEventFromINT(&AccFsm, FSM_EVT_INT);            
 	    FsmSetEventFromINT(&GyroFsm, FSM_EVT_INT);      // Gyro INT on same pin
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_RESET);          
    	break;
    }
  }
}


void jump_to_bootloader( void )
{
  HAL_NVIC_DisableIRQ(EXTI4_IRQn);
  HAL_NVIC_DisableIRQ(EXTI11_IRQn);

  /* deinitialize peripherals */
#ifndef NO_USB_PRINTF   
  HAL_PCD_MspDeInit(&hpcd_USB_OTG_FS);
#endif
#ifndef NO_TIM3 
  HAL_TIM_Base_MspDeInit(&VD_TIMESTAMP_TIM);
#endif
#ifndef NO_TIM1
  HAL_TIM_Base_MspDeInit(&MLC_POLLING_TIM);
#endif
  HAL_UART_MspDeInit(&huart4);


	/*  Disable ICACHE */
	HAL_ICACHE_DeInit();

	/* Jump to user application */
	typedef  void (*pFunction)(void);
	pFunction JumpToApplication;
	uint32_t JumpAddress;
	JumpAddress = *(__IO uint32_t *) (0x0BF90000 + 4);
	JumpToApplication = (pFunction) JumpAddress;

	/* Initialize user application's Stack Pointer */
	__set_MSP(*(__IO uint32_t *) 0x0BF90000);
	JumpToApplication();

}

void ToggleFlashBank(void)
{
  FLASH_OBProgramInitTypeDef    OBInit = {0};
  OBInit.OptionType = OPTIONBYTE_USER;
	/* Set BFB2 bit to enable boot from Flash Bank2 */
	/* Allow Access to Flash control registers and user Flash */
	HAL_FLASH_Unlock();

	/* Allow Access to option bytes sector */
	HAL_FLASH_OB_Unlock();

	/* Get the Dual boot configuration status */
	HAL_FLASHEx_OBGetConfig(&OBInit);

	/* Enable/Disable dual boot feature */
	OBInit.OptionType = OPTIONBYTE_USER;
	OBInit.USERType   = OB_USER_SWAP_BANK;

	if (((OBInit.USERConfig) & (FLASH_OPTR_SWAP_BANK)) == FLASH_OPTR_SWAP_BANK)
	{
		OBInit.USERConfig &= ~FLASH_OPTR_SWAP_BANK;
	}
	else
	{
		OBInit.USERConfig = FLASH_OPTR_SWAP_BANK;
	}

	/* SYS_DEBUGF(SYS_DBG_LEVEL_WARNING, ("HW: Switching Bank\r\n")); */
	if(HAL_FLASHEx_OBProgram (&OBInit) != HAL_OK)
	{
		/*
	    Error occurred while setting option bytes configuration.
	    User can add here some code to deal with this error.
	    To know the code error, user can call function 'HAL_FLASH_GetError()'
		 */
		while(1);
	}

	/* Start the Option Bytes programming process */
	if (HAL_FLASH_OB_Launch() != HAL_OK) {
		/*
	    Error occurred while reloading option bytes configuration.
	    User can add here some code to deal with this error.
	    To know the code error, user can call function 'HAL_FLASH_GetError()'
		 */
		while(1);
	}
	HAL_FLASH_OB_Lock();
	HAL_FLASH_Lock();
}


#if VDATALOG_PERIODIC_RTC_TASKS_ENABLED
static void RTC_PeriodicCalibrationInit(void)
{
  rtc_calib_last_ms = HAL_GetTick();
  rtc_calib_last_status = HAL_OK;
  ntp_sync_last_ms = 0U;
  ntp_client_initialized = false;
  rtc_ntp_last_sync_ms = 0U;
  rtc_ntp_count = 0U;
  rtc_ntp_head = 0U;
  rtc_ntp_anchor_valid = false;
  rtc_ntp_anchor_rtc_s = 0U;
  rtc_ntp_anchor_ntp_s = 0U;
  rtc_ntp_calibration_ready = false;
  rtc_ntp_drift_seconds = 0.0f;
  rtc_ntp_measurement_seconds = 0U;
}
#endif


#if VDATALOG_PERIODIC_RTC_NTP_ENABLED
static void NTP_PeriodicSyncTask(void)
{
  uint32_t now_ms = HAL_GetTick();

  if ((ntp_sync_last_ms != 0U) &&
      ((uint32_t)(now_ms - ntp_sync_last_ms) < RTC_NTP_SYNC_MIN_PERIOD_MS))
  {
    return;
  }

  if (!ntp_client_initialized)
  {
    NTP_ClientConfig_t cfg;
    NTP_ClientIo_t io;

    cfg.server_name = "pool.ntp.org";
    cfg.server_port = NTP_CLIENT_DEFAULT_PORT;
    cfg.timeout_ms = NTP_CLIENT_DEFAULT_TIMEOUT_MS;

    io.udp_send = NTP_Backend_UdpSend;
    io.udp_recv = NTP_Backend_UdpRecv;
    io.user_ctx = NULL;

    if (NTP_Client_Init(&ntp_client, &cfg, &io) != NTP_CLIENT_STATUS_OK)
    {
      ntp_sync_last_ms = now_ms;
      return;
    }

    ntp_client_initialized = true;
  }

  {
    uint32_t unix_time_s;
    if (NTP_Client_QueryUnixTime(&ntp_client, &unix_time_s, NULL) == NTP_CLIENT_STATUS_OK)
    {
      vdatalog_rtc_ntp_sync_update(unix_time_s);
    }
  }

  ntp_sync_last_ms = now_ms;
}
#endif


#if VDATALOG_PERIODIC_RTC_CALIB_ENABLED
static void RTC_PeriodicCalibrationTask(void)
{
  uint32_t now_ms = HAL_GetTick();

  if ((uint32_t)(now_ms - rtc_calib_last_ms) >= RTC_PERIODIC_CALIB_INTERVAL_MS)
  {
#if VDATALOG_PERIODIC_RTC_NTP_ENABLED
    if (rtc_ntp_calibration_ready)
    {
      rtc_calib_last_status = RTC_ApplySmoothCalibrationFromDrift(rtc_ntp_drift_seconds,
                                                                   rtc_ntp_measurement_seconds);
      rtc_ntp_calibration_ready = false;
    }
    else
    {
      rtc_calib_last_status = HAL_BUSY;
    }
#else
    rtc_calib_last_status = RTC_ApplySmoothCalibrationFromDrift(RTC_CALIB_RTC_MINUS_REF_SECONDS,
                                                                 RTC_CALIB_MEASUREMENT_SECONDS);
#endif
    rtc_calib_last_ms = now_ms;
  }
}
#endif

void vdatalog_rtc_ntp_sync_update(uint32_t ntp_epoch_seconds)
{
#if VDATALOG_PERIODIC_RTC_NTP_ENABLED
  RTC_NtpSyncPoint_t *slot;
  uint32_t rtc_epoch_seconds;
  uint32_t now_ms = HAL_GetTick();

  if ((rtc_ntp_last_sync_ms != 0U) &&
      ((uint32_t)(now_ms - rtc_ntp_last_sync_ms) < RTC_NTP_SYNC_MIN_PERIOD_MS))
  {
    return;
  }

  if (!RTC_GetEpochSeconds(&rtc_epoch_seconds))
  {
    return;
  }

  slot = &rtc_ntp_points[rtc_ntp_head];
  slot->rtc_epoch_s = rtc_epoch_seconds;
  slot->ntp_epoch_s = ntp_epoch_seconds;

  rtc_ntp_head = (uint8_t)((rtc_ntp_head + 1U) % RTC_NTP_SAMPLE_COUNT);
  if (rtc_ntp_count < RTC_NTP_SAMPLE_COUNT)
  {
    rtc_ntp_count++;
  }

  rtc_ntp_last_sync_ms = now_ms;

  if (!rtc_ntp_anchor_valid)
  {
    rtc_ntp_anchor_rtc_s = rtc_epoch_seconds;
    rtc_ntp_anchor_ntp_s = ntp_epoch_seconds;
    rtc_ntp_anchor_valid = true;
    return;
  }

  if (rtc_ntp_count < RTC_NTP_SAMPLE_COUNT)
  {
    return;
  }

  if (ntp_epoch_seconds > rtc_ntp_anchor_ntp_s)
  {
    uint32_t elapsed_ref_s = ntp_epoch_seconds - rtc_ntp_anchor_ntp_s;
    if (elapsed_ref_s >= RTC_NTP_CALIB_WINDOW_SECONDS)
    {
      rtc_ntp_drift_seconds = (float)((int32_t)(rtc_epoch_seconds - rtc_ntp_anchor_rtc_s) -
                                      (int32_t)(ntp_epoch_seconds - rtc_ntp_anchor_ntp_s));
      rtc_ntp_measurement_seconds = elapsed_ref_s;
      rtc_ntp_calibration_ready = true;

      /* Start a new 6 h window from the most recent sync point. */
      rtc_ntp_anchor_rtc_s = rtc_epoch_seconds;
      rtc_ntp_anchor_ntp_s = ntp_epoch_seconds;
    }
  }
#else
  (void)ntp_epoch_seconds;
#endif
}

#if VDATALOG_PERIODIC_RTC_NTP_ENABLED
static bool RTC_GetEpochSeconds(uint32_t *epoch_seconds)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};
  uint32_t year;
  uint32_t month;
  uint32_t day;
  uint32_t y;
  uint32_t days;

  if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    return false;
  }

  if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    return false;
  }

  year = 2000U + (uint32_t)sDate.Year;
  month = (uint32_t)sDate.Month;
  day = (uint32_t)sDate.Date;

  days = 0U;
  for (y = 1970U; y < year; y++)
  {
    days += RTC_IsLeapYear(y) ? 366U : 365U;
  }

  days += RTC_DaysBeforeMonth(year, month);
  days += (day - 1U);

  *epoch_seconds = (days * 86400U) +
                   ((uint32_t)sTime.Hours * 3600U) +
                   ((uint32_t)sTime.Minutes * 60U) +
                   (uint32_t)sTime.Seconds;

  return true;
}

static bool RTC_IsLeapYear(uint32_t year)
{
  return ((year % 4U == 0U) && ((year % 100U != 0U) || (year % 400U == 0U)));
}

static uint32_t RTC_DaysBeforeMonth(uint32_t year, uint32_t month)
{
  static const uint16_t days_before_month[12] =
  {
    0U, 31U, 59U, 90U, 120U, 151U, 181U, 212U, 243U, 273U, 304U, 334U
  };

  uint32_t days = days_before_month[(month - 1U) % 12U];
  if ((month > 2U) && RTC_IsLeapYear(year))
  {
    days += 1U;
  }
  return days;
}

#endif

  
