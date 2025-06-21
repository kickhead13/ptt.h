#ifndef ___PTT_H___
#define ___PTT_H___
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <arpa/inet.h>

#define PTTH_MAX_HEADER_SIZE 4096
#define REQUEST_BUFFER_SIZE 10000

enum ptth_methods_e { 
  PTTH_GET    = 0,
  PTTH_POST   = 1,
  PTTH_PUT    = 2,
  PTTH_DELETE = 3,
  PTTH_PATCH  = 4
};

typedef struct ptth_header_t {
  char *head;
} ptth_header_t;

typedef struct ptth_body_t {
  char *body;
} ptth_body_t;

typedef struct ptth_path_t {
  char *path;
} ptth_path_t;

typedef struct ptth_shared_t {
  pthread_mutex_t   mlock;
  void             *value;
} ptth_shared_t;

typedef struct ptth_request_t {
  ptth_path_t      path;
  ptth_header_t headers;
  ptth_body_t      body;
} ptth_request_t;

typedef struct ptth_response_t {
  ptth_header_t  headers;
  ptth_body_t      body;
} ptth_response_t;

typedef struct ptth_service_t {
  struct ptth_service_t                              *next;
  char                                           *endpoint;
  ptth_response_t (*sproc)(ptth_request_t, ptth_shared_t*);
} ptth_service_t;

typedef struct ptth_services_t {
  ptth_service_t *head;
  ptth_service_t *tail;
} ptth_services_t;

typedef struct ptth_server_t {
  ptth_services_t *services[5];
  char                 *ipaddr;
  size_t                  port;
  int                      sfd;
  int                      cfd;
  struct sockaddr_in   address;
} ptth_server_t;

void ptth_init_server(ptth_server_t *);
void ptth_add_service(ptth_server_t *, enum ptth_methods_e, char *, ptth_response_t (*)(ptth_request_t, ptth_shared_t*));
void ptth_services_new_service(ptth_services_t *service, char *endpoint, ptth_response_t (*)(ptth_request_t, ptth_shared_t*));
void ptth_bind_server(ptth_server_t *, char *, size_t);
#define PTTH_RESPONDE_HTML(response_payload)\
  ptth_response_t resp;\
  resp.body.body=malloc(strlen((response_payload)) + 1);\
  strcpy(resp.body.body,(response_payload));\
  resp.headers.head = malloc(strlen("HTTP/1.1 202 Accepted\r\n""Content-Type: text/html\r\n""Connection: close\r\n""\r\n") + 1);\
  strcpy(resp.headers.head, "HTTP/1.1 202 Accepted\r\n""Content-Type: text/html\r\n""Connection: close\r\n""\r\n");\
  return resp;

void ptth_init_server(ptth_server_t *server) {
  for(int i = 0; i<5; i++) 
    server -> services[i] = NULL;
  server -> ipaddr = NULL;
  server -> port   =    0;
}

void ptth_services_new_service(
    ptth_services_t   *service,
    char             *endpoint,
    ptth_response_t (*sproc)(ptth_request_t, ptth_shared_t*)
) {
  
  if(service -> head == NULL) {
    service -> head = malloc(sizeof(ptth_service_t));
    service -> head -> next = NULL;
    service -> head -> endpoint = malloc(strlen(endpoint) + 1);
    strcpy(service -> head -> endpoint, endpoint);
    service -> head -> sproc = sproc;
    service -> tail = service -> head;
    return;
  }

  if(service -> head == service -> tail) {
    service -> tail = malloc(sizeof(ptth_service_t));
    service -> tail -> next = NULL;
    service -> tail -> endpoint = malloc(strlen(endpoint) + 1);
    strcpy(service -> tail -> endpoint, endpoint);
    service -> tail -> sproc = sproc;
    service -> head -> next = service -> tail;
    return;
  }

  ptth_service_t *next = service -> head -> next;
  ptth_service_t *new = malloc(sizeof(ptth_service_t));
  new -> next = next;
  new -> endpoint = malloc(strlen(endpoint) + 1);
  strcpy(new -> endpoint, endpoint);
  new -> sproc = sproc;
  service -> head -> next = new;

}

