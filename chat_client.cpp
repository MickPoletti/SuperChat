//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


#include <sys/stat.h>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <thread>
#include <fstream>
#include <ctype.h>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <vector>


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

int write_all(FILE *file, const void *buf, int len)
{
    const char *pbuf = (const char *) buf;

    while (len > 0)
    {
        int written = fwrite(pbuf, 1, len, file);
        if (written < 1)
        {
            printf("Can't write to file");
            return -1;
        }

        pbuf += written;
        len -= written;
    }

    return 0;
}

int read_all(int sock, void *buf, int len)
{
    char *pbuf = (char *) buf;
    int total = 0;

    while (len > 0)
    {
        int rval = recv(sock, pbuf, len, 0);
        if (rval < 0)
        {
            // if the socket is non-blocking, then check
            // the socket error for WSAEWOULDBLOCK/EAGAIN
            // (depending on platform) and if true then
            // use select() to wait for a small period of
            // time to see if the socket becomes readable
            // again before failing the transfer...

            printf("Can't read from socket");
            return -1;
        }

        if (rval == 0)
        {
            printf("Socket disconnected");
            return 0;
        }

        pbuf += rval;
        len -= rval;
        total += rval;
    }

    return total;
}

void RecvFile(int sock, const char* filename)
{
    int rval;
    char buf[0x1000];

    FILE *file = fopen(filename, "wb");
    if (!file)
    {
        printf("Can't open file for writing");
        return;
    }

    // if you need to handle files > 2GB,
    // be sure to use a 64bit integer, and
    // a network-to-host function that can
    // handle 64bit integers...
    long size = 0;
    if (read_all(sock, &size, sizeof(size)) == 1)
    {
        size = ntohl(size);
        while (size > 0)
        {
            rval = read_all(sock, buf, size);
            if (rval < 1)
                break;

            if (write_all(file, buf, rval) == -1)
                break;
        }
    }

    fclose(file);
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

  bool fileExists(const std::string& filename)
  {
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
  }

  void setWindow(WINDOW* sentWindow)
  {
    window = sentWindow;
  }

  WINDOW* getWindow()
  {
    return window;
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
              unsigned int i;
              for(i=0; i<=strlen(tempUser); i++){
                username[i] = tempUser[i];
              }
              for(i=0; i<=strlen(tempPassword); i++){
                password[i] = tempPassword[i];
              }
              /* TODO: This section of code needs to be called alot let's make it a function */
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
                unsigned int i;
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
    username[0] = '\0';
    if(fileExists("userID.txt"))
    {
      std::ifstream userIn("userID.txt");
      std::ofstream userOut("userID.txt", std::ios::app);
      if(test(userIn))
      {
        make_login();
      }
      userIn.close();
      userOut.close();
    }
    else
    {
      make_login();
      std::ofstream userOut("userID.txt", std::ios::app);
      userOut << username << " " << password;
      userOut.close();
    }


    std::strcat(username, ": ");

  }

  bool test(std::ifstream& userIn)
  {
    while(userIn >> username >> password)
    {
      return true;
    }
    return false;
  }

  /* TODO: This function will need to send information to the server before returning true or false! */
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
		        wprintw(getWindow(), read_msg_.body());
            wprintw(getWindow(), "\n");
            wrefresh(getWindow());
            //refresh();
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
  int x, y,a,b;
  WINDOW* window;
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
    int x, y;

   	initscr();
  	noecho();
  	cbreak();
    refresh();
  	curs_set(FALSE);


  	getmaxyx(stdscr, y,x);
  	WINDOW *lobbybox = newwin(y-5,x-25,0,0);
  	WINDOW *txt_fieldbox = newwin(5,x-25,y-5,0);
  	WINDOW *display_usersbox = newwin(y-18,20,0,x-23);
  	WINDOW *chat_roomsbox = newwin(y-8,20,y-18,x-23);
  	WINDOW *optionsbox = newwin(5,20,y-5,x-23);

  	WINDOW *lobby = newwin(y-7,x-27,1,1);
  	WINDOW *txt_field = newwin(3,x-29,y-4,2);
  	WINDOW *display_users = newwin(y-20,18,1,x-22);
  	WINDOW *chat_rooms = newwin(y-12,18,y-17,x-22);
  	WINDOW *options = newwin(3,18,y-4,x-22);

  	scrollok(lobby, TRUE);
  	scrollok(txt_field, TRUE);
  	scrollok(display_users, TRUE);
  	box(lobbybox,0,0);
  	box(txt_fieldbox,0,0);
  	box(display_usersbox,0,0);
  	box(chat_roomsbox,0,0);
  	box(optionsbox,0,0);


  	mvwprintw(lobbybox,0,0,"LOBBY");
  	mvwprintw(txt_fieldbox,0,0,"TEXT FIELD");
  	mvwprintw(display_usersbox,0,0,"Online users");
  	mvwprintw(chat_roomsbox,0,0,"Chat Rooms");
  	mvwprintw(optionsbox,0,0,"Options");


  	wrefresh(lobbybox);
  	wrefresh(txt_fieldbox);
  	wrefresh(display_usersbox);
  	wrefresh(chat_roomsbox);
  	wrefresh(optionsbox);
  	wrefresh(lobby);
  	wrefresh(txt_field);
  	wrefresh(display_users);
  	wrefresh(chat_rooms);
  	wrefresh(options);


    std::string s1;
    s1 = "*****";
    s1 += c.get_username();
    s1 += " has entered the chatroom **********";
    c.setWindow(lobby);
    char line1[std::strlen(s1.c_str())] = {0};

    std::memcpy(line1, s1.c_str(), std::strlen(s1.c_str()) );

    chat_message msg;
    msg.body_length(std::strlen(line1));
    std::memcpy(msg.body(), line1, msg.body_length());
    msg.encode_header();
    c.write(msg);

  bool run = true;
  char xd;
  while(run)
  {
    xd = wgetch(txt_field);
    char meme[chat_message::max_body_length+1];
    meme[0] = '\0';
    std::string message;
    chat_message msg1;
    message.push_back(xd);
    strcpy(meme, message.c_str());
    mvwprintw(txt_field, 0,0, meme);
    while(xd != '\n')
    {
      xd = wgetch(txt_field);
      if(xd == 127 || xd==8)
      {
        if(!message.empty())
        {
          int y,x;
          getyx(txt_field, y,x);
          x-=1;
          mvwdelch(txt_field, y, x);
          message.pop_back();
        }
      }
      else
      {
        message.push_back(xd);
        strcpy(meme, message.c_str());
        mvwprintw(txt_field, 0,0, meme);
        wrefresh(txt_field);
      }
    }
    if(message.front() == '\n')
    {
      //Do nothing
    }
    else
    {
    message.insert(0, c.get_username());
    strcpy(meme, message.c_str());
    msg1.body_length(std::strlen(meme));
    std::memcpy(msg1.body(), meme, msg1.body_length());
    msg1.encode_header();
    c.write(msg1);

    //wprintw(lobby, meme);
    wmove(txt_field, 0,0);
    wclrtobot(txt_field);
    wrefresh(txt_field);
    wrefresh(lobby);
    message.clear();
    }
    }
    delwin(lobby);
    delwin(txt_field);
    delwin(display_users);
    delwin(chat_rooms);
    delwin(options);

    endwin();
    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
