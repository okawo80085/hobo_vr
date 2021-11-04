// (c) 2021 Okawo
// This code is licensed under MIT license (see LICENSE for details)

#pragma once

#ifndef RECEIVER_H
#define RECEIVER_H



#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <iterator>

#include <thread>
#include <chrono>

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
// #include <sys/types.h>
#include <sys/socket.h>
// #include <netinet/in.h>
#include <netdb.h> 

#define SOCKET char //needed for a type check to be possible
#include "util.h"

namespace SockReceiver {

  class DriverReceiver {
  public:
    std::vector<std::string> m_vsDevice_list;
    std::vector<int> m_viEps;
    int m_iExpectedMessageSize;
    std::string m_sIdMessage = "hello\n";

    DriverReceiver(std::string expected_pose_struct, int port=6969, std::string addr="127.0.01") {
      std::regex rgx("[htc]");
      std::regex rgx2("[0-9]+");

      m_viEps = split_to_number<int>(get_rgx_vector(expected_pose_struct, rgx2));
      m_vsDevice_list = get_rgx_vector(expected_pose_struct, rgx);
      m_iExpectedMessageSize = std::accumulate(m_viEps.begin(), m_viEps.end(), 0);

      // placeholders
      int portno;
      struct sockaddr_in serv_addr;
      struct hostent *server;

      server = gethostbyname(addr.c_str()); // oh and did i mention that this is also convoluted as shit in winsock?
      portno = port;

      m_pSocketObject = socket(AF_INET, SOCK_STREAM, 0); // create socket, surprisingly winsock didn't fuck this up

      if (m_pSocketObject < 0) {
        //log and throw
#ifdef DRIVERLOG_H
          DriverLog("receiver opening socket error");
#endif
          throw std::runtime_error("failed to open socket");
      }

      if (server == NULL){
        //log and throw
#ifdef DRIVERLOG_H
          DriverLog("receiver bad host");
#endif
          throw std::runtime_error("bad host addr");
      }

      bzero((char *) &serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;

      // a long ass function call just to copy the addr into a socket addr struct, still better then winsock
      bcopy((char *)server->h_addr, 
           (char *)&serv_addr.sin_addr.s_addr,
           server->h_length);
      serv_addr.sin_port = htons(portno); // copy port to the same struct

      if (connect(m_pSocketObject,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) { // connect and check if successful
        //log and throw
#ifdef DRIVERLOG_H
          DriverLog("receiver failed to connect to host");
#endif
          throw std::runtime_error("connection error");
      }
    }

    ~DriverReceiver() {
      this->stop();
    }

     void start() {
      m_bThreadKeepAlive = true;
      this->send2(m_sIdMessage.c_str());

      this->m_pMyTread = new std::thread(this->my_thread_enter, this);

      if (!this->m_pMyTread || !m_bThreadKeepAlive) {
        // log failed to create recv thread
        // printf("thread start error\n");
        this->close_me();
#ifdef DRIVERLOG_H
        DriverLog("receiver thread start error\n");
#endif
        throw std::runtime_error("failed to crate receiver thread or thread already exited");
      }
    }

    void stop() {
      this->close_me();
      m_bThreadKeepAlive = false;
      // m_pCallback = &m_NullCallback;
      if (this->m_pMyTread) {
        this->m_pMyTread->join();
        delete this->m_pMyTread;
        this->m_pMyTread = nullptr;
      }
    }

    void close_me() {
      if (m_pSocketObject) {
        this->send2("CLOSE\n");
        close(m_pSocketObject);


      }

      m_pSocketObject = 0;
    }

    int send2(const char* message) {
      return write(m_pSocketObject, message, (int)strlen(message));
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

    // Callback m_NullCallback;
    // Callback* m_pCallback = &m_NullCallback;
    Callback* m_pCallback = nullptr;

    int m_pSocketObject;

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

            if (!m_bThreadReset && m_pCallback != nullptr)
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

}
#undef SOCKET


#endif // RECEIVER_H
