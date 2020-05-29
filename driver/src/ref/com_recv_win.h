#ifndef COM_RECV_WIN_H
#define COM_RECV_WIN_H
#endif

#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

#include <queue>
#include <thread>
#include <mutex>


#include <Ws2tcpip.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 600
#define DEFAULT_PORT "6969"

// using namespace std;

namespace SP {
////////////////////////////////////////////////////////////////////////////
// socketPoser class
////////////////////////////////////////////////////////////////////////////

class socketPoser {
public:
  int returnStatus;
  int bufLen;
  int expectedPoseSize;

  void convert2ss(char *buffer, int *len) {
    // auto buffer2 = cleanCharList(buffer, len);
    int spaceCount = 0;
    for (int i = 0; i < *len; i++) {
      if (buffer[i] == ' ') {
        spaceCount++;
      }
    }
    spaceCount++;
    *len = spaceCount;
    if (spaceCount > expectedPoseSize){
      spaceCount = expectedPoseSize;
    }

    std::stringstream ss;

    ss.precision(10);

    ss << buffer;

    double temp;
    newPose.clear();
    for (int i=0; i<spaceCount; i++){
      ss >> temp;
      newPose.push_back(temp);
    }
  }

  void cleanCharList(char *buffer, int *len) {
    int i;
    for (i = 0; i < *len && buffer[i] != '\0'; i++) {
      if (buffer[i] == '\n') {
        buffer[i] = ' ';
      }
    }
    *len = i - 1;
  }

  int socSend(const char *buff, int len) {
    if (returnStatus == 0) {
      // Send an initial buffer
      iResult = send(ConnectSocket, buff, len, 0);
      if (iResult == SOCKET_ERROR) {
        DriverLog("send failed with error: %d\n", WSAGetLastError());
        socClose();
      }else{
        DriverLog("Bytes Sent: %ld\n", iResult);
      }
    }

    return returnStatus;
  }

  int socRecv() {
    if (returnStatus == 0) {
      m_mtx.lock();
      iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
      bufLen = iResult;
      m_mtx.unlock();
      if (iResult == 0) {
        DriverLog("Connection closed\n");
        returnStatus = -1;
      } else if (iResult < 0) {
        returnStatus = WSAGetLastError();
        DriverLog("recv failed with error: %d\n", returnStatus);
      }
    }

    return returnStatus;
  }

  int socClose() {
    // cleanup
    // shutdown the connection since no more data will be sent
    socSend("CLOSE\n", 6);
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
      DriverLog("shutdown failed with error: %d\n", WSAGetLastError());
      closesocket(ConnectSocket);
      WSACleanup();
      returnStatus = iResult;
    }

    closesocket(ConnectSocket);
    WSACleanup();

    return returnStatus;
  }

  void start() {
    m_bMyThreadKeepAlive = true;
    socSend("hello\n", 6);
    m_pMyTread = new std::thread(threadEnter, this);

    if (!m_pMyTread) {
      DriverLog("failed to start listening thread\n");
      returnStatus = -1;
    }
  }

  std::vector<double> getPose() {
    m_mtx.lock();
    for (int i=0; i<bufLen; i++) {
      copybuf[i] = recvbuf[i];
    }
    bufLen2 = bufLen;
    m_mtx.unlock();
    if (bufLen2 > 0){
      // cleanCharList(recvbuf, &bufLen);
      convert2ss(copybuf, &bufLen2);
      if (bufLen2 != expectedPoseSize) {
        DriverLog("received pose packet size mismatch, %d expected but got %d, "
                  "returning null",
                  expectedPoseSize, bufLen2);
        // std::cout << "poses missmatch " << bufLen2 << '\n';

        if (bufLen2 < expectedPoseSize) {
          newPose.clear();
          for (int i=0; i<expectedPoseSize; i++){
            newPose.push_back(0.0);
          }
        }
      }
    }
    return newPose;
  }

  socketPoser(int i_expectedPoseSize) {
    m_bMyThreadKeepAlive = false;
    m_pMyTread = nullptr;

    expectedPoseSize = i_expectedPoseSize;
    recvbuflen = DEFAULT_BUFLEN;
    bufLen = recvbuflen;
    ConnectSocket = INVALID_SOCKET;
    result = NULL;
    ptr = NULL;
    returnStatus = 0;
    newPose.clear();
    for (int i=0; i<expectedPoseSize; i++){
      newPose.push_back(0.0);
    }


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
      DriverLog("WSAStartup failed with error: %d\n", iResult);
      returnStatus = iResult;
    } else {

      ZeroMemory(&hints, sizeof(hints));
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_protocol = IPPROTO_TCP;

      // Resolve the server address and port
      iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
      if (iResult != 0) {
        DriverLog("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        returnStatus = iResult;
      } else {

        // Attempt to connect to an address until one succeeds
        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

          // Create a SOCKET for connecting to server
          ConnectSocket =
              socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
          if (ConnectSocket == INVALID_SOCKET) {
            DriverLog("socket failed with error: %ld\n", WSAGetLastError());
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
          DriverLog("Unable to connect to server!\n");
          WSACleanup();
          returnStatus = -1;
        }

        // unsigned long iMode = 21;

        // iResult = ioctlsocket(ConnectSocket, FIONBIO, &iMode);
        // if (iResult != NO_ERROR)
        //  DriverLog("ioctlsocket failed with error: %ld\n", iResult);
      }
    }
  }

  ~socketPoser() {
    m_bMyThreadKeepAlive = false;
    if (m_pMyTread) {
      m_pMyTread->join();
      delete m_pMyTread;
      m_pMyTread = nullptr;
    }
    socClose();
  }

private:
  WSADATA wsaData;
  SOCKET ConnectSocket;
  struct addrinfo *result;
  struct addrinfo *ptr;
  struct addrinfo hints;
  int iResult;
  int recvbuflen;
  char recvbuf[DEFAULT_BUFLEN];
  char copybuf[DEFAULT_BUFLEN];
  int bufLen2;

  std::vector<double> newPose;

  std::mutex m_mtx;

  bool m_bMyThreadKeepAlive;
  std::thread *m_pMyTread;
  std::queue<std::vector<double>> qq;

  static void threadEnter(socketPoser* t) {
    t->listenThread();
  }
  void listenThread() {
    DWORD dwThreadPri;

    if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
    {
      DriverLog("failed to enter THREAD_PRIORITY_TIME_CRITICAL mode for listen thread: %d\n", GetLastError());
    }

    dwThreadPri = GetThreadPriority(GetCurrentThread());
    DriverLog("current listen thread priority: %d", dwThreadPri);


    while (m_bMyThreadKeepAlive) {
      if (returnStatus != 0){
        break;
      }
      socRecv();

      // if (!qq.empty()) {
      //   qq.pop();
      // }
      // qq.push(newPose);
      std::this_thread::sleep_for(std::chrono::microseconds(1));
    }

    DriverLog("listen thread finished with status: %d", returnStatus);
  }
};

////////////////////////////////////////////////////////////////////////////
} // namespace SP