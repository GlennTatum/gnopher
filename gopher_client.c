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
  char* pathname;
};

void free_gopher_client(struct gopher_client_tk *gc) {
  free(gc->hostname);
  free(gc->pathname);
  free(gc);
}

int gopher_uri(char* resource, int resource_size, char* dst) {
  /*  
  in: gopher.floodgap.com/gopher/wbgopher
  out: length of path
  */
  char buf[resource_size];

  // first occurence of '/'
  int offset;
  for (int i = 0; i < resource_size; i++)
  {
    if (resource[i] == '/') {
      offset = i;
      break;
    }
  }

  // copy the uri into the buffer /gopher/wbgopher -> buf
  int j;
  for (int i = offset; i < resource_size; i++) {
    buf[j] = resource[i];
    j++;
  }

  // compute the size of the uri
  int r_uri_size;
  for (int i = 0; i < (int)sizeof(buf); i++) {
    if (buf[i] == '\0') {
      r_uri_size = i;
      break;
    }
  }
  memcpy(dst, buf, r_uri_size);

  return r_uri_size;
}

struct gopher_client_tk* build_gopher_client(char* resource) {

  char dst[strlen(resource)];
  int uri_size = gopher_uri(resource, strlen(resource), dst);

  char msg_buf[uri_size + sizeof(CR_LF)];

  char uri[uri_size];
  memcpy(uri, dst, uri_size);
  sprintf(msg_buf, "%s%s", uri, CR_LF);

  // first occurence of '/'
  int offset;
  for (int i = 0; i < (int)strlen(resource); i++)
  {
    if (resource[i] == '/') {
      offset = i;
      break;
    }
  }
  char hostname[offset];

  memcpy(hostname, resource, offset);

  struct gopher_client_tk *gc;
  gc = malloc(sizeof(struct gopher_client_tk));

  gc->hostname = malloc(strlen(hostname)*sizeof(hostname));
  strcpy(gc->hostname, hostname);
  gc->pathname = malloc(strlen(msg_buf)*sizeof(msg_buf));
  strcpy(gc->pathname, msg_buf);

  return gc;

}

int main(int argc, char **argv) {

  int s, bytes;
  struct sockaddr_in sa;
  char buffer[BUFSIZ+1];

  char *gopher_fqdn = argv[1];

  struct gopher_client_tk *gc = build_gopher_client(gopher_fqdn);

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

  write(s, gc->pathname, sizeof(char)*strlen(gc->pathname)); // Send the URL

  while ((bytes = read(s, buffer, BUFSIZ)) > 0) {
    // write the response to standard out (1)
    write(1, buffer, bytes);
  }

  free_gopher_client(gc);
  close(s);
  return 0;
}