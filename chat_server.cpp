//
// chat_server.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include <string>
#include <sstream>

#include "asio.hpp"
#include <ncurses.h>
#include "chat_message.hpp"

using asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------


int usersavail;

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
  char userID[100];
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {

    //chat_message xd;
    //xd.body_length(std::strlen("Hello  user"));
    //std::memcpy(xd.body(), "Hello user", std::strlen("Hello  user"));
    //xd.encode_header();
    //participant->deliver(xd);


    participants_.insert(participant);
	  usersavail++;
	  availability(usersavail,participant);


    for (auto msg: recent_msgs_)
      participant->deliver(msg);

  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
	  usersavail--;
    if(usersavail < 0)
    {
      usersavail = 0;
    }
	  availability(usersavail,participant);
  }


  void availability(int a,chat_participant_ptr participant)
  {
    std::string s1;
    s1 = "Users available: ";
    s1 += std::to_string(a);
    char message[s1.length()-1];
    strcpy(message, s1.c_str());
    /*
    chat_message av;
    av.body_length(std::strlen(message));
    std::memcpy(av.body(), message , av.body_length());
    av.encode_header();
    participant->deliver(av);
    */
    std::cout<<"Users available: "<<a<<std::flush<<'\r';
  }

void deliver(const chat_message& msg)
{
  recent_msgs_.push_back(msg);
  while (recent_msgs_.size() > max_recent_msgs)
  recent_msgs_.pop_front();



  for (auto participant: participants_)
  participant->deliver(msg);
}

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
  {
  }



  void start()
  {
    room_.join(shared_from_this());
    do_read_header();

  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }


  }

int send_all(int sock, const void *buf, int len)
{
    const char *pbuf = (const char *) buf;

    while (len > 0)
    {
        int sent = send(sock, pbuf, len, 0);
        if (sent < 1)
        {
            // if the socket is non-blocking, then check
            // the socket error for WSAEWOULDBLOCK/EAGAIN
            // (depending on platform) and if true then
            // use select() to wait for a small period of
            // time to see if the socket becomes writable
            // again before failing the transfer...

            printf("Can't write to socket");
            return -1;
        }

        pbuf += sent;
        len -= sent;
    }

    return 0;
}

void str_server(int sock)
{
    char buf[0x1000];
    const char* filename = "test.text";
    struct stat s;

    if (stat(filename, &s) == -1)
    {
        printf("Can't get file info");
        return;
    }

    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        printf("Can't open file for reading");
        return;
    }

    // if you need to handle files > 2GB,
    // be sure to use a 64bit integer, and
    // a host-to-network function that can
    // handle 64bit integers...
    long size = s.st_size;
    long tmp_size = htonl(size);
    if (send_all(sock, &tmp_size, sizeof(tmp_size)) == 0)
    {
        while (size > 0)
        {
            int rval = fread(buf, 1, size, file);
            if (rval < 1)
            {
                printf("Can't read from file");
                break;
            }

            if (send_all(sock, buf, rval) == -1)
                break;

            size -= rval;
        }
    }

    fclose(file);
}


private:
  void do_read_header()
  {
    auto self(shared_from_this());
    asio::async_read(socket_,
        asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](std::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    asio::async_read(socket_,
        asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](std::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            room_.deliver(read_msg_);
            do_read_header();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    asio::async_write(socket_,
        asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](std::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();

            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(asio::io_context& io_context,
      const tcp::endpoint& endpoint)
    : acceptor_(io_context, endpoint)
  {
    do_accept();
  }





private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](std::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<chat_session>(std::move(socket), room_)->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  chat_room room_;
};



//----------------------------------------------------------------------


int main(int argc, char* argv[])
{
usersavail=0;
  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }

    asio::io_context io_context;


    std::list<chat_server> servers;
    for (int i = 1; i < argc; ++i)
    {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_context, endpoint);
    }

    io_context.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
