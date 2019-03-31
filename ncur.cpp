#include <ncurses.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

/*
void draw_win(WINDOW *screen)
{
	int x, y, i;
	getmaxyx(screen, y, x);
	mvwprintw(screen, 0,0,"+");
	mvwprintw(screen, y-1,0,"+");
	mvwprintw(screen, 0,x-1,"+");
	mvwprintw(screen, y-1,x-1,"+");

	for(i=1;i<(y-1);i++)
{
	mvwprintw(screen,i,0,"|");
	mvwprintw(screen, i, x-1, "|");

}
	for(i=1;i<(x-1);i++)
{
	mvwprintw(screen, 0,i,"-");
	mvwprintw(screen, y-1,i,"-");

}
}
*/

int main(int argc, char** argv)
{
	int x, y;
	int x1,y1;
		
 	initscr();
	noecho();
	curs_set(FALSE);
	
	
	getmaxyx(stdscr, y,x);
	WINDOW *lobby = newwin(y-5,x-25,0,0);	
	WINDOW *txt_field = newwin(5,x-25,y-5,0);
	WINDOW *display_users = newwin(y-18,20,0,x-23);
	WINDOW *chat_rooms = newwin(y-8,20,y-18,x-23);
	WINDOW *options= newwin(5,20,y-5,x-23);

	box(lobby,0,0);
	box(txt_field,0,0);
	box(display_users,0,0);
	box(chat_rooms,0,0);
	box(options,0,0);
/*
	while(1)
	{
	
	getmaxyx(stdscr,y1,x1);
	if(y1!=y || x1!=x)
	{
	x = x1;
	y = y1;
	wresize(lobby,y1-5,x);
	wresize(txt_field,5, x1);
	wresize(display_users,y1-5, x1);
	wresize(options,5, x1);
	wclear(stdscr);
	wclear(lobby);
	wclear(txt_field);
	wclear(display_users);
	wclear(chat_rooms);
	wclear(options);
	

	}
*/



	
	

	mvwprintw(lobby,0,0,"LOBBY");
	mvwprintw(txt_field,0,0,"TEXT FIELD");
	mvwprintw(display_users,0,0,"Online users");
	mvwprintw(chat_rooms,0,0,"Chat Rooms");
	mvwprintw(options,0,0,"Options");



	wrefresh(lobby);
	wrefresh(txt_field);
	wrefresh(display_users);
	wrefresh(chat_rooms);
	wrefresh(options);
	

	sleep(10);

	delwin(lobby);
	delwin(txt_field);
	delwin(display_users);
	delwin(chat_rooms);
	delwin(options);

	endwin();
	return 0;	


  
}
