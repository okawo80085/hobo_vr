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

namespace SockReceiver {
  int receive_till_zero( SOCKET sock, char* buf, int& numbytes, int max_packet_size ) {
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

  void remove_message_from_buffer( char* buf, int& numbytes, int msglen ) {
    // thanks to https://stackoverflow.com/a/13528453/10190971
    // remove complete message from the buffer.
    memmove( buf, buf + msglen, numbytes - msglen );
    numbytes -= msglen;
  }

  std::string bufferToString(char* buffer, int bufflen)
  {
      std::string ret(buffer, bufflen);

      return ret;
  }

  std::vector<std::string> split_string(std::string text){
    std::istringstream iss(text);

    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                            std::istream_iterator<std::string>{}};

    return tokens;
  }

  std::vector<double> split2double(std::vector<std::string> split) {
    std::vector<double> out(split.size());
    std::transform(split.begin(), split.end(), out.begin(), [](const std::string& val)
    {
      return std::stod(val);
    });

    return out;
  }

  bool str_in_str(std::string a, std::string b) {
    for (auto i : a) {
      if (b.find(i) != std::string::npos) return true;
    }
    return false;
  }


  

};

#endif // RECEIVER_H