/* The original source code of this library is licensed under th GPLv3 License.
 * So have fun with this :3.
 *
 * Original Maintainer: Ana Alexandru-Gabriel <ana.alexandru.gabriel@proton.me>
 * Original Git Repo: https://github.com/kickhead13/ptt.h
 * */


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
#include <regex.h>

#define PTTH_MAX_HEADER_SIZE 4096
#define PTTH_REQ_BUFFER_SIZE 10000
#define PTTH_SIZE_OF_MAP 10


#define PTTH1_1 "HTTP/1.1"
// TODO: add all HTTP methods
#define PTTH1_1_404_NOT_FOUND PTTH1_1" 404 Not Found"
#define PTTH1_1_202_ACCEPTED PTTH1_1" 202 Accepted"

enum ptth_methods_e { 
  PTTH_GET    = 0,
  PTTH_POST   = 1,
  PTTH_PUT    = 2,
  PTTH_DELETE = 3,
  PTTH_PATCH  = 4
};

typedef struct ptth_map_t {
  char *keys[PTTH_SIZE_OF_MAP];
  char *vals[PTTH_SIZE_OF_MAP];
  size_t len;
} ptth_map_t;

typedef struct ptth_header_t {
  char *head;
} ptth_header_t;

typedef struct ptth_body_t {
  char *body;
} ptth_body_t;

typedef struct ptth_path_t {
  char *path;
  ptth_map_t kv_map;
} ptth_path_t;

typedef struct ptth_shared_t {
  pthread_mutex_t   mlock;
  void             *value;
} ptth_shared_t;

typedef struct ptth_request_t {
  ptth_map_t     *mpath;
  ptth_header_t headers;
  ptth_body_t      body;
} ptth_request_t;

typedef struct ptth_response_t {
  ptth_header_t  headers;
  ptth_body_t      body;
} ptth_response_t;

typedef struct ptth_endpoint_t {
  regex_t rgx; 
} ptth_endpoint_t;

