#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 6969

using namespace std;

double *convert2ss(char *buffer, int *len) {
  int spacesCount = 0;
  for (int i = 0; i < *len; i++) {
    if (buffer[i] == ' ') {
      spacesCount++;
    }
  }
  spacesCount++;

  string numbers[spacesCount];
  for (int i = 0; i < spacesCount; i++) {
    numbers[i] = "";
  }
  int index = 0;

  for (int i = 0; i < *len; i++) {
    if (buffer[i] == ' ') {
      index++;
    } else {
      numbers[index] += buffer[i];
    }
  }
  double *numz = new double[spacesCount];
  for (int i = 0; i < spacesCount; i++) {
    stringstream yeet(numbers[i]);
    yeet >> numz[i];
  }
  *len = spacesCount;
  return numz;
}

int main() {
  int sock = 0;
  struct sockaddr_in serv_addr;
  char *hello = "Hello from client\n";
  char buffer[1024] = {0};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("\n Socket creation error \n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
    printf("\nInvalid address/ Address not supported \n");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("\nConnection Failed \n");
    return -1;
  }
  send(sock, hello, strlen(hello), 0);
  printf("Hello message sent\n");
  auto valread = read(sock, buffer, 1024);
  if (valread < 0)
    return -1;

  int bufLen = 1024;
  char buffer[1024] = {0};
  double *z = convert2ss(buffer, &bufLen);
  for (int i = 0; i < bufLen; i++) {
    cout << z[i] + 5 << ", ";
  }
  delete z;
  z = nullptr;
  cout << '\n';
  return 0;
}