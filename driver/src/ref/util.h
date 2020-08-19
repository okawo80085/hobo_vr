#ifndef UTIL_H
#define UTIL_H
#pragma once

#include <vector>
#include <iterator>
#include <regex>
#include <string>
#include <sstream>

namespace SockReceiver {
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

  std::string first_rgx_match(std::string ss, std::regex rgx) {
    std::smatch mm;
    if (std::regex_search(ss, mm, rgx))
      return mm[0];
    return "";
  }

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

  template <typename T>
  std::vector<int> get_poses_shape(std::vector<std::vector<T>> arr) {
    std::vector<int> out;
    for (auto i : arr)
      out.push_back(i.size());
    return out;
  }

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

  bool strings_share_characters(std::string a, std::string b)
  {
    for (auto i : b) {
      if (a.find(i) != std::string::npos) return true;
    }
    return false;
  }
}

#endif // UTIL_H