void ptth_add_service(
    ptth_server_t         *server,
    enum ptth_methods_e    method, 
    char                *endpoint,
    ptth_response_t (*sproc)(ptth_request_t, ptth_shared_t*)
) {

  if(server->services[method] == NULL) {
    server -> services[method] = malloc(sizeof(ptth_services_t));
    server -> services[method] -> head = NULL;
    server -> services[method] -> tail = NULL;
  }
  ptth_services_new_service(server->services[method], endpoint, sproc);

}

void ptth_bind_server(ptth_server_t *server, char *ipaddr, size_t port) {
  server -> ipaddr = malloc(strlen(ipaddr) + 1);
  strcpy(server -> ipaddr, ipaddr);
  server -> port = port;
}

enum ptth_methods_e ptth_get_method(char *buffer) {
  char *method = malloc(7);
  int i = 0;
  for(; i<7; i++) {
    if(buffer[i] == ' ')  {
      break;
    }
    method[i] = buffer[i];
  }
  method[i] = '\0';

  printf("%s ", method);

  if(strcmp(method, "GET") == 0) return PTTH_GET;
  if(strcmp(method, "PUT") == 0) return PTTH_PUT;
  if(strcmp(method, "POST") == 0) return PTTH_PUT;
  if(strcmp(method, "PATCH") == 0) return PTTH_PATCH;
  return PTTH_DELETE;
}

char *ptth_get_req_endpoint(char *req) {
  char ep[2048];
  int sp = 0, iter = 0, cont = 0;
  while(sp < 2) {
    if(req[iter] == ' ') sp++;
    else if(sp > 0) {
      ep[cont] = req[iter];
      cont++;
    }
    iter++;
  }
  ep[cont] = '\0';
  char *ret = malloc(strlen(ep) + 1);
  strcpy(ret, ep);
  return ret;
}

void ptth_handle(int cfd, ptth_services_t *services[5]) {
  printf("[SERVER] ");

  char request[REQUEST_BUFFER_SIZE];
  int bytes_read = read(cfd, request, REQUEST_BUFFER_SIZE-1);
  
  ptth_request_t req;
  ptth_shared_t *sh;

  char *req_ep = ptth_get_req_endpoint(request);
  printf("%s\n", req_ep);

  ptth_response_t resp;
  ptth_service_t *head = services[ptth_get_method(request)] -> head;

  int fine = 0;

  while(head != NULL) {
    if(strncmp(head -> endpoint, req_ep, strlen(head->endpoint)) == 0) {
      resp = head -> sproc(req, sh);
      fine = 1;
      break;
    }
  }

  if(fine == 0) close(cfd);

  write(cfd, resp.headers.head, strlen(resp.headers.head));
  write(cfd, resp.body.body, strlen(resp.body.body));
  close(cfd);
  return;
}

int ptth_start(ptth_server_t server) {
  int sfd, cfd;
  int opt = 1;
  int addrlen = sizeof(server.address);
    
  if ((server.sfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("[SERVER] FAILED ON socket(...)");
    exit(EXIT_FAILURE);
  }
    
  if (setsockopt(server.sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("[SERVER] FAILED ON setsockopt(...)");
    exit(EXIT_FAILURE);
  }
    
  server.address.sin_family = AF_INET;
  if(server.ipaddr != NULL) server.address.sin_addr.s_addr = inet_addr(server.ipaddr);
  else server.address.sin_addr.s_addr = INADDR_ANY;
  server.address.sin_port = htons(server.port);
  

  if (bind(server.sfd, (struct sockaddr *)&server.address, sizeof(server.address)) < 0) {
    perror("[SERVER] FAILED ON bind(...)");
    exit(EXIT_FAILURE);
  }
    
  if (listen(server.sfd, 3) < 0) {
    perror("[SERVER] FAILED ON listen(...)");
    exit(EXIT_FAILURE);
  }
    
  printf("[START] SERVER listening on [ADDR=\"%s\" PORT=%ld]\n",inet_ntoa(server.address.sin_addr), server.port);
    
  while (1) {
    if ((server.cfd = accept(server.sfd, (struct sockaddr *)&server.address, (socklen_t*)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
      continue;
    }
        
    ptth_handle(server.cfd, server.services);
  }
    
  return 0;
} 
#endif
