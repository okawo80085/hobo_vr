#pragma once

#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <iterator>
#include <regex>
#include <string>
#include <sstream>

namespace SockReceiver {
  //can receive packets ending with \t\r\n using either winsock2 or unix sockets
  template <typename T>
  int receive_till_zero( T sock, char* buf, int& numbytes, int max_packet_size )
  {
    // receives a message until an end token is reached
    // thanks to https://stackoverflow.com/a/13528453/10190971
    int i = 0;
    int n=-1;
    do {
      // Check if we have a complete message
      for( ; i < numbytes-2; i++ ) {
        if((buf[i] == '\t' && buf[i+1] == '\r' && buf[i+2] == '\n')) {
          // \0 indicate end of message! so we are done
          return i + 3; // return length of message
        }
      }
      if constexpr(std::is_same<T, SOCKET>::value)
      {
        n = recv( sock, buf + numbytes, max_packet_size - numbytes, 0 );
      }
      else if constexpr(std::is_same<T, int>::value)
      {
        n = read( sock, buf + numbytes, max_packet_size - numbytes);
      } else {
        #ifdef DRIVERLOG_H
        DriverLog("that is not a socket you retard");
        #endif
        throw std::runtime_error("bruh, thats not a raw socket");
      }

      if( n == -1 ) {
        return -1; // operation failed!
      }
      numbytes += n;
    } while( true );
  }

  // reshapes packet vector into shape inxs
  template <typename T>
  std::vector<std::vector<T>> split_pk(std::vector<T> arr, std::vector<int> inxs) {
    std::vector<std::vector<T>> out;
    int oof = 0;
    int ms = arr.size();
    for (int i : inxs) {
      if (oof + i <= ms) {
        std::vector<T> temp(arr.begin() + oof, arr.begin() + oof + i);
        out.push_back(temp);
        oof += i;
      }
      else {
        std::vector<T> temp(arr.begin() + oof, arr.end());
        out.push_back(temp);
        break;
      }

    }
    return out;
  }

  // returns the found text
  std::string first_rgx_match(std::string ss, std::regex rgx) {
    std::smatch mm;
    if (std::regex_search(ss, mm, rgx))
      return mm[0];
    return "";
  }

  // get regex vector
  std::vector<std::string> get_rgx_vector(std::string ss, std::regex rgx) {
    std::sregex_token_iterator iter(ss.begin(),
        ss.end(),
        rgx,
        0);
    std::sregex_token_iterator end;

    std::vector<std::string> out;
    for (; iter != end; ++iter)
      out.push_back((std::string)*iter);
    return out;
  }

  // split by regex
  std::vector<std::string> split_by_rgx(std::string ss, std::regex rgx) {
    std::sregex_token_iterator iter(ss.begin(),
        ss.end(),
        rgx,
        -1);
    std::sregex_token_iterator end;

    std::vector<std::string> out;
    for (; iter != end; ++iter)
      out.push_back((std::string) * iter);
    return out;
  }

  // gets the shape of a 2D vector
  template <typename T>
  std::vector<int> get_poses_shape(std::vector<std::vector<T>> arr) {
    std::vector<int> out;
    for (auto i : arr)
      out.push_back(i.size());
    return out;
  }

  // convert vector of strings to type
  template <typename T>
  std::vector<T> split_to_number(std::vector<std::string> split)
  {
    // converts a vector of strings to a vector of doubles
    std::vector<T> out(split.size());
    try {
      std::transform(split.begin(), split.end(), out.begin(), [](const std::string& val)
          {
            return std::stod(val);
          });
    }
    catch (...) {
      out = { (T)0,(T) 0 };
    }

    return out;
  }

  // something socket related idk
  void remove_message_from_buffer( char* buf, int& numbytes, int msglen )
  {
    // remove complete message from the buffer.
    // thanks to https://stackoverflow.com/a/13528453/10190971
    memmove( buf, buf + msglen, numbytes - msglen );
    numbytes -= msglen;
  }

  // char buffer to string
  std::string buffer_to_string(char* buffer, int bufflen)
  {
    std::string ret(buffer, bufflen);

    return ret;
  }

  // splits string somehow
  std::vector<std::string> split_string(std::string text)
  {
    std::istringstream iss(text);

    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                            std::istream_iterator<std::string>{}};

    return tokens;
  }

  // yes
  bool strings_share_characters(std::string a, std::string b)
  {
    for (auto i : b) {
      if (a.find(i) != std::string::npos) return true;
    }
    return false;
  }

  class Callback {
  public:
    virtual void OnPacket(char* buff, int len) {};
  };
}

#endif // UTIL_H