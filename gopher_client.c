#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define CR_LF "\x0D\x0A"
#define DEBUG_URL_MSG "/gopher/wbgopher\x0D\x0A"

struct gopher_client_tk {
    char* hostname;
    char type;
    char* path;
};

void free_gopher_client(struct gopher_client_tk *gc) {
    free(gc->hostname);
    free(gc);
}

int gopher_send_cmd(struct gopher_client_tk *gc, int s) {
    int msg_n = (sizeof(char)*strlen(gc->path))+(sizeof(char)*strlen(CR_LF));
    char msg_buf[msg_n];
    
    sprintf(msg_buf, "%s%s", gc->path, CR_LF);

    /*send network message onf socket (s)*/

    write(s, msg_buf, sizeof(char)*strlen(msg_buf));

    return msg_n;
}

struct gopher_client_tk* read_url(char *msg) {

    int fqdn_loc = (int)(strchr(msg, '/') - msg); // fqdn trails up to n characters
    int gh_type_loc = fqdn_loc + sizeof(char); // type offset
    int path_loc = gh_type_loc + sizeof(char); // path offset

    char fqdn[fqdn_loc];
    strncpy(fqdn, msg, fqdn_loc);

    char type = msg[gh_type_loc];

    char *path = msg + (sizeof(char)*path_loc);

    struct gopher_client_tk *gc;
    gc = malloc(sizeof(struct gopher_client_tk));
    gc->hostname = malloc(sizeof(char)*strlen(fqdn));
    strcpy(gc->hostname, fqdn);
    gc->type = type;
    gc->path = path;

    return gc;
}

int main(int argc, char **argv) {

  int s, bytes;
  struct sockaddr_in sa;
  char buffer[BUFSIZ+1];

  char *gopher_fqdn = argv[1];

  struct gopher_client_tk *gc = read_url(gopher_fqdn);

  struct hostent *ha;

  // docs: https://docs.freebsd.org/en/books/developers-handbook/sockets/
  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("failed to build socket");
    return -1;
  }

  // gets a list of a hosts ip addresses
  // refer to inet_aton(ip_addr, &(struct in_addr)) for
  // turning a ipv4 network address into network address order
  ha = gethostbyname(gc->hostname);

  sa.sin_family = AF_INET;
  sa.sin_port = htons(70); // Gopher communicates on port 70
  sa.sin_addr = *(struct in_addr*)ha->h_addr_list[0];

  struct sockaddr *peer;
  peer = &sa;

  if (connect(s, peer, sizeof sa) == -1) {
    perror("connection failed to establish");
    close(s);
    return -1;
  }

  gopher_send_cmd(gc, s);

  while ((bytes = read(s, buffer, BUFSIZ)) > 0) {
    // write the response to standard out (1)
    write(1, buffer, bytes);
  }

  free_gopher_client(gc);
  close(s);
  return 0;
}