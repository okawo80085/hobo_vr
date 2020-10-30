#pragma once

#ifndef RECEIVER_H
#define RECEIVER_H

#include <tchar.h>


#include <Ws2tcpip.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

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

  class DriverReceiver {
  public:
    std::vector<std::string> device_list;
    std::vector<int> eps;
    int m_iBuffSize;

    DriverReceiver(std::string expected_pose_struct, int port=6969) {
      std::regex rgx("[htc]");
      std::regex rgx2("[0-9]+");

      this->eps = split_to_number<int>(get_rgx_vector(expected_pose_struct, rgx2));
      this->device_list = get_rgx_vector(expected_pose_struct, rgx);
      this->threadKeepAlive = false;
      m_iBuffSize = std::accumulate(this->eps.begin(), this->eps.end(), 0);

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

      // create socket
      this->mySoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      if (this->mySoc == INVALID_SOCKET) {
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
      iResult = connect(this->mySoc, (SOCKADDR *) & addrDetails, sizeof (addrDetails));
      if (iResult == SOCKET_ERROR) {
        // log connect error
        // printf("cennect error: %d\n", iResult);
#ifdef DRIVERLOG_H
        DriverLog("receiver connect error: %d\n", WSAGetLastError());
#endif
        iResult = closesocket(this->mySoc);
        if (iResult == SOCKET_ERROR)
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
      this->threadKeepAlive = true;
      this->send2("hello\n");

      this->m_pMyTread = new std::thread(this->my_thread_enter, this);

      if (!this->m_pMyTread || !this->threadKeepAlive) {
        // log failed to create recv thread
        // printf("thread start error\n");
        this->close();
#ifdef DRIVERLOG_H
        DriverLog("receiver thread start error\n");
#endif
        throw std::runtime_error("failed to crate receiver thread or thread already exited");
      }
    }

    void stop() {
      this->close();
      m_pCallback = &m_NullCallback;
      this->threadKeepAlive = false;
      if (this->m_pMyTread) {
        this->m_pMyTread->join();
        delete this->m_pMyTread;
        this->m_pMyTread = nullptr;
      }
    }

    void close() {
      if (this->mySoc != NULL) {
        int res = this->send2("CLOSE\n");
        int iResult = closesocket(this->mySoc);
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

      this->mySoc = NULL;
    }

    int send2(const char* message) {
      return send(this->mySoc, message, (int)strlen(message), 0);
    }

    void setCallback(Callback* pCb){
      m_pCallback = pCb;
    }

  private:

    bool threadKeepAlive;
    std::thread *m_pMyTread;

    SOCKET mySoc;

    Callback m_NullCallback;
    Callback* m_pCallback = &m_NullCallback;

    static void my_thread_enter(DriverReceiver *ptr) {
      ptr->my_thread();
    }

    void my_thread() {
      int numbit = 0, msglen;
      int tempSize = m_iBuffSize*4*2;
      char* mybuff = new char[tempSize];

#ifdef DRIVERLOG_H
      DriverLog("receiver thread started\n");
#endif

      while (this->threadKeepAlive) {
        try {
          msglen = receive_till_zero(this->mySoc, mybuff, numbit, tempSize);

          if (msglen == -1) break;

          m_pCallback->OnPacket(mybuff, msglen);

          remove_message_from_buffer(mybuff, numbit, msglen);

          std::this_thread::sleep_for(std::chrono::microseconds(1));

        } catch(...) {
          break;
        }
      }
      delete[] mybuff;

      this->threadKeepAlive = false;

      // log end of recv thread
#ifdef DRIVERLOG_H
      DriverLog("receiver thread ended\n");
#endif

    }

    // int _handle(std::string packet) {
    //   auto temp = split_pk(split_to_number<double>(split_string(packet)), this->eps);

    //   if (get_poses_shape(temp) == this->eps) {
    //     this->newPose = temp;
    //     return -1;
    //   }

    //   else
    //     return temp.size();
    // }
  };

};

#endif // RECEIVER_H
