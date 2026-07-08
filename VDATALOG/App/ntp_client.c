/**
 ******************************************************************************
 * @file    ntp_client.c
 * @brief   Lightweight NTP client implementation for embedded systems.
 ******************************************************************************
 */

#include "ntp_client.h"

#include <string.h>

#define NTP_PACKET_SIZE                         48U
#define NTP_CLIENT_REQUEST_FLAGS                0x23U /* LI=0, VN=4, Mode=3 (client) */
#define NTP_MODE_SERVER                         4U
#define NTP_MODE_BROADCAST                      5U

static uint32_t NTP_ReadU32Be(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) |
         ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) |
         (uint32_t)p[3];
}

NTP_ClientStatus_t NTP_Client_Init(NTP_Client_t *client,
                                   const NTP_ClientConfig_t *cfg,
                                   const NTP_ClientIo_t *io)
{
  if ((client == NULL) || (cfg == NULL) || (io == NULL))
  {
    return NTP_CLIENT_STATUS_INVALID_PARAM;
  }

  if ((cfg->server_name == NULL) || (io->udp_send == NULL) || (io->udp_recv == NULL))
  {
    return NTP_CLIENT_STATUS_INVALID_PARAM;
  }

  client->io = *io;
  client->cfg = *cfg;

  if (client->cfg.server_port == 0U)
  {
    client->cfg.server_port = NTP_CLIENT_DEFAULT_PORT;
  }

  if (client->cfg.timeout_ms == 0U)
  {
    client->cfg.timeout_ms = NTP_CLIENT_DEFAULT_TIMEOUT_MS;
  }

  return NTP_CLIENT_STATUS_OK;
}

NTP_ClientStatus_t NTP_Client_QueryUnixTime(NTP_Client_t *client,
                                            uint32_t *unix_time_s,
                                            uint32_t *ntp_fraction)
{
  uint8_t request[NTP_PACKET_SIZE];
  uint8_t response[NTP_PACKET_SIZE];
  int32_t send_rc;
  int32_t recv_rc;
  uint32_t ntp_seconds;
  uint8_t mode;

  if ((client == NULL) || (unix_time_s == NULL))
  {
    return NTP_CLIENT_STATUS_INVALID_PARAM;
  }

  if ((client->io.udp_send == NULL) || (client->io.udp_recv == NULL) ||
      (client->cfg.server_name == NULL))
  {
    return NTP_CLIENT_STATUS_INVALID_PARAM;
  }

  (void)memset(request, 0, sizeof(request));
  request[0] = NTP_CLIENT_REQUEST_FLAGS;

  send_rc = client->io.udp_send(client->io.user_ctx,
                                client->cfg.server_name,
                                client->cfg.server_port,
                                request,
                                (uint16_t)sizeof(request));
  if (send_rc < 0)
  {
    return NTP_CLIENT_STATUS_IO_ERROR;
  }

  recv_rc = client->io.udp_recv(client->io.user_ctx,
                                response,
                                (uint16_t)sizeof(response),
                                client->cfg.timeout_ms);
  if (recv_rc == 0)
  {
    return NTP_CLIENT_STATUS_TIMEOUT;
  }

  if (recv_rc < 0)
  {
    return NTP_CLIENT_STATUS_IO_ERROR;
  }

  if (recv_rc < (int32_t)NTP_PACKET_SIZE)
  {
    return NTP_CLIENT_STATUS_INVALID_RESPONSE;
  }

  mode = (uint8_t)(response[0] & 0x07U);
  if ((mode != NTP_MODE_SERVER) && (mode != NTP_MODE_BROADCAST))
  {
    return NTP_CLIENT_STATUS_INVALID_RESPONSE;
  }

  if (response[1] == 0U)
  {
    return NTP_CLIENT_STATUS_INVALID_RESPONSE;
  }

  ntp_seconds = NTP_ReadU32Be(&response[40]);
  if (ntp_seconds < NTP_UNIX_EPOCH_DELTA_SECONDS)
  {
    return NTP_CLIENT_STATUS_UNIX_TIME_UNAVAILABLE;
  }

  *unix_time_s = ntp_seconds - NTP_UNIX_EPOCH_DELTA_SECONDS;

  if (ntp_fraction != NULL)
  {
    *ntp_fraction = NTP_ReadU32Be(&response[44]);
  }

  return NTP_CLIENT_STATUS_OK;
}

NTP_ClientStatus_t NTP_Client_NtpToUnix(uint32_t ntp_seconds,
                                        uint32_t *unix_seconds)
{
  if (unix_seconds == NULL)
  {
    return NTP_CLIENT_STATUS_INVALID_PARAM;
  }

  if (ntp_seconds < NTP_UNIX_EPOCH_DELTA_SECONDS)
  {
    return NTP_CLIENT_STATUS_UNIX_TIME_UNAVAILABLE;
  }

  *unix_seconds = ntp_seconds - NTP_UNIX_EPOCH_DELTA_SECONDS;

  return NTP_CLIENT_STATUS_OK;
}
