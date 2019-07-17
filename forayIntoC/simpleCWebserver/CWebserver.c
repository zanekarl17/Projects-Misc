#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

/* cool bckgrnd color {background-color: #8F594D} */
/* Note that this format for the 'webpage' string just concatenates all the characters into one long line of text */

char webpage[] =
"HTTP/1.1 200 OK\r\n"
"Content-Type: text/html; charset=UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<html><head><title>Zane's C web page</title>\r\n"
"<style>body {background-image: url(\"backimg.jpg\")}"
"img {border-radius: 5px}</style></head>\r\n"
"<body><center><h1>Hello Zane Welcome to your home page!</h1><br>\r\n"
  "<img src=\"img.jpg\"></center></body></html>\r\n";

int cntLines (const char* filename) {
  FILE *file;
  int s=0, lines=0; char c;
  
  if ((file = fopen(filename, "r")) == NULL) {
    printf("Could not open %s", filename);
    return 0;
  }
  lines++;
  while ((s = fread(&c, sizeof(char), 1, file)) != 0)
    if (c == '\n')
      lines++;
  fclose(file);
  
  return lines;
}
const char* webfile = "index1.html";
//char** webpage;
/* note countLines will ret total num strings and 256 is max length */
void readIndexHTML (const char* filename, char** wp) {
  FILE *file;
  int r = cntLines(filename), c = 256;
  int i, j=0; char ch;
  wp = (char **)malloc(r * sizeof(char));
  for (i=0; i<r; ++i) {
    wp[i] = (char *)malloc(c * sizeof(char));
  }

  if ((file = fopen(filename, "r")) == NULL) {
    printf("Could not open %s", filename);
    return;
  }
  for (i=0; i<r; ++i) {
    do {
      fread(&ch, sizeof(char), 1, file);
      wp[i][j] = ch; // or *(*(wp + i) + (j++)) = ch;
    } while (ch != '\n' && j<c);
  }
}

int main (int argc, char* argv[]) {
  //readIndexHTML(webfile, webpage);
  struct sockaddr_in server_addr, client_addr;
  socklen_t sin_len = sizeof(client_addr);
  int fd_server, fd_client;
  char buf[2048];
  int fdimg;
  int on = 1;

  if ((fd_server = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    exit(1);
  }

  setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(8123);

  if (bind(fd_server, (struct sockaddr*) & server_addr, sizeof(server_addr)) == -1) {
    perror("bind failed");
    close(fd_server);
    exit(1);
}

  if (listen(fd_server, 10) == -1) {
    perror("listen failed");
    close(fd_server);
    exit(1);
  }

  while(1) {
    if ( (fd_client = accept(fd_server, (struct sockaddr*) &client_addr, &sin_len)) == -1) {
      perror("connection failed...\n");
      continue;
    }

    printf("Got client connection\n");

    if (!fork()) {
      /* child process */
      close(fd_server);
      memset(buf, 0 , 2048);
      read(fd_client, buf, 2047);
      printf("%s\n", buf); // buf starts with GET /...

      if (!strncmp(buf, "GET /backimg.jpg", 16)) {
	if ((fdimg = open("backimg.jpg", O_RDONLY)) == -1) {
	  printf("failed to open backimg.jpg\n");
	}
	sendfile(fd_client, fdimg, NULL, 150600);
	close(fdimg);
      }
      else if(!strncmp(buf, "GET /img.jpg", 12)) { // for the length of str
	if ((fdimg = open("img.jpg", O_RDONLY)) == -1) {
	  printf("failed to open img.jpg\n");
	}
	printf("successfully opened img.jpg\n");
	sendfile(fd_client, fdimg, NULL, 20000);
	close(fdimg);
      }
      else {
	write(fd_client, webpage, sizeof(webpage)-1);
      }
      close(fd_client);
      printf("closing...\n");
      exit(0);
    }
    /* parent process */
    close(fd_client);
  }
  
  return 0;
}
