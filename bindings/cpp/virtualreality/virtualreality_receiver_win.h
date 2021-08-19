// (c) 2021 Okawo
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#ifndef RECEIVER_H
#define RECEIVER_H

#include <tchar.h>


#include <Ws2tcpip.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#include "virtualreality_util.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <numeric>


#include <thread>
#include <chrono>

#include <stdio.h>

#include <locale>
#include <windows.h>

namespace utilz {

  // this is needed cuz windows
  std::wstring s2w(const std::string& str)
  {
      int len;
      int slength = (int)str.length() + 1;
      len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, 0, 0);
      wchar_t* buf = new wchar_t[len];
      MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, buf, len);
      std::wstring result(buf);
      return buf;
  }

  static bool g_bDriverReceiver_wsastartup_happen = false;

  class SocketObj {
  public:
    std::string m_sIdMessage = "holla\n";

    SocketObj(std::string addr, int port=6969, int recvBuffSize=512): m_iExpectedMessageSize(recvBuffSize) {

      if (!g_bDriverReceiver_wsastartup_happen) {
        // init winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != NO_ERROR) {
          // log init error
          // printf("init error: %d\n", iResult);

          throw std::runtime_error("failed to init winsock");
        }
        g_bDriverReceiver_wsastartup_happen = true;
      }

      // create socket
      m_pSocketObject = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      if (m_pSocketObject == INVALID_SOCKET) {
        // log create error
        // printf("create error\n");
        WSACleanup();
        throw std::runtime_error("failed to create socket");
      }

      // addr details
      sockaddr_in addrDetails;
      addrDetails.sin_family = AF_INET;
      InetPton(AF_INET, addr.c_str(), &addrDetails.sin_addr.s_addr);
      addrDetails.sin_port = htons(port);

      // connect socket
      int iResult2 = connect(m_pSocketObject, (SOCKADDR *) & addrDetails, sizeof (addrDetails));
      if (iResult2 == SOCKET_ERROR) {
        // log connect error
        // printf("cennect error: %d\n", iResult);
        int iResult3 = closesocket(m_pSocketObject);
        if (iResult3 == SOCKET_ERROR)
          // log closesocket error
          // printf("closesocket error: %d\n", iResult);
        WSACleanup();
        throw std::runtime_error("failed to connect");
      }

    }

    ~SocketObj() {
      this->stop();
    }

    void start() {
      m_bThreadKeepAlive = true;
      this->send2(m_sIdMessage.c_str());

      m_pMyTread = new std::thread(my_thread_enter, this);

      if (!m_pMyTread || !m_bThreadKeepAlive) {
        // log failed to create recv thread
        // printf("thread start error\n");
        close();
        throw std::runtime_error("failed to crate receiver thread or thread already exited");
      }
    }

    void stop() {
      this->close();
      m_pCallback = &m_NullCallback;
      m_bThreadKeepAlive = false;
      if (m_pMyTread) {
        m_pMyTread->join();
        delete m_pMyTread;
        m_pMyTread = nullptr;
      }
    }

    void close() {
      if (m_pSocketObject != NULL) {
        int res = send2("CLOSE\n");
        int iResult = closesocket(m_pSocketObject);
        if (iResult == SOCKET_ERROR) {
          // log closesocket error
          // printf("closesocket error: %d\n", WSAGetLastError());
          WSACleanup();
          #ifdef __HOBO_VR_LOG
          hvr::Log("failed to close socket");
          #endif
          //throw std::runtime_error("failed to closesocket");
        }
        else
          WSACleanup();
      }

      m_pSocketObject = NULL;
    }

    int send2(const char* message) {
      return send(m_pSocketObject, message, (int)strlen(message), 0);
    }

    int send2(const char* message, int msg_len_bytes) {
      return send(m_pSocketObject, message, msg_len_bytes, 0);
    }

    void setCallback(Callback* pCb){
      m_pCallback = pCb;
    }

  private:
    bool m_bThreadKeepAlive = false;
    std::thread *m_pMyTread = nullptr;
    bool m_bThreadReset = false;

    int m_iExpectedMessageSize;

    SOCKET m_pSocketObject;

    Callback m_NullCallback;
    Callback* m_pCallback = &m_NullCallback;

    static void my_thread_enter(SocketObj *ptr) {
      ptr->my_thread();
    }

    void my_thread() {
      while (m_bThreadKeepAlive){
        m_bThreadReset = false;
        int numbit = 0, msglen;
        int l_iTempMsgSize = m_iExpectedMessageSize*4*2;
        char* l_cpRecvBuffer = new char[l_iTempMsgSize];



        while (m_bThreadKeepAlive && !m_bThreadReset) {
          try {
            msglen = receive_till_zero(m_pSocketObject, l_cpRecvBuffer, numbit, l_iTempMsgSize);

            if (msglen == -1 || m_bThreadReset) break;

            if (!m_bThreadReset)
              m_pCallback->OnPacket(l_cpRecvBuffer, msglen);

            remove_message_from_buffer(l_cpRecvBuffer, numbit, msglen);

            std::this_thread::sleep_for(std::chrono::microseconds(1));

          } catch(...) {

            break;
          }
        }
        delete[] l_cpRecvBuffer;


        // log end of recv thread

    }
    m_bThreadKeepAlive = false;

    }
  };

}; // namespace utilz

#endif // RECEIVER_H
