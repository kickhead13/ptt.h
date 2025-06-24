#include "ptt.h"
#include <unistd.h>

ptth_response_t get_func(ptth_request_t req, ptth_shared_t *__) {
  char *test = ptth_map_get_value(*(req.mpath), "var_name");
  char message[100];
  message[0] = '\0';
  strcat(message, "Hi, ");
  strcat(message, test);
  strcat(message, "! :3");
  PTTH_RESPONDE_HTML(message)
}

int main() {
  ptth_server_t server;
  ptth_init_server(&server);
  ptth_add_service(&server, PTTH_GET, "/hi/{var_name}/", &get_func);
  ptth_bind_server(&server, "127.0.0.1", 8081);
  ptth_start_single(server);
}
