//
// chat_client.cpp
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
#include <thread>
#include <fstream>
#include <ctype.h>
#include <sstream>

#include "asio.hpp"
#include <ncurses.h>
#include <form.h>
#include "chat_message.hpp"

using asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_client
{
public:
  chat_client(asio::io_context& io_context,
      const tcp::resolver::results_type& endpoints)
    : io_context_(io_context),
      socket_(io_context)
  {
    do_connect(endpoints);
  }

  void write(const chat_message& msg)
  {
    asio::post(io_context_,
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

  void close()
  {
    asio::post(io_context_, [this]() { socket_.close(); });
  }

  void make_login()
  {
    FIELD *field[4];
    FORM  *my_form;
    int ch;
    /* Initialize curses */
    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    /* Initialize few color pairs */

    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_WHITE);

    /* Initialize the fields */
    field[0] = new_field(1, 10, 6, 40, 0, 0);
    field[1] = new_field(1, 10, 8, 40, 0, 0);
    field[2] = new_field(1, 1, 10, 47, 0, 0);
    field[3] = NULL;

    /* Set field options */
    //set_field_fore(field[0], COLOR_PAIR(1));/* Put the field with blue background */
    //set_field_back(field[0], COLOR_PAIR(2));/* and white foreground (characters */
              /* are printed in white 	*/
    set_field_back(field[0], A_UNDERLINE);
    set_field_type(field[0], TYPE_ALNUM);
    field_opts_off(field[0], O_AUTOSKIP);  	/* Don't go to next field when this */
              /* Field is filled up 		*/
    set_field_type(field[1], TYPE_ALNUM);
    set_field_back(field[1], A_UNDERLINE);
    field_opts_off(field[1], O_AUTOSKIP);
    //field_opts_off(field[1], O_PUBLIC);
    set_field_type(field[2], TYPE_ALNUM);
    set_field_back(field[2], A_UNDERLINE);
    field_opts_off(field[2], O_AUTOSKIP);
    set_field_buffer(field[2], 0 , "Y");

    /* Create the form and post it */
    my_form = new_form(field);
    post_form(my_form);

    refresh();


    set_current_field(my_form, field[0]); /* Set focus to the colored field */

    mvprintw(6, 24, "Username:");
    mvprintw(8, 24, "Password:");
    mvprintw(10, 24, "First time user?(Y/N)");
    mvprintw(4, 34, "Please login!");
    mvprintw(12, 45, "Login");
    form_driver(my_form, REQ_FIRST_FIELD);
    refresh();

    if(strlen(username) > 0)
    {
      set_field_buffer(field[0], 0, username);
      set_field_buffer(field[2], 0 , "N");
    }
    bool out = false;
    /* Loop through to get user requests */
    while(out == false)
    {
      ch = getch();
      switch(ch)
      {	case KEY_DOWN:
          /* Go to next field */
          form_driver(my_form, REQ_NEXT_FIELD);
          break;
        case KEY_UP:
          /* Go to previous field */
          form_driver(my_form, REQ_PREV_FIELD);
          break;
        case KEY_BACKSPACE:
          form_driver(my_form, REQ_PREV_CHAR);
          form_driver(my_form, REQ_DEL_CHAR);
          break;
        case KEY_LEFT:
          form_driver(my_form, REQ_PREV_CHAR);
          break;
        case KEY_RIGHT:
          form_driver(my_form, REQ_NEXT_CHAR);
          break;
        case '\t':
          form_driver(my_form, REQ_NEXT_FIELD);
          break;
        case '\n':
          char* choice;
          char* tempUser;
          char* tempPassword;
          form_driver(my_form, REQ_VALIDATION);
          tempUser = trim_whitespaces(field_buffer(field[0], 0));
          tempPassword = trim_whitespaces(field_buffer(field[1], 0));
          choice = field_buffer(field[2], 0);

          if ((strlen(tempUser) <1) || (strlen(tempPassword)< 1))
          {
            attron(COLOR_PAIR(1));
            mvprintw(14, 25, "Please enter a username or password");
            attroff(COLOR_PAIR(1));
            refresh();
          }
          else
          {
            if(strcmp(choice, "Y") == 0)
            {
              int i;
              for(i=0; i<=strlen(tempUser); i++){
                username[i] = tempUser[i];
              }
              for(i=0; i<=strlen(tempPassword); i++){
                password[i] = tempPassword[i];
              }
              attron(COLOR_PAIR(3));
              mvprintw(14, 25, "Please enter a username or password");
              attroff(COLOR_PAIR(3));
              attron(COLOR_PAIR(1));
              mvprintw(12, 45, "Login");
              attroff(COLOR_PAIR(1));
              refresh();
              out = true;
            }
            else
            {
              if(verify(tempUser, tempPassword))
              {
                int i;
                for(i=0; i<=strlen(tempUser); i++){
                  username[i] = tempUser[i];
                }
                for(i=0; i<=strlen(tempPassword); i++){
                  password[i] = tempPassword[i];
                }
                attron(COLOR_PAIR(3));
                mvprintw(14, 25, "Please enter a username or password");
                attroff(COLOR_PAIR(3));
                attron(COLOR_PAIR(1));
                mvprintw(12, 45, "Login");
                attroff(COLOR_PAIR(1));
                refresh();
                out = true;
              }
              else
              {
                attron(COLOR_PAIR(1));
                mvprintw(14, 25, "Incorrect username or password");
                attroff(COLOR_PAIR(1));
                refresh();
              }
            }
            //ch = KEY_F(1);
          }
          break;
        default:
          /* If this is a normal character, it gets */
          /* Printed				  */
          form_driver(my_form, ch);
          break;
      }

    }
    /* Un post form and free the memory */

    unpost_form(my_form);
    free_form(my_form);
    free_field(field[0]);
    free_field(field[1]);
    free_field(field[2]);


    endwin();
  }

  void login()
  {
    std::ifstream userIn("userID.txt");
    std::ofstream userOut("userID.txt", std::ios::app);
    if(userIn.is_open())
    {
      if(test(userIn))
      {
        make_login();
      }
    }
    else
    {
      make_login();
      userOut << username << " " << password;
      //userOut << username;
    }

    userIn.close();
    userOut.close();
    std::strcat(username, ": ");

  }

  bool test(std::ifstream& userIn)
  {
    while(userIn >> username >> password){
      return true;
    }
    return false;
  }

  bool verify(char* tempUser, char* tempPassword)
  {
    if(std::strcmp(tempPassword, password) == 0)
    {
      if(std::strcmp(tempUser, username) == 0)
      {
        return true;
      }
    }
    else
    {
      return false;
    }

    /*unsigned int i;
    for(i=0; i<strlen(username); i++)
    {
      if
    }
    */
    /*
    while(userIn >> username >> password)
    {
      if(strlen(username) > 0)
      {
        userIn.close();
        return true;
      }
      else
      {
        userIn.close();
        return false;
      }
    }

    */
    return false;
  }

  char* get_username()
  {
    return username;
  }

  static char* trim_whitespaces(char *str)
  {
  	char *end;

  	// trim leading space
  	while(isspace(*str))
  		str++;

  	if(*str == 0) // all spaces?
  		return str;

  	// trim trailing space
  	end = str + strnlen(str, 128) - 1;

  	while(end > str && isspace(*end))
  		end--;

  	// write new null terminator
  	*(end+1) = '\0';

  	return str;
  }

private:
  void do_connect(const tcp::resolver::results_type& endpoints)
  {
    asio::async_connect(socket_, endpoints,
        [this](std::error_code ec, tcp::endpoint)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }

  void do_read_header()
  {
    asio::async_read(socket_,
        asio::buffer(read_msg_.data(), chat_message::header_length),
        [this](std::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_read_body()
  {
    asio::async_read(socket_,
        asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](std::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            std::cout.write(read_msg_.body(), read_msg_.body_length());
            std::cout << "\n";
            do_read_header();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_write()
  {
    asio::async_write(socket_,
        asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this](std::error_code ec, std::size_t /*length*/)
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
            socket_.close();
          }
        });
  }

private:
  asio::io_context& io_context_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
  char username[11];
  char password[11];
};

int main(int argc, char* argv[])
{

  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    chat_client c(io_context, endpoints);
    c.login();
    std::thread t([&io_context](){ io_context.run(); });




    char line[chat_message::max_body_length + 1];
    char newline[chat_message::max_body_length + 1];
    while (std::cin.getline(line, chat_message::max_body_length + 1))
    {
      chat_message msg;
      std::strcpy(newline, c.get_username());
      std::strcat(newline, line);
      msg.body_length(std::strlen(newline));
      std::memcpy(msg.body(), newline, msg.body_length());
      msg.encode_header();
      c.write(msg);
      std::strcpy(newline, "");
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