typedef struct ptth_service_t {
  struct ptth_service_t                              *next;
  regex_t                                        *endpoint;
  ptth_map_t                                      *url_map;
  int                              order[PTTH_SIZE_OF_MAP];
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


// Please notice the API exposed by the library here:
// API for managing HTTP server:
void ptth_init_server(ptth_server_t *);
void ptth_add_service(ptth_server_t *, enum ptth_methods_e, char *, ptth_response_t (*)(ptth_request_t, ptth_shared_t*));
void ptth_services_new_service(ptth_services_t *service, char *endpoint, ptth_response_t (*)(ptth_request_t, ptth_shared_t*));
void ptth_bind_server(ptth_server_t *, char *, size_t);
ptth_response_t ptth_new_repsonse();
ptth_request_t ptth_empty_request();

// API for managing underlying maps (please use carefully)
int ptth_map_add_kv(ptth_map_t *, char *, char *);
char *ptth_map_value(ptth_map_t, char *);
int ptth_key_index(ptth_map_t, char *);
char *ptth_map_get_value(ptth_map_t, char *);  // This one is particularly useful (see dynamic_request_path.c in examples)
ptth_map_t *new_ptth_map();
int ptth_map_set_value(ptth_map_t *, char *, char *);

#define PTTH_RESPONDE_HTML(response_payload)\
  ptth_response_t resp;\
  resp.body.body=malloc(strlen((response_payload)) + 1);\
  strcpy(resp.body.body,(response_payload));\
  resp.headers.head = malloc(strlen(PTTH1_1_202_ACCEPTED"\r\n""Content-Type: text/html\r\n""Connection: close\r\n""\r\n") + 1);\
  strcpy(resp.headers.head, PTTH1_1_202_ACCEPTED"\r\n""Content-Type: text/html\r\n""Connection: close\r\n""\r\n");\
  return resp;


char *ptth_map_value(ptth_map_t map, char *key) {
  for(size_t i = 0; i < PTTH_SIZE_OF_MAP; i++) {
    if(map.keys[i] == NULL) break;
    if(strcmp(map.keys[i], key) == 0) return map.vals[i];
  }
  return NULL;
}

int ptth_key_index(ptth_map_t map, char *fk) {
  for(size_t i = 0; i < map.len; i++) {
    if(strcmp(map.keys[i], fk) == 0) return i;
  }
  return -1;
}

char *ptth_map_get_value(ptth_map_t map, char *key) {
  for(size_t i = 0; i<map.len; i++) {
    if(strcmp(map.keys[i], key) == 0) return map.vals[i];
  }
  return NULL;
}

ptth_map_t *new_ptth_map() {
  ptth_map_t * ret = malloc(sizeof(ptth_map_t));
  ret -> len = 0;
  return ret;
}

int ptth_map_set_value(ptth_map_t *map, char *key, char *val) {
  if(map == NULL) return 0;
  for(size_t i = 0; i < map -> len; i++) {
    if(strcmp(map -> keys[i], key) == 0) {
      if(map -> vals[i] != NULL) free(map -> vals[i]);
      map -> vals[i] = malloc(strlen(val) + 1);
      strcpy(map -> vals[i], val);
      return 1;
    }
  }
  return ptth_map_add_kv(map, key, val);
}

int ptth_map_add_kv(ptth_map_t *map, char *key, char *val) {
  if(map == NULL) return 0;


  map -> keys[map -> len] = malloc(strlen(key)+1);
  map -> vals[map -> len] = malloc(strlen(val)+1);

  strcpy(map -> keys[map -> len], key);
  strcpy(map -> vals[map -> len], val);

  if(map -> len < PTTH_SIZE_OF_MAP - 1) map -> len = map -> len + 1; 
  return 1;
}

ptth_response_t ptth_new_reponse() {
  ptth_response_t *r = malloc(sizeof(ptth_response_t));
  return *r;
}

ptth_request_t ptth_empty_request() {
  ptth_request_t *req = malloc(sizeof(ptth_request_t));
  return *req;
}

void ptth_init_server(ptth_server_t *server) {
  for(int i = 0; i<5; i++) 
    server -> services[i] = malloc(sizeof(ptth_services_t));
  server -> ipaddr = NULL;
  server -> port   =    0;
}

regex_t *ptth_process_regex(char *load, ptth_map_t *map, int *order) {
  size_t ll = strlen(load);
  int exp = 0;
  char *nload = malloc(20+(strlen(load) * 2));
  size_t nlc = 0;
  char nk[20];
  size_t nkc = 0;
  size_t sc = -1;
  for(size_t i = 0; i < ll; i++) {
    if(load[i] == '/') sc++;
    if(load[i] == '{') {
      exp = 1;
      nload[nlc] = '\0';
      strcat(nload, "[.a-zA-z0-9]+");
      nlc = strlen(nload);
    }
    if(exp == 0) {
      nload[nlc] = load[i];
      nlc++;
    }
    if(load[i] == '}') {
      exp = 0;
      nk[nkc] = '\0';
      if(map == NULL)map = new_ptth_map();
      order[map -> len] = sc;
      ptth_map_set_value(map, nk, "");
    }
    if(exp == 1 && load[i] != '{') {
      nk[nkc] = load[i];
      nkc++;
    }
  }
  nload[nlc] = '\0';

  regex_t *nr = malloc(sizeof(regex_t));
  regcomp(nr, nload, REG_EXTENDED);
  return nr;
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
    service -> head -> url_map = new_ptth_map();
    service -> head -> endpoint = ptth_process_regex(endpoint, service -> head -> url_map, service -> head -> order);
    service -> head -> sproc = sproc;
    service -> tail = service -> head;
    return;
  }
  
  if(service -> head == service -> tail) {
    service -> tail = malloc(sizeof(ptth_service_t));
    service -> tail -> next = NULL;
    service -> tail -> endpoint = malloc(strlen(endpoint) + 1);
    service -> tail -> url_map = new_ptth_map();
    service -> tail -> endpoint = ptth_process_regex(endpoint, service -> tail -> url_map, service -> tail -> order);
    service -> tail -> sproc = sproc;
    service -> head -> next = service -> tail;
    return;
  }

  ptth_service_t *next = service -> head -> next;
  ptth_service_t *new = malloc(sizeof(ptth_service_t));
  new -> next = next;
  new -> endpoint = malloc(strlen(endpoint) + 1);
  new -> url_map = new_ptth_map();
  new -> endpoint = ptth_process_regex(endpoint, new -> url_map, new -> order);
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

  if(strcmp(method, "GET") == 0) return PTTH_GET;
  if(strcmp(method, "PUT") == 0) return PTTH_PUT;
  if(strcmp(method, "POST") == 0) return PTTH_POST;
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

void ptth_log(enum ptth_methods_e method, char *ed, int fine) {
  
  char tbuffer[100];
  struct tm* tm_info;
  time_t timer;
  
  timer = time(NULL);
  tm_info = localtime(&timer);
  strftime(tbuffer, 100, "%Y-%m-%d %H:%M:%S ", tm_info);

  write(1, tbuffer, strlen(tbuffer));

  if(method == PTTH_GET) write(1, "[GET   ]", 8);
  if(method == PTTH_POST) write(1, "[POST  ]", 8);
  if(method == PTTH_PUT) write(1, "[PUT   ]", 8);
  if(method == PTTH_PATCH) write(1, "[PATCH ]", 8);
  if(method == PTTH_DELETE) write(1, "[DELETE]", 8);
  
  if(fine) {
    write(1, " [FINE   ]", 10);
  } else {
    write(1, " [INVALID]", 10);
  }

  write(1, " ", 1);
  write(1, ed, strlen(ed));
  write(1, "\n", 1);
}

void ptth_handle(int cfd, ptth_services_t *services[5]) {
  
  char request[PTTH_REQ_BUFFER_SIZE];
  int bytes_read = read(cfd, request, PTTH_REQ_BUFFER_SIZE-1);

  ptth_request_t req;
  ptth_shared_t *sh;

  char *req_ep = ptth_get_req_endpoint(request);
  char *sreq_ep = malloc(strlen(req_ep) + 1);
  strcpy(sreq_ep, req_ep);

  ptth_response_t resp;
  enum ptth_methods_e meth = ptth_get_method(request);
  ptth_service_t *head = services[meth] -> head;
  
  int fine = 0;
  
  char *hold_split_endpoint[PTTH_SIZE_OF_MAP];

  while(head != NULL) {
    if(regexec(head -> endpoint, req_ep, 0, NULL, 0) == 0) {
      char *token = strtok(sreq_ep, "/");
      size_t ec = 0;
      while(token != NULL) {
        hold_split_endpoint[ec] = token;
        token = strtok(NULL, "/");
        ec++;
      }

      for(size_t i = 0; i < head -> url_map -> len; i++) {
        ptth_map_set_value(head -> url_map, head -> url_map -> keys[i], hold_split_endpoint[head -> order[i]]);
      }
      
      req.mpath =  head -> url_map;
      fine = 1;
      ptth_log(meth, req_ep, fine);
      resp = head -> sproc(req, sh);
      break;
    }
    head = head -> next;
  }
  
  
  if(fine == 0) {
    ptth_log(meth, req_ep, fine);
    write(cfd, PTTH1_1_404_NOT_FOUND, sizeof(PTTH1_1_404_NOT_FOUND));
    close(cfd);
    return;
  }

  write(cfd, resp.headers.head, strlen(resp.headers.head));
  write(cfd, resp.body.body, strlen(resp.body.body));
  close(cfd);
  return;
}

#define _sv_cstx(x) ((ptth_server_t *)(x))
void *ptth_thread_operate(void *server) {
  int addrlen = sizeof(_sv_cstx(server)->address);
  while (1) {
    if ((_sv_cstx(server)->cfd = accept(_sv_cstx(server)->sfd, (struct sockaddr *)&_sv_cstx(server)->address, (socklen_t*)&addrlen)) < 0) {
      perror("accept");
      exit(EXIT_FAILURE);
      continue;
    }
        
    ptth_handle(_sv_cstx(server)->cfd, _sv_cstx(server)->services);
  }
}

ptth_server_t *ptth_server_copy(ptth_server_t server) {
  ptth_server_t *cp = malloc(sizeof(ptth_server_t));
  cp -> cfd = server.cfd;
  cp -> sfd = server.sfd;
  for(size_t iter = 0; iter < 5; iter++) 
    cp -> services[iter] = server.services[iter];
  cp -> address = server.address;
  cp -> ipaddr = NULL;
  return cp;
}

int ptth_start(ptth_server_t server, int no_of_workers) {
  int opt = 1;
    
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
  fflush(stdout);
  
  pthread_t pt[no_of_workers];
  ptth_server_t *nsa[no_of_workers];
  
  char loading[no_of_workers+1];
  loading[no_of_workers] = '\0';
  for(size_t iter = 0; iter < no_of_workers; iter++) loading[iter] = '.';
  for(size_t iter = 0; iter < no_of_workers; iter++) {
    nsa[iter] = ptth_server_copy(server);
    int pthread_create_ret = pthread_create(&pt[iter], NULL, ptth_thread_operate, nsa[iter]);
    if(pthread_create_ret) {
      perror("[START] failed to create worker using pthread_create(...)");
      exit(EXIT_FAILURE);
    }
    loading[iter] = '#';
    printf("\r[START] Booting Workers up [%s]",loading);
    fflush(stdout);
  }
  write(1, "\n", 1);

  for(size_t iter = 0; iter < no_of_workers; iter++) {
    pthread_join(pt[iter], NULL);
  }
  return 0;
} 

#define ptth_start_single(sv) ptth_start((sv), 1)
#endif
