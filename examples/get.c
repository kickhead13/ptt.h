#include "ptt.h"
#include <unistd.h>

ptth_response_t get_func(ptth_request_t _, ptth_shared_t *__) {
  PTTH_RESPONDE_HTML(
    "<!DOCTYPE html>"
    "<head></head>"
    "<body>"
    "HI! :)"
    "<button>CLICK ME!</button>"
    "</body>")
}

int main() {
  ptth_server_t server;
  ptth_init_server(&server);
  ptth_add_service(&server, PTTH_GET, "/example", &get_func);
  ptth_bind_server(&server, "127.0.0.1", 8081);
  ptth_start_single(server);
}
