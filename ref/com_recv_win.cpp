#include <Ws2tcpip.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 1024
#define DEFAULT_PORT "6969"

// using namespace std;

namespace SP {

////////////////////////////////////////////////////////////////////////////
// socketPoser class
////////////////////////////////////////////////////////////////////////////

class socketPoser {
public:
  double *convert2ss(char *buffer, int *len);
  char *cleanCharList(char *buffer, int *len);
  int socSend(const char *buf, int len);
  int socRecv();
  int socClose();

  double *newPose;
  int returnStatus;

  socketPoser() {
    recvbuflen = DEFAULT_BUFLEN;
    bufLen = recvbuflen;
    ConnectSocket = INVALID_SOCKET;
    result = NULL;
    ptr = NULL;
    returnStatus = 0;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
      printf("WSAStartup failed with error: %d\n", iResult);
      returnStatus = iResult;
    } else {

      ZeroMemory(&hints, sizeof(hints));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;

      // Resolve the server address and port
      iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
      if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        returnStatus = iResult;
      } else {

        // Attempt to connect to an address until one succeeds
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

          // Create a SOCKET for connecting to server
          ConnectSocket =
              socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
          if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            returnStatus = iResult;
            break;
          }

          // Connect to server.
          iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
          if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
          }
          break;
        }

        freeaddrinfo(result);

        if (ConnectSocket == INVALID_SOCKET) {
          printf("Unable to connect to server!\n");
          WSACleanup();
          returnStatus = -1;
        }
      }
    }
  }

  ~socketPoser() { socClose(); }

private:
  WSADATA wsaData;
  SOCKET ConnectSocket;
  struct addrinfo *result;
  struct addrinfo *ptr;
  struct addrinfo hints;
  int iResult;
  int recvbuflen;
  char recvbuf[DEFAULT_BUFLEN];
  int bufLen;
};

int socketPoser::socSend(const char *buff, int len) {
  if (returnStatus == 0) {
    // Send an initial buffer
    iResult = send(ConnectSocket, buff, len, 0);
    if (iResult == SOCKET_ERROR) {
      printf("send failed with error: %d\n", WSAGetLastError());
      socClose();
      returnStatus = iResult;
    }
    printf("Bytes Sent: %ld\n", iResult);
  }

  return returnStatus;
}

int socketPoser::socRecv() {
  if (returnStatus == 0) {
    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    bufLen = iResult;
    if (iResult > 0) {
      printf("Bytes received: %d\n", iResult);
      newPose = convert2ss(recvbuf, &bufLen);
    } else if (iResult == 0) {
      printf("Connection closed\n");
      returnStatus = -1;
    } else {
      returnStatus = WSAGetLastError();
      printf("recv failed with error: %d\n", returnStatus);
    }
  }

  return returnStatus;
}

int socketPoser::socClose() {
  // cleanup
  // shutdown the connection since no more data will be sent
  iResult = shutdown(ConnectSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed with error: %d\n", WSAGetLastError());
    closesocket(ConnectSocket);
    WSACleanup();
    returnStatus = iResult;
  }

  closesocket(ConnectSocket);
  WSACleanup();

  return returnStatus;
}

char *socketPoser::cleanCharList(char *buffer, int *len) {
  int i;
  for (i = 0; i < *len && buffer[i] != '\0'; i++) {
    if (buffer[i] == '\n') {
      buffer[i] = ' ';
    }
  }
  *len = i - 1;
  return buffer;
}

double *socketPoser::convert2ss(char *buffer, int *len) {
  // auto buffer2 = cleanCharList(buffer, len);
  int spacesCount = 0;
  for (int i = 0; i < *len; i++) {
    if (buffer[i] == ' ') {
      spacesCount++;
    }
  }
  spacesCount++;

  std::string *numbers = new std::string[spacesCount];
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
    std::stringstream yeet(numbers[i]);
    yeet >> numz[i];
  }
  delete[] numbers;
  numbers = nullptr;
  *len = spacesCount;
  return numz;
}

////////////////////////////////////////////////////////////////////////////

} // namespace SP

int main() {
  SP::socketPoser one;
  int res = 0;
  res = one.socSend("hello\n", 6);

  if (res != 0) {
    res = one.socClose();
    return res;
  }

  res = one.socRecv();
  if (res != 0) {
    res = one.socClose();
    return res;
  }

  for (int i = 0; i < 7; i++) {
    std::cout << one.newPose[i] << ", ";
  }
  std::cout << std::endl;
  res = one.returnStatus;
  return res;
}