//
// chat_client.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://wwc.boost.org/LICENSE_1_0.txt)
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
    std::strcpy(chatroom_name, "Lobby");
  }

  WINDOW *lobbybox;
  WINDOW *txt_fieldbox;
  WINDOW *display_usersbox;
  WINDOW *chat_roomsbox;
  WINDOW *optionsbox;

  WINDOW *lobby;
  WINDOW *txt_field;
  WINDOW *display_users;
  WINDOW *chat_rooms;
  WINDOW *options;

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

  void mainWindow()
  {
    int x, y;

    initscr();
    noecho();
    cbreak();
    refresh();
    curs_set(FALSE);

    getmaxyx(stdscr, y,x);
    lobbybox = newwin(y-5,x-25,0,0);
    txt_fieldbox = newwin(5,x-25,y-5,0);
    display_usersbox = newwin(y-15,20,0,x-23);
    chat_roomsbox = newwin(y-9,20,y-15,x-23);

    lobby = newwin(y-7,x-27,1,1);
    txt_field = newwin(3,x-29,y-4,2);
    display_users = newwin(y-17,18,1,x-22);
    chat_rooms = newwin(y-11,18,y-14,x-22);



    scrollok(lobby, TRUE);
    scrollok(txt_field, TRUE);
    scrollok(display_users, TRUE);
    box(lobbybox,0,0);
    box(txt_fieldbox,0,0);
    box(display_usersbox,0,0);
    box(chat_roomsbox,0,0);


    mvwprintw(lobbybox,0,0,"LOBBY");
    mvwprintw(txt_fieldbox,0,0,"TEXT FIELD");
    mvwprintw(display_usersbox,0,0,"Online users");
    mvwprintw(chat_roomsbox,0,0,"Chat Rooms");


    wrefresh(lobbybox);
    wrefresh(txt_fieldbox);
    wrefresh(display_usersbox);
    wrefresh(chat_roomsbox);
    wrefresh(lobby);
    wrefresh(txt_field);
    wrefresh(display_users);
    wrefresh(chat_rooms);

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
    if(strcmp(username, "Mick") == 0)
    {
      addAdmin(username);
    }
    else
    {
    std::string usernameMessage;
    usernameMessage = "username::";
    usernameMessage += get_username();
    char charUsernameMessage[usernameMessage.length()+1];
    charUsernameMessage[0] = '\0';
    std::strcat(charUsernameMessage, usernameMessage.c_str());
    chat_message user;
    char serverIsUser[6] = {0};
    std::strcpy(serverIsUser, "server");
    user.make_username(serverIsUser);
    user.set_chatname_current(chatroom_name);
    user.set_chatname_new(chatroom_name);
    user.set_cmd(0);
    user.body_length(std::strlen(charUsernameMessage));
    std::memcpy(user.body(), charUsernameMessage, user.body_length());
    user.encode_header();
    write(user);
    std::memcpy(user.body(), "\0", strlen("\0"));
    }
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

  void set_chatname(char* name)
  {
    std::strcpy(chatroom_name, name);
  }

  char* get_server_response()
  {
    return chatroom_name;
  }

  void addAdmin(char* username)
  {
    listofAdmins.push_back(username);
  }

  bool isAdmin(char* username)
  {
    if(std::find(listofAdmins.begin(), listofAdmins.end(), username) != listofAdmins.end())
    {
      return true;
    }
    return false;
  }

  void block(char* usern)
  {
    for(int i = 0; i<200; i++)
    {
      if(blocked_users[i] == NULL)
      {
        blocked_users[i] = (char*)malloc((std::strlen(usern)+1)*sizeof(char));
        std::strcpy(blocked_users[i], usern);
        wprintw(lobby, blocked_users[i]);
        break;
      }
    }
  }

  bool isBlocked(char* usern)
  {
    for(int i = 0; i<200; i++)
    {
      if(blocked_users[i] != NULL)
      {
        if(std::strstr(blocked_users[i], usern))
        {
          return true;
        }
      }
    }
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
          if (!ec && read_msg_.decode_header() && read_msg_.decode_command() == 0)
          {

            do_read_body();
          }
          else if(!ec && read_msg_.decode_header() && read_msg_.decode_command() == 3)
          {
            if(std::strcmp(chatroom_name, read_msg_.decode_chatname_new()) != 0)
            {
              std::strcpy(chatroom_name, read_msg_.decode_chatname_old());
              do_read_body();
            }
            else
            {
              do_read_header();
            }
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
          int printed = 1;
          char arr[11] = {0};
          std::string chatroomarray = "Lobby";
          strcpy(arr, read_msg_.decode_user());
          if (!ec && (isBlocked(arr) != true))
          {
            chat_message test;
            test.body_length(read_msg_.body_length());
            std::memcpy(test.body(), read_msg_.body(), read_msg_.body_length());
            if(std::strstr(test.body(), "username::"))
            {
              //don't print
            }
            else if(std::strstr(test.body(), "fromserver:"))
            {
              std::string users(test.body());
              users = users.substr(users.find(':') + 1);
              char *ok = new char[users.length() + 1];
              std::strcpy(ok, users.c_str());
              mvwprintw(display_users, 0,0, ok);
              wrefresh(display_users);
              delete ok;
            }
            else if(std::strstr(test.body(), "chatroom:"))
            {
              std::string chatroom(test.body());
              chatroom = chatroom.substr(chatroom.find(':') + 1);
              char *ok = new char[chatroom.length() + 1];
              std::strcpy(ok, chatroom.c_str());
              //find a way to compare rooms
              if(chatroomarray.find(ok) != std::string::npos)
              {

                if(printed == 1)
                {
                  wprintw(chat_rooms, ok);
                  wprintw(chat_rooms, "\n");
                  wrefresh(chat_rooms);
                  delete ok;
                  printed++;
                }
              }
              else
              {
                chatroomarray + ok + " ";
                wprintw(chat_rooms, ok);
                wprintw(chat_rooms, "\n");
                wrefresh(chat_rooms);
                delete ok;
              }
              chatroom.clear();
            }
            else
            {
		        wprintw(lobby, test.body());
            wprintw(lobby, "\n");
            wrefresh(lobby);

            }
            do_read_header();
            //refresh();

          }
          else if(!ec)
          {
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
  char chatroom_name[20];
  char* blocked_users[200] = {0};
  std::vector<std::string> listofAdmins;
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
    c.mainWindow();

    std::thread t([&io_context](){ io_context.run(); });

    char chatroom_name[20] = "Lobby";
    std::string return_str="";


    std::string s1;
    s1 = "*****";
    s1 += c.get_username();
    s1 += " has entered the chatroom **********";
    char line1[s1.length()+1];
    line1[0] = '\0';
    std::strcat(line1, s1.c_str());
    chat_message msg;
    chat_message user;
    char serverIsUser[6] = {0};
    std::strcpy(serverIsUser, "server");
    msg.set_chatname_current(chatroom_name);
    msg.set_chatname_new(chatroom_name);
    msg.make_username(serverIsUser);
    msg.set_cmd(0);
    msg.body_length(std::strlen(line1));
    std::memcpy(msg.body(), line1, msg.body_length());
    msg.encode_header();
    c.write(msg);
    std::memcpy(msg.body(), "\0", strlen("\0"));


  bool run = true;
  char xd = '\0';
  while(run)
  {
    xd = wgetch(c.txt_field);
    char meme[chat_message::max_body_length+1] = {0};
    std::string message;
    chat_message msg1;
    message.push_back(xd);
    std::strcat(meme, message.c_str());
    mvwprintw(c.txt_field, 0,0, meme);
    while(xd != '\n')
    {
      xd = wgetch(c.txt_field);
      if(xd == 127 || xd==8)
      {
        if(!message.empty())
        {
          int y,x;
          getyx(c.txt_field, y,x);
          x-=1;
          mvwdelch(c.txt_field, y, x);
          message.pop_back();
        }
      }
      else
      {
        message.push_back(xd);
        strcpy(meme, message.c_str());
        mvwprintw(c.txt_field, 0,0, meme);
        wrefresh(c.txt_field);
      }
    }
    if(message.front() == '\n')
    {
      message.clear();
    }
    else
    {
    message.pop_back();

    if(message.find("/help") != std::string::npos)
    {
      char helpMessage[500] = {0};
      std::strcat(helpMessage, "-----Help Menu-----\n");
      std::strcat(helpMessage, "/create <roomname>  -- Creates a room with the given roomname.\n");
      std::strcat(helpMessage, "/delete <roomname> -- Deletes a given room with a given roomname.\n");
      std::strcat(helpMessage, "/block <username> -- Blocks the user entered from speaking in chat (admin only).\n");
      std::strcat(helpMessage, "/goto <roomname> -- Magically transports you to the chat room of your choosing!\n");
      std::strcat(helpMessage, "-------------------\n");

      wprintw(c.lobby, helpMessage);
    }
    else if(message.find("/create") != std::string::npos)
    {
      //create the new chatroom
      std::string snew_name = message.substr(message.find(' ') + 1);
      chat_message msg;
      msg.body_length(0);
      msg.set_chatname_current(chatroom_name);
      std::strcpy(chatroom_name, snew_name.c_str());
      msg.set_chatname_new(chatroom_name);
      msg.set_cmd(2);
      msg.encode_header();
      c.set_chatname(chatroom_name);
      c.write(msg);
      wclear(c.lobby);
      wrefresh(c.lobby);
    }
    else if(message.find("/delete") != std::string::npos)
    {
      //delete the chatroom
      char deleted[100] = {0};
      std::string sdeleted = message.substr(message.find(' ') + 1);
      std::strcpy(deleted, sdeleted.c_str());
      chat_message msg;
      msg.body_length(0);
      msg.set_chatname_current(chatroom_name);
      msg.set_chatname_new(deleted);
      msg.set_cmd(3);
      msg.encode_header();
      c.set_chatname(chatroom_name);
      c.write(msg);
    }
    else if(message.find("/block") != std::string::npos)
    {
      //if the user is an admin then block the user
      char blockeduser[100] = {0};
      std::string sblockedUser = message.substr(message.find(' ') + 1);
      std::strcat(blockeduser, sblockedUser.c_str());
      c.block(blockeduser);
      wprintw(c.lobby, " has been blocked!\n");
    }
    else if(message.find("/goto") != std::string::npos)
    {
      //goto the room
      std::string sthe_room = message.substr(message.find(' ') + 1);
      chat_message msg;
      msg.body_length(0);
      msg.set_chatname_current(chatroom_name);
      std::strcpy(chatroom_name, sthe_room.c_str());
      msg.set_chatname_new(chatroom_name);
      msg.set_cmd(1);
      msg.encode_header();
      c.set_chatname(chatroom_name);
      c.write(msg);
    }
    else{
    char addendum[] = {0};
    strcpy(addendum, c.get_username());
    strcat(addendum, ": ");
    message.insert(0, addendum);
    strcpy(meme, message.c_str());
    msg1.make_username(c.get_username());
    msg1.set_cmd(0);
    msg1.set_chatname_current(chatroom_name);
    msg1.set_chatname_new(chatroom_name);
    msg1.body_length(std::strlen(meme));
    std::memcpy(msg1.body(), meme, msg1.body_length());
    msg1.encode_header();
    c.write(msg1);
    }
    //wprintw(lobby, meme);
    wmove(c.txt_field, 0,0);
    wclrtobot(c.txt_field);
    wrefresh(c.txt_field);
    wrefresh(c.lobby);
    message.clear();
    std::memcpy(msg1.body(), "\0", strlen("\0"));
    }
    }
    delwin(c.lobby);
    delwin(c.txt_field);
    delwin(c.display_users);
    delwin(c.chat_rooms);
    delwin(c.options);

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
