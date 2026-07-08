/**
  ******************************************************************************
  * @file    Lsm6dsv16x_Mlc_PnPL.c
  * @author  SRA
  * @brief   Lsm6dsv16x_Mlc PnPL Component Manager
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
  * dtmi:appconfig:steval_mkboxpro:STAIOTCRAFT_SDATALOG:sensors:lsm6dsv16x_mlc;1
  *
  * Created by: DTDL2PnPL_cGen version 3.1.0-alpha.2
  *
  * WARNING! All changes made to this file will be lost if this is regenerated
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "App_model.h"
#include "IPnPLComponent.h"
#include "IPnPLComponent_vtbl.h"
#include "PnPLCompManager.h"

#include "Lsm6dsv16x_Mlc_PnPL.h"

static const IPnPLComponent_vtbl sLsm6dsv16x_Mlc_PnPL_CompIF_vtbl =
{
  Lsm6dsv16x_Mlc_PnPL_vtblGetKey,
  Lsm6dsv16x_Mlc_PnPL_vtblGetNCommands,
  Lsm6dsv16x_Mlc_PnPL_vtblGetCommandKey,
  Lsm6dsv16x_Mlc_PnPL_vtblGetStatus,
  Lsm6dsv16x_Mlc_PnPL_vtblSetProperty,
  Lsm6dsv16x_Mlc_PnPL_vtblExecuteFunction
};

/**
  *  Lsm6dsv16x_Mlc_PnPL internal structure.
  */
struct _Lsm6dsv16x_Mlc_PnPL
{
  /* Implements the IPnPLComponent interface. */
  IPnPLComponent_t component_if;
};

/* Objects instance ----------------------------------------------------------*/
static Lsm6dsv16x_Mlc_PnPL sLsm6dsv16x_Mlc_PnPL;

/* Public API definition -----------------------------------------------------*/
IPnPLComponent_t *Lsm6dsv16x_Mlc_PnPLAlloc()
{
  IPnPLComponent_t *pxObj = (IPnPLComponent_t *) &sLsm6dsv16x_Mlc_PnPL;
  if (pxObj != NULL)
  {
    pxObj->vptr = &sLsm6dsv16x_Mlc_PnPL_CompIF_vtbl;
  }
  return pxObj;
}

uint8_t Lsm6dsv16x_Mlc_PnPLInit(IPnPLComponent_t *_this)
{
  IPnPLComponent_t *component_if = _this;
  PnPLAddComponent(component_if);
  lsm6dsv16x_mlc_comp_init();
  return PNPL_NO_ERROR_CODE;
}


/* IPnPLComponent virtual functions definition -------------------------------*/
char *Lsm6dsv16x_Mlc_PnPL_vtblGetKey(IPnPLComponent_t *_this)
{
  (void)_this;

  return lsm6dsv16x_mlc_get_key();
}

uint8_t Lsm6dsv16x_Mlc_PnPL_vtblGetNCommands(IPnPLComponent_t *_this)
{
  (void)_this;

  return 1;
}

char *Lsm6dsv16x_Mlc_PnPL_vtblGetCommandKey(IPnPLComponent_t *_this, uint8_t id)
{
  (void)_this;

  switch (id)
  {
    case 0:
      return "lsm6dsv16x_mlc*load_model";
      break;
  }
  return (char *)PNPL_NO_ERROR_CODE;
}

uint8_t Lsm6dsv16x_Mlc_PnPL_vtblGetStatus(IPnPLComponent_t *_this, char **serializedJSON, uint32_t *size,
                                          uint8_t pretty)
{
  (void)_this;

  JSON_Value *tempJSON;
  JSON_Object *JSON_Status;

  tempJSON = json_value_init_object();
  JSON_Status = json_value_get_object(tempJSON);

  bool temp_b = 0;
  lsm6dsv16x_mlc_get_enable(&temp_b);
  json_object_dotset_boolean(JSON_Status, "lsm6dsv16x_mlc.enable", temp_b);
  char *temp_s = "";
  lsm6dsv16x_mlc_get_model_filename(&temp_s);
  json_object_dotset_string(JSON_Status, "lsm6dsv16x_mlc.model_filename", temp_s);

  if (pretty == 1)
  {
    *serializedJSON = json_serialize_to_string_pretty(tempJSON);
    *size = json_serialization_size_pretty(tempJSON);
  }
  else
  {
    *serializedJSON = json_serialize_to_string(tempJSON);
    *size = json_serialization_size(tempJSON);
  }

  /* No need to free temp_j as it is part of tempJSON */
  json_value_free(tempJSON);

  return PNPL_NO_ERROR_CODE;
}

uint8_t Lsm6dsv16x_Mlc_PnPL_vtblSetProperty(IPnPLComponent_t *_this, char *serializedJSON)
{
  (void)_this;

  JSON_Value *tempJSON = json_parse_string(serializedJSON);
  JSON_Object *tempJSONObject = json_value_get_object(tempJSON);

  uint8_t ret = PNPL_NO_ERROR_CODE;
  if (json_object_dothas_value(tempJSONObject, "lsm6dsv16x_mlc.enable"))
  {
    bool enable = json_object_dotget_boolean(tempJSONObject, "lsm6dsv16x_mlc.enable");
    ret = lsm6dsv16x_mlc_set_enable(enable);
  }
  json_value_free(tempJSON);
  return ret;
}


uint8_t Lsm6dsv16x_Mlc_PnPL_vtblExecuteFunction(IPnPLComponent_t *_this, char *serializedJSON)
{
  (void)_this;

  JSON_Value *tempJSON = json_parse_string(serializedJSON);
  JSON_Object *tempJSONObject = json_value_get_object(tempJSON);

  uint8_t ret = PNPL_NO_ERROR_CODE;
  if (json_object_dothas_value(tempJSONObject, "lsm6dsv16x_mlc*load_model.arguments"))
  {
    const char *filename;
    int32_t size;
    const char *content;
    if (json_object_dothas_value(tempJSONObject, "lsm6dsv16x_mlc*load_model.arguments.filename"))
    {
      filename =  json_object_dotget_string(tempJSONObject, "lsm6dsv16x_mlc*load_model.arguments.filename");
      if (json_object_dothas_value(tempJSONObject, "lsm6dsv16x_mlc*load_model.arguments.size"))
      {
        size = (int32_t) json_object_dotget_number(tempJSONObject, "lsm6dsv16x_mlc*load_model.arguments.size");
        if (json_object_dothas_value(tempJSONObject, "lsm6dsv16x_mlc*load_model.arguments.content"))
        {
          content =  json_object_dotget_string(tempJSONObject, "lsm6dsv16x_mlc*load_model.arguments.content");
          ret = lsm6dsv16x_mlc_load_model((char *) filename, size, (char *) content);
        }
      }
    }
  }
  json_value_free(tempJSON);
  return ret;
}

