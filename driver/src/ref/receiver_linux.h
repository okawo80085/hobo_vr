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
    std::vector<std::string> device_list;
    std::vector<int> eps;
    int m_iBuffSize;

    DriverReceiver(std::string expected_pose_struct, char *port="6969", char* addr="127.0.01") {
      std::regex rgx("[htc]");
      std::regex rgx2("[0-9]+");

      this->eps = split_to_number<int>(get_rgx_vector(expected_pose_struct, rgx2));
      this->device_list = get_rgx_vector(expected_pose_struct, rgx);
      this->threadKeepAlive = false;
      m_iBuffSize = std::accumulate(this->eps.begin(), this->eps.end(), 0);

      // placeholders
      int portno, n;
      struct sockaddr_in serv_addr;
      struct hostent *server;

      server = gethostbyname(addr); // oh and did i mention that this is also convoluted as shit in winsock?
      portno = atoi(port);

      this->mySoc = socket(AF_INET, SOCK_STREAM, 0); // create socket, surprisingly winsock didn't fuck this up

      if (this->mySoc < 0) {
        //log and throw
#ifdef DRIVERLOG_H
          DriverLog("receiver opening socket error");
#endif
          this->mySoc = NULL;
          throw std::runtime_error("failed to open socket");
      }

      if (server == NULL){
        //log and throw
#ifdef DRIVERLOG_H
          DriverLog("receiver bad host");
#endif
          this->mySoc = NULL;
          throw std::runtime_error("bad host addr");
      }

      bzero((char *) &serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;

      // a long ass function call just to copy the addr into a socket addr struct, still better then winsock
      bcopy((char *)server->h_addr, 
           (char *)&serv_addr.sin_addr.s_addr,
           server->h_length);
      serv_addr.sin_port = htons(portno); // copy port to the same struct

      if (connect(this->mySoc,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) { // connect and check if successful
        //log and throw
#ifdef DRIVERLOG_H
          DriverLog("receiver failed to connect to host");
#endif
          this->mySoc = NULL;
          throw std::runtime_error("connection error");
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
        this->close_me();
#ifdef DRIVERLOG_H
        DriverLog("receiver thread start error\n");
#endif
        throw std::runtime_error("failed to crate receiver thread or thread already exited");
      }
    }

    void stop() {
      this->close_me();
      this->threadKeepAlive = false;
      m_pCallback = &m_NullCallback;
      if (this->m_pMyTread) {
        this->m_pMyTread->join();
        delete this->m_pMyTread;
        this->m_pMyTread = nullptr;
      }
    }

    void close_me() {
      if (this->mySoc != NULL) {
        int res = this->send2("CLOSE\n");
        close(this->mySoc);

      }

      this->mySoc = NULL;
    }

    int send2(const char* message) {
      return write(this->mySoc, message, (int)strlen(message));
    }

    void setCallback(Callback* pCb){
      m_pCallback = pCb;
    }

  private:

    bool threadKeepAlive;
    std::thread *m_pMyTread;

    Callback m_NullCallback;
    Callback* m_pCallback = &m_NullCallback;

    int mySoc;

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
#undef SOCKET


#endif // RECEIVER_H
