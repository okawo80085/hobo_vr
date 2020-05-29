#ifndef RECEIVER_H
#define RECEIVER_H

#pragma once

#include <Ws2tcpip.h>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

namespace hobo {
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
};

#endif // RECEIVER_H