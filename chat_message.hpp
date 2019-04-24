//
// chat_message.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <ncurses.h>

class chat_message
{
public:
  enum { header_length = 66 };
  enum { max_body_length = 512 };

  chat_message()
    : body_length_(0), new_room_number(0), chat_room_number(0), command(0), username{'\0'}, chatname_new{'\0'}, chatname_old{'\0'}
  {
  }


  const char* data() const
  {
    return data_;
  }


  char* data()
  {
    return data_;
  }

  std::size_t length() const
  {
    return header_length + body_length_;
  }

  const char* body() const
  {
    return data_ + header_length;
  }

  char* body()
  {
    return data_ + header_length;
  }

  std::size_t body_length() const
  {
    return body_length_;
  }

  void body_length(std::size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > max_body_length)
      body_length_ = max_body_length;
  }

  void make_username(char* name)
  {
    std::strcpy(username, name);
  }


  void set_nrn(int nrn)
  {
    new_room_number = nrn;
  }

  void set_crn(int crn)
  {
    chat_room_number = crn;
  }

  void set_cmd(int cmd)
  {
    command = cmd;
  }

  void set_chatname_current(char* cn)
  {
    std::strncpy(chatname_old, cn, 20);
  }

  void set_chatname_new(char* cn)
  {
    std::strncpy(chatname_new, cn, 20);
  }

  //Gets the chatroom number of a message
  int decode_crn()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    header[12] = '\0';
    int crn_offset = 8;
    int crn = std::atoi(header + crn_offset);

    return crn;
  }

  int decode_command()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    //So it only reads the body size part of the header
    header[16] = '\0';
    return std::atoi(header+12);

  }

  char* decode_user()
  {
    char header[header_length + 1] = {0};
    std::strncat(header, data_, header_length);
    for(int i=16; i < 26; i++)
    {
      if (header[i] == ' ')
      {
        header[i] = '\0';
      }
    }
    header[26] = '\0';
    return (header+16);
  }

  int decode_nrn()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    //So it only reads the body size part of the header
    header[8] = '\0';
    return std::atoi(header+4);
  }

  char* decode_chatname_old()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    //So it only reads the body size part of the header
    for(int i=46; i<66; i++)
    {
      if(header[i] == ' ')
      {
        header[i] = '\0';
      }
    }
    header[66] = '\0';
    return (header+46);
  }

  char* decode_chatname_new()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    //So it only reads the body size part of the header
    for(int i=26; i<46; i++)
    {
      if(header[i] == ' ')
      {
        header[i] = '\0';
      }
    }
    header[46] = '\0';
    return (header+26);
  }

  bool decode_header()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    header[4] = '\0';

    body_length_ = std::atoi(header);
    if (body_length_ > max_body_length)
    {
      body_length_ = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    char header[header_length + 1] = "";
    std::sprintf(header, "%4d%4d%4d%4d%-10s%-20s%-20s", static_cast<int>(body_length_), new_room_number, chat_room_number, command, username, chatname_new, chatname_old);
    std::memcpy(data_, header, header_length);
  }


private:
  std::size_t body_length_;
  int new_room_number;
  int chat_room_number;
  int command;
  char username[11];
  char chatname_new[21];
  char chatname_old[21];
  char data_[header_length + max_body_length] = {0};

};

#endif // CHAT_MESSAGE_HPP
