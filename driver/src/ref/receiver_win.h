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

#include "util.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <numeric>


#include <thread>
#include <chrono>

#include <stdio.h>

namespace SockReceiver {
  static bool g_bDriverReceiver_wsastartup_happen = false;

  class DriverReceiver {
  public:
    std::vector<std::string> m_vsDevice_list;
    std::vector<int> m_viEps;
    int m_iExpectedMessageSize;
    std::string m_sIdMessage = "hello\n";

    DriverReceiver(std::string expected_pose_struct, int port=6969) {
      std::regex rgx("[htc]");
      std::regex rgx2("[0-9]+");

      m_viEps = split_to_number<int>(get_rgx_vector(expected_pose_struct, rgx2));
      m_vsDevice_list = get_rgx_vector(expected_pose_struct, rgx);
      m_iExpectedMessageSize = std::accumulate(m_viEps.begin(), m_viEps.end(), 0);

      if (!g_bDriverReceiver_wsastartup_happen) {
        // init winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != NO_ERROR) {
          // log init error
          // printf("init error: %d\n", iResult);
  #ifdef DRIVERLOG_H
          DriverLog("receiver init error: %d\n", WSAGetLastError());
  #endif
          throw std::runtime_error("failed to init winsock");
        }
        g_bDriverReceiver_wsastartup_happen = true;
      }

      // create socket
      m_pSocketObject = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      if (m_pSocketObject == INVALID_SOCKET) {
        // log create error
        // printf("create error\n");
#ifdef DRIVERLOG_H
        DriverLog("receiver create error: %d\n", WSAGetLastError());
#endif
        WSACleanup();
        throw std::runtime_error("failed to create socket");
      }

      // addr details
      sockaddr_in addrDetails;
      addrDetails.sin_family = AF_INET;
      InetPton(AF_INET, _T("127.0.0.1"), &addrDetails.sin_addr.s_addr);
      addrDetails.sin_port = htons(port);

      // connect socket
      int iResult2 = connect(m_pSocketObject, (SOCKADDR *) & addrDetails, sizeof (addrDetails));
      if (iResult2 == SOCKET_ERROR) {
        // log connect error
        // printf("cennect error: %d\n", iResult);
#ifdef DRIVERLOG_H
        DriverLog("receiver connect error: %d\n", WSAGetLastError());
#endif
        int iResult3 = closesocket(m_pSocketObject);
        if (iResult3 == SOCKET_ERROR)
          // log closesocket error
          // printf("closesocket error: %d\n", iResult);
#ifdef DRIVERLOG_H
          DriverLog("receiver connect error: %d\n", WSAGetLastError());
#endif
        WSACleanup();
        throw std::runtime_error("failed to connect");
      }

    }

    ~DriverReceiver() {
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
#ifdef DRIVERLOG_H
        DriverLog("receiver thread start error\n");
#endif
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
#ifdef DRIVERLOG_H
          DriverLog("receiver closesocket error: %d\n", WSAGetLastError());
#endif
          WSACleanup();
          throw std::runtime_error("failed to closesocket");
        }
        else
          WSACleanup();
      }

      m_pSocketObject = NULL;
    }

    int send2(const char* message) {
      return send(m_pSocketObject, message, (int)strlen(message), 0);
    }

    void setCallback(Callback* pCb){
      m_pCallback = pCb;
    }

    void UpdateParams(std::string new_udu_string) {
      std::regex rgx("[htc]");
      std::regex rgx2("[0-9]+");

      m_viEps = split_to_number<int>(get_rgx_vector(new_udu_string, rgx2));
      m_vsDevice_list = get_rgx_vector(new_udu_string, rgx);
      m_iExpectedMessageSize = std::accumulate(m_viEps.begin(), m_viEps.end(), 0);

      m_bThreadReset = true;
    }

    void UpdateParams(std::vector<std::string> newDeviceList, std::vector<int> newEps) {
      m_viEps = newEps;
      m_vsDevice_list = newDeviceList;
      m_iExpectedMessageSize = std::accumulate(m_viEps.begin(), m_viEps.end(), 0);

      m_bThreadReset = true;
    }

  private:
    bool m_bThreadKeepAlive = false;
    std::thread *m_pMyTread = nullptr;
    bool m_bThreadReset = false;

    SOCKET m_pSocketObject;

    Callback m_NullCallback;
    Callback* m_pCallback = &m_NullCallback;

    static void my_thread_enter(DriverReceiver *ptr) {
      ptr->my_thread();
    }

    void my_thread() {
      while (m_bThreadKeepAlive){
        m_bThreadReset = false;
        int numbit = 0, msglen;
        int l_iTempMsgSize = m_iExpectedMessageSize*4*10;
        char* l_cpRecvBuffer = new char[l_iTempMsgSize];

      #ifdef DRIVERLOG_H
            DriverLog("receiver thread started\n");
      #endif

        while (m_bThreadKeepAlive && !m_bThreadReset) {
          try {
            msglen = receive_till_zero(m_pSocketObject, l_cpRecvBuffer, numbit, l_iTempMsgSize);

            if (msglen == -1 || m_bThreadReset) break;

            if (!m_bThreadReset)
              m_pCallback->OnPacket(l_cpRecvBuffer, msglen);

            remove_message_from_buffer(l_cpRecvBuffer, numbit, msglen);

            std::this_thread::sleep_for(std::chrono::microseconds(1));

          } catch(...) {
            #ifdef DRIVERLOG_H
            DriverLog("receiver thread error");
            #endif
            break;
          }
        }
        delete[] l_cpRecvBuffer;


        // log end of recv thread
      #ifdef DRIVERLOG_H
            DriverLog("receiver thread ended\n");
      #endif
    }
    m_bThreadKeepAlive = false;

    }
  };

};

#endif // RECEIVER_H
