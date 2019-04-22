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
#include <fstream>
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
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  chat_room()
  {
    chat_room_names[0] = (char*)malloc(6*sizeof(char));
    strcpy(chat_room_names[0], "Lobby");
    for(int i=1; i<10; i++)
    {
      chat_room_names[i] = NULL;
    }
  }
  ~chat_room()
  {
    for(int i=0; i<10; i++)
    {
      if(chat_room_names[i] != NULL)
      {
        free(chat_room_names[i]);
      }
    }
  }

  void change_room(chat_participant_ptr participant, char* old_name, char* new_name)
  {
    int new_room_number = -1;
    int old_room_number = -1;
    int happened = 0;

    for(int i=0; i< 10; i++)
    {
      if(chat_room_names[i] != NULL)
      {
        if(strcmp(new_name, chat_room_names[i]) == 0)
        {
          new_room_number = i;
          happened = 1;
        }
        if(strcmp(old_name, chat_room_names[i]) == 0)
        {
          old_room_number = i;
        }
      }
    }
    /*
    if(happened < 1)
    {
      printf("Error: chatroom %s does not exist.\n", new_name);
      //leave(participant, new_room_number);
      //join(participant, old_room_number);
      return;
    }*/

    if(new_room_number != -1 && old_room_number != -1)
    {
      participants_[old_room_number].erase(participant);
      printf("Successfully changed rooms!\n");
      rejoin(participant, new_room_number);
    }
  }

  void create_room(chat_participant_ptr participant, char* old_name, char* new_name)
  {
    bool success = true;
    int new_room_number = 0;
    int old_room_number = 0;
    for(int i=0; i< 10 && success; i++)
    {
      if(chat_room_names[i] != NULL)
      {
        if(strcmp(new_name, chat_room_names[i]) == 0)
        {
          printf("Error: chatroom %s already exists.\n", old_name);
          success = false;
        }
        if(strcmp(old_name, chat_room_names[i]) == 0)
        {
          //printf("Error: chatroom %s already exists.\n", name);
          //success = false;
          old_room_number = i;
        }
      }
    }
    //success = false;
    for(int i=0; i< 10 && success; i++)
    {
      if(chat_room_names[i] == NULL)
      {
        if(i!=0)
        {
          printf("Found empty chatroom, assigning name %s to chatroom %d.\n", new_name, i);
          chat_room_names[i] = (char*)malloc((std::strlen(new_name)+1)*sizeof(char));
          strcpy(chat_room_names[i], new_name);
          new_room_number = i;
          success = true;
          chatroom_display();
          break;
        }

      }
      else
      {
        if(i==10)
        {
          success = false;
        }
      }
    }

    if(success)
    {
      participants_[old_room_number].erase(participant);
      printf("Successfully created room %s!\n", new_name);
      rejoin(participant, new_room_number);
    }
  }

  char* whereami(chat_participant_ptr participant)
  {

    for(int i=0; i<10; i++)
    {
      if(participants_[i].count(participant) == 1)
      {
        return chat_room_names[i];
      }
    }
    //Sent to Lobby by default
    return chat_room_names[0];
  }

  void delete_room(char* name)
  {
    for(int i=1; i<10; i++)
    {
      if(chat_room_names[i] != NULL)
      {
        if(std::strcmp(chat_room_names[i], name) == 0)
        {
          if(participants_[i].empty())
          {
            free(chat_room_names[i]);
            chat_room_names[i] = NULL;
            while (recent_msgs_[i].size() > 0)
            {
              recent_msgs_[i].pop_front();
            }
            printf("Successfully deleted chatroom %s\n", name);
            chatroom_display();
            break;
          }
        }
      }
    }
  }
  //If the message has an error in it there's no way of knowing which chatroom to leave
  //So we attempt to leave all of them
  void leave_all(chat_participant_ptr participant)
  {
    for(int i=0; i<10; i++)
    {
      participants_[i].erase(participant);
    }
    usersavail--;

    if(usersavail < 0)
    {
      usersavail = 0;
    }
    availability(usersavail);
  }

  void join(chat_participant_ptr participant, int chat_room_number)
  {
    //chat_message xd;
    //xd.body_length(std::strlen("Hello  user"));
    //std::memcpy(xd.body(), "Hello user", std::strlen("Hello  user"));
    //xd.encode_header();
    //participant->deliver(xd);
    participants_[chat_room_number].insert(participant);
    for(auto msg: recent_msgs_[chat_room_number])
    {
      participant->deliver(msg);
    }
    if(chat_room_number == 0)
    {
      usersavail++;
      availability(usersavail);
    }
    chatroom_display();
  }

  void rejoin(chat_participant_ptr participant, int chat_room_number)
  {
    //chat_message xd;
    //xd.body_length(std::strlen("Hello  user"));
    //std::memcpy(xd.body(), "Hello user", std::strlen("Hello  user"));
    //xd.encode_header();
    //participant->deliver(xd);
    participants_[chat_room_number].insert(participant);
    for(auto msg: recent_msgs_[chat_room_number])
    {
      participant->deliver(msg);
    }
  }

  void leave(chat_participant_ptr participant, int chat_room_number)
  {
    participants_[chat_room_number].erase(participant);
    if(chat_room_number == 0)
    {
      usersavail--;
      availability(usersavail);
    }
    if(usersavail < 0)
    {
      usersavail = 0;
    }
	  availability(usersavail);
  }

  void chatroom_display()
  {
    char chatroommsg[30] = {0};

    for(int i = 0; i < 10; i++)
    {
      if(chat_room_names[i] != NULL)
      {
        chat_message msg1;
        std::strcpy(chatroommsg, "chatroom:");
        std::strcat(chatroommsg, chat_room_names[i]);
        msg1.set_cmd(0);
        msg1.body_length(std::strlen(chatroommsg));
        std::memcpy(msg1.body(), chatroommsg, msg1.body_length());
        msg1.encode_header();
        deliver(msg1, chat_room_names[i]);
      }
    }
  }

  void availability(int a)
  {
    std::string s1;
    s1 = "fromserver:Users online(";
    s1 += std::to_string(a);
    s1 += ")\n";
    char message[s1.length()-1];
    strcpy(message, s1.c_str());

    chat_message av;
    av.body_length(std::strlen(message));
    std::memcpy(av.body(), message , av.body_length());
    av.encode_header();
    deliver(av, "Lobby");



    std::cout<<"Users available: "<<a<<std::flush<<'\r';
  }

