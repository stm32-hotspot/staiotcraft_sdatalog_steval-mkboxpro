/**
  ******************************************************************************
  * @file    App_model_Lsm6dsv16x_Gyro.h
  * @author  SRA
  * @brief   Lsm6dsv16x_Gyro PnPL Components APIs
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *
  ******************************************************************************
  */

/**
  ******************************************************************************
  * This file has been auto generated from the following DTDL Component:
  * dtmi:appconfig:steval_mkboxpro:STAIOTCRAFT_SDATALOG:sensors:lsm6dsv16x_gyro;1
  *
  * Created by: DTDL2PnPL_cGen version 3.1.0-alpha.2
  *
  * WARNING! All changes made to this file will be lost if this is regenerated
  ******************************************************************************
  */

/**
  ******************************************************************************
  * Component APIs *************************************************************
  * - Component init function
  *    <comp_name>_comp_init(void)
  * - Component get_key function
  *    <comp_name>_get_key(void)
  * - Component GET/SET Properties APIs ****************************************
  *  - GET Functions
  *    uint8_t <comp_name>_get_<prop_name>(prop_type *value)
  *      if prop_type == char --> (char **value)
  *  - SET Functions
  *    uint8_t <comp_name>_set_<prop_name>(prop_type value)
  *      if prop_type == char --> (char *value)
  *  - Component COMMAND Reaction Functions
  *      uint8_t <comp_name>_<command_name>(
  *                     field1_type field1_name, field2_type field2_name, ...,
  *                     fieldN_type fieldN_name);
  *  - Component TELEMETRY Send Functions
  *      uint8_t <comp_name>_create_telemetry(tel1_type tel1_name,
  *                     tel2_type tel2_name, ..., telN_type telN_name,
  *                     char **telemetry, uint32_t *size)
  ******************************************************************************
  */

#ifndef APP_MODEL_LSM6DSV16X_GYRO_H_
#define APP_MODEL_LSM6DSV16X_GYRO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "Lsm6dsv16x_Gyro_PnPL.h"

/* USER includes -------------------------------------------------------------*/
#define DEFAULT_GYRO_ODR_HZ        pnpl_lsm6dsv16x_gyro_odr_hz240
#define DEFAULT_GYRO_ODR_HZ_FLOAT  240.0f
#define DEFAULT_GYRO_FS_DPS        pnpl_lsm6dsv16x_gyro_fs_dps4000
#define DEFAULT_GYRO_FS_DPS_INT    4000

/* LSM6DSV16X_GYRO PnPL Component --------------------------------------------- */
uint8_t lsm6dsv16x_gyro_comp_init(void);
char *lsm6dsv16x_gyro_get_key(void);
uint8_t lsm6dsv16x_gyro_get_enable(bool *value);
uint8_t lsm6dsv16x_gyro_get_data_type(char **value);
uint8_t lsm6dsv16x_gyro_get_dimensions(int32_t *value);
uint8_t lsm6dsv16x_gyro_get_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_short_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_sensor_category(int32_t *value);
uint8_t lsm6dsv16x_gyro_get_odr(pnpl_lsm6dsv16x_gyro_odr_t *enum_id);
uint8_t lsm6dsv16x_gyro_get_fs(pnpl_lsm6dsv16x_gyro_fs_t *enum_id);
uint8_t lsm6dsv16x_gyro_get_initial_timestamp(float_t *value);
uint8_t lsm6dsv16x_gyro_get_measured_odr(float_t *value);
uint8_t lsm6dsv16x_gyro_get_sensitivity(float_t *value);
uint8_t lsm6dsv16x_gyro_get_annotation(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__channel_specification_interface(
  pnpl_lsm6dsv16x_gyro_output_st_stream__channel_specification_interface_t *enum_id);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__channel_specification_protocol_type(
  pnpl_lsm6dsv16x_gyro_output_st_stream__channel_specification_protocol_type_t *enum_id);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__channel_specification_protocol_version(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__channel_specification_payload_type(
  pnpl_lsm6dsv16x_gyro_output_st_stream__channel_specification_payload_type_t *enum_id);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__channel_specification_payload_version(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__channel_specification_stream_id(int32_t *value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_timestamp_data_type(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_timestamp_dimensions(int32_t *value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_timestamp_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_timestamp_short_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_timestamp_labels(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_x_data_type(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_x_dimensions(int32_t *value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_x_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_x_short_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_x_labels(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_y_data_type(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_y_dimensions(int32_t *value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_y_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_y_short_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_y_labels(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_z_data_type(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_z_dimensions(int32_t *value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_z_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_z_short_unit(char **value);
uint8_t lsm6dsv16x_gyro_get_output_st_stream__payload_specification_z_labels(char **value);
uint8_t lsm6dsv16x_gyro_set_enable(bool value);
uint8_t lsm6dsv16x_gyro_set_odr(pnpl_lsm6dsv16x_gyro_odr_t enum_id);
uint8_t lsm6dsv16x_gyro_set_fs(pnpl_lsm6dsv16x_gyro_fs_t enum_id);
uint8_t lsm6dsv16x_gyro_set_annotation(const char *value);


#ifdef __cplusplus
}
#endif

#endif /* APP_MODEL_LSM6DSV16X_GYRO_H_ */
