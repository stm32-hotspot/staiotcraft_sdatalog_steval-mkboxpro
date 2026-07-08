/**
 ******************************************************************************
 * @file    ntp_client.h
 * @brief   Lightweight NTP client interface for embedded systems.
 ******************************************************************************
 */

#ifndef __NTP_CLIENT_H
#define __NTP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define NTP_CLIENT_DEFAULT_PORT                 123U
#define NTP_CLIENT_DEFAULT_TIMEOUT_MS           2000U
#define NTP_UNIX_EPOCH_DELTA_SECONDS            2208988800UL

typedef enum
{
  NTP_CLIENT_STATUS_OK = 0,
  NTP_CLIENT_STATUS_INVALID_PARAM,
  NTP_CLIENT_STATUS_IO_ERROR,
  NTP_CLIENT_STATUS_TIMEOUT,
  NTP_CLIENT_STATUS_INVALID_RESPONSE,
  NTP_CLIENT_STATUS_UNIX_TIME_UNAVAILABLE
} NTP_ClientStatus_t;

typedef int32_t (*NTP_ClientUdpSendFn)(void *user_ctx,
                                       const char *server_name,
                                       uint16_t server_port,
                                       const uint8_t *payload,
                                       uint16_t payload_len);

typedef int32_t (*NTP_ClientUdpRecvFn)(void *user_ctx,
                                       uint8_t *payload,
                                       uint16_t payload_max_len,
                                       uint32_t timeout_ms);

typedef struct
{
  NTP_ClientUdpSendFn udp_send;
  NTP_ClientUdpRecvFn udp_recv;
  void *user_ctx;
} NTP_ClientIo_t;

typedef struct
{
  const char *server_name;
  uint16_t server_port;
  uint32_t timeout_ms;
} NTP_ClientConfig_t;

typedef struct
{
  NTP_ClientIo_t io;
  NTP_ClientConfig_t cfg;
} NTP_Client_t;

NTP_ClientStatus_t NTP_Client_Init(NTP_Client_t *client,
                                   const NTP_ClientConfig_t *cfg,
                                   const NTP_ClientIo_t *io);

NTP_ClientStatus_t NTP_Client_QueryUnixTime(NTP_Client_t *client,
                                            uint32_t *unix_time_s,
                                            uint32_t *ntp_fraction);

NTP_ClientStatus_t NTP_Client_NtpToUnix(uint32_t ntp_seconds,
                                        uint32_t *unix_seconds);

#ifdef __cplusplus
}
#endif

#endif /* __NTP_CLIENT_H */
