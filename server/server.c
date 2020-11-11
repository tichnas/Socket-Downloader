#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8000
#define PACKET_SIZE 4096

long long min(long long a, long long b) { return a < b ? a : b; }

long long myRead(int fd, char* buf, long long size) {
  long long done = 0, sz;
  int noConnection = 0;
  while (done < size) {
    sz = read(fd, buf + done, size - done);
    if (sz < 0) return -1;
    if (sz == 0) noConnection++;
    if (noConnection > 10000) {
      printf("\nConnection Timed Out (Client Exited)\n");
      return -2;
    }
    if (sz > 0) {
      done += sz;
      noConnection = 0;
    }
  }
  return size;
}

long long myWrite(int fd, char* buf, long long size) {
  long long done = 0, sz;
  while (done < size) {
    sz = write(fd, buf + done, size - done);
    done += sz;
    if (sz < 0) return -1;
  }
  return size;
}

int main() {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1;

  if (sockfd <= 0 || setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                &opt, sizeof(opt))) {
    perror("Error");
    return -1;
  }

  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);
  int addrlen = sizeof(address);

  if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0 ||
      listen(sockfd, 1) < 0) {
    perror("Error");
    return -1;
  }

  printf("Server Started\n");

  sockfd = accept(sockfd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

  if (sockfd < 0) {
    perror("Error");
    return -1;
  }

  printf("Client Connected\n");

  int fileFd;
  char str[PACKET_SIZE + 1] = {0};
  long long fileSize, sz;

  while (1) {
    // get file to read
    if (myRead(sockfd, str, PACKET_SIZE + 1) == -2) return 0;
    if (str[0] == '1') break;

    // send the size of file
    fileFd = open(str + 1, O_RDONLY);
    if (fileFd < 0) {
      perror("File Open Error");
      str[0] = '1';
      myWrite(sockfd, str, PACKET_SIZE + 1);
      continue;
    }
    fileSize = lseek(fileFd, 0, SEEK_END);
    lseek(fileFd, 0, SEEK_SET);
    if (fileSize < 0) {
      perror("File Read Error");
      str[0] = '1';
      myWrite(sockfd, str, PACKET_SIZE + 1);
      continue;
    }
    sprintf(str, "0%lld", fileSize);
    myWrite(sockfd, str, PACKET_SIZE + 1);

    // Read & Send file
    for (long long bytesRead = 0; bytesRead < fileSize;
         bytesRead += PACKET_SIZE) {
      sz = myRead(fileFd, str, min(PACKET_SIZE, fileSize - bytesRead));

      if (sz == -2) return 0;
      if (sz < 0) {
        perror("File Read Error");
        str[0] = '1';
        myWrite(sockfd, str, PACKET_SIZE + 1);
        break;
      }

      while (sz) {
        str[sz] = str[sz - 1];
        sz--;
      }
      str[0] = '0';
      myWrite(sockfd, str, PACKET_SIZE + 1);
    }
  }

  printf("Server Exited\n");
  return 0;
}