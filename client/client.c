#include <arpa/inet.h>
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
    if (sz < 0) {
      printf("\n\033[0;31mUnexpected Error\033[0;00m\n");
      return -2;
    }
    if (sz == 0) noConnection++;
    if (noConnection > 10000) {
      printf("\n\033[0;31mConnection Timed Out (Server Exited)\033[0;00m\n");
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

int getFile(int sockfd, char* fileName) {
  char str[PACKET_SIZE + 1];

  // Request server for file
  sprintf(str, "0%s", fileName);
  myWrite(sockfd, str, PACKET_SIZE + 1);

  // Get file size
  if (myRead(sockfd, str, PACKET_SIZE + 1) == -2) return -1;

  if (str[0] == '1') {
    printf("\033[0;31m%s: File doesn't exist / Read Error\033[0;00m\n",
           fileName);
    return 0;
  }
  long long fileSize = atoll(str + 1);

  // Create file and start reading
  int fileFd = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0600);
  printf("\033[0;33m");
  for (long long bytesRead = 0; 1; bytesRead += PACKET_SIZE) {
    if (bytesRead >= fileSize) {
      printf("\r\033[0;32m%s: 100%%   ", fileName);
      break;
    }

    printf("\r%s: %.2f%%", fileName, 100.0 * bytesRead / fileSize);

    if (myRead(sockfd, str, PACKET_SIZE + 1) == -2) return -1;

    if (str[0] == '1') {
      printf("\033[0;31m%s Read Error\n", fileName);
      break;
    }

    myWrite(fileFd, str + 1, min(PACKET_SIZE, fileSize - bytesRead));
  }
  printf("\033[0;00m\n");
  return 0;
}

void getCommand(char** args) {
  long unsigned size = 1024;
  char* str = NULL;

  int len = getline(&str, &size, stdin);
  str[len - 1] = '\0';

  args[0] = NULL;
  char* token = strtok(str, " \t");

  for (int i = 0; token != NULL; i++) {
    args[i] = token;
    args[i + 1] = NULL;
    token = strtok(NULL, " \t");
  }
}

int main(int argc, char** argv) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd <= 0) {
    perror("Error");
    return -1;
  }

  struct sockaddr_in address;
  memset(&address, '0', sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(PORT);

  if (inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) <= 0) {
    printf("Invalid address/ Address not supported\n");
    return -1;
  }

  if (connect(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    printf("Connection Failed\n");
    return -1;
  }

  sockfd = -1;

  char** args = NULL;

  while (1) {
    printf("\033[0;34mclient>\033[0;00m ");
    args = malloc(20 * sizeof(char*));
    getCommand(args);

    if (args[0] == NULL) continue;

    if (!strcmp(args[0], "exit")) {
      printf("..\n");
      char str[PACKET_SIZE + 1];
      str[0] = '1';
      myWrite(sockfd, str, PACKET_SIZE + 1);
      printf(".\n");
      return 0;
    } else if (!strcmp(args[0], "get")) {
      for (int i = 1; args[i] != NULL; i++) {
        if (getFile(sockfd, args[i]) < 0) return 0;
      }
    } else {
      printf("\033[0;31mInvalid Command\033[0;00m\n");
    }
  }

  return 0;
}
