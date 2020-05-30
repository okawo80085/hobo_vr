#ifndef RECEIVER_H
#define RECEIVER_H

#pragma once

#include <Ws2tcpip.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <thread>
#include <chrono>

#include <stdio.h>

namespace SockReceiver {
  int receive_till_zero( SOCKET sock, char* buf, int& numbytes, int max_packet_size )
  {
    // receives a message until an end token is reached
    // thanks to https://stackoverflow.com/a/13528453/10190971
    int i = 0;
    do {
      // Check if we have a complete message
      for( ; i < numbytes; i++ ) {
        if( buf[i] == '\0' || buf[i] == '\n') {
          // \0 indicate end of message! so we are done
          return i + 1; // return length of message
        }
      }
      int n = recv( sock, buf + numbytes, max_packet_size - numbytes, 0 );
      if( n == -1 ) {
        return -1; // operation failed!
      }
      numbytes += n;
    } while( true );
  }

  void remove_message_from_buffer( char* buf, int& numbytes, int msglen )
  {
    // remove complete message from the buffer.
    // thanks to https://stackoverflow.com/a/13528453/10190971
    memmove( buf, buf + msglen, numbytes - msglen );
    numbytes -= msglen;
  }

  std::string buffer_to_string(char* buffer, int bufflen)
  {
    std::string ret(buffer, bufflen);

    return ret;
  }

  std::vector<std::string> split_string(std::string text)
  {
    std::istringstream iss(text);

    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                            std::istream_iterator<std::string>{}};

    return tokens;
  }

  std::vector<double> split_to_double(std::vector<std::string> split)
  {
    // converts a vector of strings to a vector of doubles
    std::vector<double> out(split.size());
    try {
      std::transform(split.begin(), split.end(), out.begin(), [](const std::string& val)
      {
        return std::stod(val);
      });
    } catch (...) {
      out = {0.0, 0.0};
    }

    return out;
  }

  bool strings_share_characters(std::string a, std::string b)
  {
    for (auto i : b) {
      if (a.find(i) != std::string::npos) return true;
    }
    return false;
  }


  class DriverReceiver{
  public:
    DriverReceiver(int expected_pose_size, const char* addr="127.0.0.1", int port=6969) {
      this->eps = expected_pose_size;
      this->threadKeepAlive = false;

      for (int i=0; i<this->eps; i++) {
        this->newPose.push_back(0.0);
      }

      // init winsock
      WSADATA wsaData;
      int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
      if (iResult != NO_ERROR) {
          // log init error
          // printf("init error: %d\n", iResult);
          throw std::runtime_error("failed to init winsock");
      }

      // create socket
      this->mySoc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

      if (this->mySoc == INVALID_SOCKET) {
          // log create error
          // printf("create error\n");
          WSACleanup();
          throw std::runtime_error("failed to create socket");
      }

      // addr details
      sockaddr_in addrDetails;
      addrDetails.sin_family = AF_INET;
      addrDetails.sin_addr.s_addr = inet_addr(addr);
      addrDetails.sin_port = htons(port);

      // connect socket
      iResult = connect(this->mySoc, (SOCKADDR *) & addrDetails, sizeof (addrDetails));
      if (iResult == SOCKET_ERROR) {
          // log connect error
          // printf("cennect error: %d\n", iResult);
          iResult = closesocket(this->mySoc);
          if (iResult == SOCKET_ERROR)
              // log closesocket error
              // printf("closesocket error: %d\n", iResult);
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
        throw std::runtime_error("failed to crate receiver thread or thread already exited");
      }
    }

    void stop() {
      this->close();
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
            WSACleanup();
            throw std::runtime_error("failed to closesocket");
        }
        else
          WSACleanup();
      }

      this->mySoc = NULL;
    }

    std::vector<double> get_pose() {return this->newPose;}

    int send2(const char* message) {
      return send(this->mySoc, message, (int)strlen(message), 0);
    }

  private:
    std::vector<double> newPose;
    int eps;

    bool threadKeepAlive;
    std::thread *m_pMyTread;

    SOCKET mySoc;

    static void my_thread_enter(DriverReceiver *ptr) {
      ptr->my_thread();
    }

    void my_thread() {
      char mybuff[2048];
      int numbit = 0, msglen, packErr;

      // std::string b = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ{}[]()=+<>/,";

      while (this->threadKeepAlive) {
        try {
          msglen = receive_till_zero(this->mySoc, mybuff, numbit, 2048);

          if (msglen == -1) break;

          auto packet = buffer_to_string(mybuff, msglen-1);

          remove_message_from_buffer(mybuff, numbit, msglen);

          packErr = this->_handle(packet);
          // if (!packErr) {
          //   // log packet process error
          // }

          std::this_thread::sleep_for(std::chrono::microseconds(1));


        } catch(...) {
          break;
        }
      }

      this->threadKeepAlive = false;

      // log end of recv thread

    }

    int _handle(std::string packet) {
      std::vector<double> temp = split_to_double(split_string(packet));

      if (temp.size() == this->eps) {
        this->newPose = temp;
        return 1;
      }

      else
        return 0;
    }
  };

};

#endif // RECEIVER_H