void deliver(const chat_message& msg, char* chat_room_name)
{
  int chat_room_number = -1;
  for(int i=0; i< 10; i++)
  {
    if(chat_room_names[i] != NULL)
    {
      if(strcmp(chat_room_name, chat_room_names[i]) == 0)
      {
        //printf("Error: chatroom %s already exists.\n", name);
        //success = false;
        chat_room_number = i;
      }
    }
  }
  //printf("Inside deliver with crn = %d.\n", chat_room_number);
  if(chat_room_number != -1)
  {
    recent_msgs_[chat_room_number].push_back(msg);
    while (recent_msgs_[chat_room_number].size() > max_recent_msgs)
      recent_msgs_[chat_room_number].pop_front();

    for (auto participant: participants_[chat_room_number])
    {
      participant->deliver(msg);
    }
  }
}

private:
  std::set<chat_participant_ptr> participants_[50];
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_[10];
  char* chat_room_names[10];
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
    room_.join(shared_from_this(), 0);
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
        if (!ec && read_msg_.decode_header() && read_msg_.decode_command() == 0)
        {
          do_read_body();
        }
        else if(!ec && read_msg_.decode_header() && read_msg_.decode_command() == 1)
        {
          char input[20] = {0};
          char input2[20] = {0};
          std::strcpy(input, read_msg_.decode_chatname_old());
          std::strcpy(input2, read_msg_.decode_chatname_new());
          printf("Attempting to change rooms from %s to %s...\n", input, input2);
          char* output = room_.whereami(shared_from_this());
          if(strcmp(output, read_msg_.decode_chatname_new()) == 0)
          {
            chat_message msg;
            char* error = "Error entering room, returning to previous room.";
            msg.body_length(std::strlen(error));
            std::memcpy(msg.body(), error, std::strlen(error));
            msg.set_chatname_current(output);
            msg.set_cmd(0);
            msg.encode_header();
            shared_from_this()->deliver(msg);
          }
          else
          {
            room_.change_room(shared_from_this(), input, input2);
          }
          do_read_header();
        }
        else if(!ec && read_msg_.decode_header() && read_msg_.decode_command() == 2)
        {
          printf("Attempting to create new chatroom...\n");
          char input[20];
          char input2[20];
          std::strcpy(input, read_msg_.decode_chatname_old());
          std::strcpy(input2, read_msg_.decode_chatname_new());
          char* output = room_.whereami(shared_from_this());
          if(strcmp(output, read_msg_.decode_chatname_new()) == 0)
          {
            chat_message msg;
            char* error = "sMDError entering room, returning to previous room.";
            //msg.set_roomname_old(read_msg_.decode_roomname_old());
            msg.body_length(std::strlen(error));
            std::memcpy(msg.body(), error, std::strlen(error));
            msg.set_chatname_current(output);
            msg.set_cmd(3);
            msg.encode_header();
            //printf("msg data: %sEND\n", msg.data());
            shared_from_this()->deliver(msg);
          }
          else
          {
          room_.create_room(shared_from_this(), input, input2);
          }

          do_read_header();
        }
        else if(!ec && read_msg_.decode_header() && read_msg_.decode_command() == 3)
        {
          printf("Attempting to delete chatroom...\n");
          //printf("header data: %s\n", read_msg_.data());
          //printf("Chatroom name: %s.\n", read_msg_.decode_chatname());
          char input[20];
          std::strcpy(input, read_msg_.decode_chatname_new());
          room_.delete_room(input);
          do_read_header();
        }
        else
        {
          room_.leave_all(shared_from_this());
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
            chat_message username;
            username.body_length(read_msg_.body_length());
            std::memcpy(username.body(), read_msg_.body(), read_msg_.body_length());
            if(std::strstr(username.body(), "username::"))
            {
              char userID[100] = {0};
              std::strcpy(userID, username.body());
              std::ofstream userOut("userIDs.txt", std::ios::app);
              std::strcpy(userID, &userID[10]);
              userOut << userID;
            }
            char in[20];
            std::strcpy(in, read_msg_.decode_chatname_old());
            room_.deliver(read_msg_, in);
            do_read_header();
          }
          else
          {
            room_.leave_all(shared_from_this());
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
            room_.leave_all(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  char usernames[500] = {0};
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
