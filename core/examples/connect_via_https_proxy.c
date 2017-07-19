#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtm.h>

static char const *endpoint = "YOUR_ENDPOINT";
static char const *appkey = "YOUR_APPKEY";
static char const *proxy_endpoint = "http://127.0.0.1:7711";
static char const *channel = "OPEN_CHANNEL";

void pdu_handler(rtm_client_t *client, const rtm_pdu_t *pdu) {
  switch (pdu->action) {
    case RTM_ACTION_SUBSCRIPTION_DATA: {
      char *message;
      while ((message = rtm_iterate(&pdu->message_iterator))) {
        printf("Got message: %s\n", message);
      }
      break;
    }
    default:
      rtm_default_pdu_handler(client, pdu);
      break;
  }
}

int main(void) {
  void *memory = malloc(rtm_client_size);
  rtm_client_t *client = rtm_init(memory, &pdu_handler, 0);
  rtm_enable_verbose_logging(client);
  rtm_status rc = rtm_connect_via_https_proxy(client, endpoint, appkey, proxy_endpoint);

  if (rc != RTM_OK) {
    fprintf(stderr, "Failed to connect: %d %s\n", rc, rtm_error_string(rc));
    goto cleanup;
  }

  printf("Connected to Satori RTM!\n");

  unsigned request_id;
  rc = rtm_subscribe(client, channel, &request_id);
  if (rc != RTM_OK) {
    fprintf(stderr, "Failed to send subscribe request: %s\n", rtm_error_string(rc));
    goto cleanup;
  }

  while (1) {
    rc = rtm_wait_timeout(client, 10 /* seconds */);
    if (rc != RTM_OK && rc != RTM_ERR_TIMEOUT) {
      fprintf(stderr, "Error while waiting for subscription data: %s\n", rtm_error_string(rc));
      goto cleanup;
    }
  }

  cleanup:
  rtm_close(client);
  free(client);
  return rc;
}