#ifndef KYBD_READ_THREAD
#define KYBD_READ_THREAD
/**
 * This is the keyboard callable. 
 * This example implements a more
 * C++-y version of read-line.c.
 */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <functional>
#include <termios.h>
#include <algorithm>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <alloca.h>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <regex.h>
#include <fcntl.h>
#include <thread>
#include <vector>
#include <cctype>
#include <mutex>
#include <stack>
/** Include wildcards for tab completion **/
#include "wildcard.hpp"
#include "command.hpp"

extern FILE * yyin;
extern void yyrestart(FILE*);
extern int yyparse();

class read_line
{
public:
   read_line() { m_get_mode = 1; }

   bool write_with_error(int _fd, char & c);
   bool write_with_error(int _fd, const char * s);
   bool write_with_error(int _fd, const char * s, const size_t & len);

   bool read_with_error(int _fd, char & c);
   size_t get_term_width();

   void operator() () {
	  // Raw mode
	  char input;
	  std::string _line = "";

	  // Read in the next character
	  for (; true ;) {
		 /* If you can't read from 0, don't continue! */
		 if (!read_with_error(0, input)) break;

		 // Echo the character to the screen
		 if (input >= 32 && input != 127) {
			// Character is printable
			if (input == '!') {
			   if (!write_with_error(0, "!")) continue;

			   /* Check for "!!" and "!-<n>" */
			   if (!m_history.size()) {
				  _line += input;
				  continue;
			   }
	  
			   char ch1;
			   /* read next char from stdin */
			   if (!read_with_error(0, ch1)) continue;

			   /* print newline and stop looping */
			   if ((ch1 == '\n') && !write_with_error(1, "\n", 1)) break;
			   
			   else if (ch1 == '!') {
				  // "!!" = run prior command
				  if (!write_with_error(1, "!", 1)) continue;

				  _line += m_history[m_history.size() - 1];
				  _line.pop_back();
				  m_show_line = true;
				  continue;
			   } else if (ch1 == '-') {
				  if (!write_with_error(1, "-", 1)) continue;

				  auto && is_digit = [](char b) { return '0' <= b && b <= '9'; };
				  
/* "!-<n>" = run what I did n commands ago. */
				  char * buff = (char*) alloca(20); char * b;
				  for (b=buff;read(0,b,1)&&write(1,b,1)&&is_digit(*b);*(++b+1)=0);
				  int n = atoi(buff); bool run_cmd = false;
				  if (*b=='\n') run_cmd = true;
				  if (n > 0) {
					 int _idx = m_history.size() - n;
					 _line += m_history[(_idx >= 0) ? _idx : 0];
					 _line.pop_back();
					 m_show_line = true;
					 if (run_cmd) {
						if (m_buff.size()) {
						   char ch;
						   for (; m_buff.size();) {
							  ch = m_buff.top(); m_buff.pop();
							  _line += ch;
							  
							  if (!write_with_error(1, ch)) continue;
						   }
						}
						history_index = m_history.size();
						break;
					 }
				  }
			   } else {
				  if (!write_with_error(1, ch1)) continue;
				  _line += "!"; _line += ch1;
				  continue;
			   }
			}
	
			if (m_buff.size()) {
			   /**
				* If the buffer has contents, they are
				* in the middle of a line.
				*/
			   _line += input;
	  
			   /* Write current character */
			   if (!write_with_error(1, input)) continue;

			   /* Copy buffer and print */
			   std::stack<char> temp = m_buff;
			   for (char d = 0; temp.size(); ) {
				  d = temp.top(); temp.pop();
				  if (!write_with_error(1, d)) continue;
			   }

			   // Move cursor to current position.
			   for (size_t x = 0; x < m_buff.size(); ++x) {
				  if (!write_with_error(1, "\b", 1)) continue;
			   }
			} else {
			   _line += input;
			   if ((size_t)history_index == m_history.size())
				  m_current_line_copy += input;
			   /* Write to screen */
			   if (!write_with_error(1, input)) continue;
			}
		 } else if (input == 10) {
			// Enter was typed
			if (m_buff.size()) {
			   char ch;
			   for (; m_buff.size();) {
				  ch = m_buff.top(); m_buff.pop();
				  _line += ch;
				  if (!write_with_error(1, ch)) continue;
			   }
			} if (!write_with_error(1, input)) continue;
			history_index = m_history.size();
			break;
		 } else if (input == 1) {
			// Control A
			if (!_line.size()) continue;

			register size_t term_width = get_term_width();
			
			for (; _line.size();) {
			   m_buff.push(_line.back()); _line.pop_back();
			   /* Next check if we need to go up */
			   /* @todo this does not quite work -- but it's close */
			   if (_line.size() == term_width) {
				  if (!write_with_error(1, "\033[1A\x1b[33;1m$ \x1b[0m")) continue;
				  else if (!write_with_error(1, _line.c_str(), term_width - 3)) continue;

				  for (size_t k = 0; k < term_width - 2; ++k)
					 if (!write_with_error(1, "\b", 1)) break;
			   } else if (!write_with_error(1, "\b", 1)) continue;
			}
		 } else if (input == 5) {
			// Control E
			char ctrle[m_buff.size() + 1];
			size_t len = m_buff.size();
			memset(ctrle, 0, m_buff.size() + 1);

			for (char * d = ctrle; m_buff.size();) {
			   *(d) = m_buff.top(); m_buff.pop();
			   _line += *(d++);
			}

			if (!write_with_error(1, ctrle, len)) continue;
		 } else if (input == 4) {
			// Control D
			if (!m_buff.size()) continue;
			if (!_line.size()) {
			   m_buff.pop();
			   std::stack<char> temp = m_buff;
			   for (char d = 0; temp.size();)
				  if (write(1, &(d = (temp.top())), 1)) temp.pop();
	  
			   if (!write(1, " ", 1)) std::cerr<<"WAT.\n";
			   for (int x = m_buff.size() + 1; x -= write(1, "\b", 1););
			   continue;
			}
	
			if (m_buff.size()) {
			   // Buffer!
			   std::stack<char> temp = m_buff;
			   temp.pop();
			   for (char d = 0; temp.size(); ) {
				  d = temp.top(); temp.pop();
				  if (!write(1, &d, 1)) {
					 perror("write");
					 continue;
				  }
			   }
			   char b = ' ';
			   if (!write(1, &b, 1)) {
				  perror ("write");
				  continue;
			   } b = '\b';
			   if (!write(1, &b, 1)) {
				  perror("write");
				  continue;
			   } m_buff.pop();
			   /* Move cursor to current position. */
			   for (size_t x = 0; x < m_buff.size(); ++x) {
				  if (!write(1, &b, 1)) {
					 perror("write");
					 continue;
				  }
			   }
			} else continue;
			if ((size_t)history_index == m_history.size()) m_current_line_copy.pop_back();
		 } else if (input == 11) {
			/// Control K
			if (!m_buff.size()) continue;
			size_t count = m_buff.size() + 1;
			/// Clear the stack. On its own thread.
			std::thread stack_killer([this](){
				  for(;m_buff.size();m_buff.pop());
			   }); stack_killer.detach();
			char * spaces = (char*) malloc(count + 1);
			char * bspaces = (char*) malloc(count + 1);
			memset(spaces, ' ', count); spaces[count] = '\0';
			memset(bspaces, '\b', count); bspaces[count] = '\0';
			if (write(1, spaces, count) != (int) count) {
			   std::cerr<<"Could not write spaces to stdout"<<std::endl;
			} else if (write(1, bspaces, count) != (int) count) {
			   std::cerr<<"count not write backspaces to stdout!"<<std::endl;
			}
		 } else if (input == 8 || input == 127) {
			/* backspace */
			if (!_line.size()) continue;

			if (m_buff.size()) {
			   // Buffer!
			   char b = '\b';
			   if (!write(1, &b, 1)) {
				  perror("write");
				  continue;
			   }
			   _line.pop_back();
			   std::stack<char> temp = m_buff;
			   for (char d = 0; temp.size(); ) {
				  d = temp.top(); temp.pop();
				  if (!write(1, &d, 1)) {
					 perror("write");
					 continue;
				  }
			   }
			   b = ' ';
			   if (!write(1, &b, 1)) {
				  perror("write");
				  continue;
			   } b = '\b';
		  
			   if (!write(1, &b, 1)) {
				  perror("write");
				  continue;
			   }

			   /* get terminal width */
			   register size_t term_width = get_term_width();
			   register size_t line_size = _line.size() + m_buff.size() + 2;
		  
			   for (size_t x = 0; x < m_buff.size(); ++x, --line_size) {
				  /* if the cursor is at the end of a line, print line up */
				  if (line_size && (line_size % term_width == 0)) {
					 /* need to go up a line */			  
					 const size_t p_len = strlen("\033[1A\x1b[33;1m$ \x1b[0m");

					 /* now we print the string */
					 if (line_size == term_width) {	
						if (!write(1, "\033[1A\x1b[33;1m$ \x1b[0m", p_len)) {
						   /**
							* @todo Make sure you print the correct prompt!
							* this currently is not going to print that prompt.
							*/
						   perror("write");
						   continue;
						}  else if (!write(1, _line.c_str(), _line.size())) {
						   perror("write");
						   continue;
						} else break;
					 } else {
						if (!write(1, "\033[1A \b", strlen("\033[1A \b"))) {
						   perror("write");
						   continue;
						} else if (!write(1, _line.c_str() + (_line.size() - line_size),
										  _line.size() - line_size)) {
						   perror("write");
						   continue;
						}
					 }
				  } else if (!write(1, "\b", 1)) {
					 perror("write");
					 continue;
				  }
			   }
			} else {
			   char ch = '\b';
			   if (!write(1, &ch, 1)) {
				  perror("write");
				  continue;
			   }
			   ch = ' ';
			   if (!write(1, &ch, 1)) {
				  perror("write");
				  continue;
			   }
			   ch = '\b';
			   if (!write(1, &ch, 1)) {
				  perror("write");
				  continue;
			   }
			   _line.pop_back();
			} if ((size_t) history_index == m_history.size()) m_current_line_copy.pop_back();
		 } else if (input == 9) {
			Command::currentSimpleCommand = std::unique_ptr<SimpleCommand>
			   (new SimpleCommand());
	
			/* Part 1: add a '*' to the end of the stream. */
			std::string _temp;

			std::vector<std::string> _split;
			if (_line.size()) {
			   _split = string_split(_line, ' ');
			   _temp = tilde_expand(_split.back()) + "*";
			} else _temp = "*";

			char * _complete_me = strndup(_temp.c_str(), _temp.size());
	
			/* Part 2: Invoke wildcard expand */
			wildcard_expand(_complete_me);

			/**
			 * Part 3: If wc_collector.size() <= 1, then complete the tab.
			 *         otherwise, run "echo <text>*"
			 */
			std::string * array = Command::currentCommand.wc_collector.data();
			std::sort(array, array + Command::currentCommand.wc_collector.size(),
					  Comparator());

			if (Command::currentCommand.wc_collector.size() == 1) {
			   /* First check if the line has any spaces! */
			   /*     If so, we will wrap in quotes!      */
			   bool quote_wrap = false; 
			   if (Command::currentCommand.wc_collector[0].find(" ")
				   != std::string::npos) {
				  Command::currentCommand.wc_collector[0].insert(0, "\"");
				  quote_wrap = true;
			   }
			   char ch[_line.size() + 1]; char sp[_line.size() + 1];
			   ch[_line.size()] = '\0'; sp[_line.size()] = '\0';
			   memset(ch, '\b', _line.size()); memset(sp, ' ', _line.size());
			   if (!write(1, ch, _line.size())) perror("write");
			   if (!write(1, sp, _line.size())) perror("write");
			   if (!write(1, ch, _line.size())) perror("write");
			   _line = "";
	  
			   for (size_t x = 0; x < _split.size() - 1; _line += _split[x++] + " ");
			   _line += Command::currentCommand.wc_collector[0];
			   if (quote_wrap) _line = _line + "\"";
			   m_current_line_copy = _line;
			   Command::currentCommand.wc_collector.clear();
			   Command::currentCommand.wc_collector.shrink_to_fit();
			} else if (Command::currentCommand.wc_collector.size() == 0) {
			   /* now we check the binaries! */
			   char * _path = getenv("PATH");
			   if (!_path) {
				  /* if path isn't set, continue */
				  free(_complete_me); continue;
			   } std::string path(_path);
			   
			   /* part 1: split the path variable into individual dirs */
			   std::vector<std::string> _path_dirs = vector_split(path, ':');
			   std::vector<std::string> path_dirs;

			   /* part 1.5: remove duplicates */
			   for (auto && x : _path_dirs) {
				  /* @todo it would be nice to not do this */
				  bool add = true;
				  for (auto && y : path_dirs) {
					 if (x == y) add = false;
				  } if (add) path_dirs.push_back(x);
			   }

			   /* part 2: go through the paths, coallate matches */
			   for (auto && x : path_dirs) {
				  /* add trailing '/' if not already there */
				  if (x.back() != '/') x += '/';
				  /* append _complete_me to current path */
				  x += _complete_me;
				  /* duplicate the string */
				  char * _x_cpy = strndup(x.c_str(), x.size());

				  /* invoke wildcard_expand */
				  wildcard_expand(_x_cpy); free(_x_cpy);
			   } std::vector<std::string> wc_expanded =
					Command::currentCommand.wc_collector;

			   /* part 4: check for a single match */
			   if (wc_expanded.size() == 1) {
				  /* First check if the line has any spaces! */
				  /*     If so, we will wrap in quotes!      */
				  bool quote_wrap = false; 
				  if (Command::currentCommand.wc_collector[0].find(" ")
					  != std::string::npos) {
					 Command::currentCommand.wc_collector[0].insert(0, "\"");
					 quote_wrap = true;
				  }
				  char ch[_line.size() + 1]; char sp[_line.size() + 1];
				  ch[_line.size()] = '\0'; sp[_line.size()] = '\0';
				  memset(ch, '\b', _line.size()); memset(sp, ' ', _line.size());
				  if (!write(1, ch, _line.size())) perror("write");
				  if (!write(1, sp, _line.size())) perror("write");
				  if (!write(1, ch, _line.size())) perror("write");
				  _line = "";
	  
				  for (size_t x = 0; x < _split.size() - 1; _line += _split[x++] + " ");
				  _line += Command::currentCommand.wc_collector[0];
				  if (quote_wrap) _line = _line + "\"";
				  m_current_line_copy = _line;
				  Command::currentCommand.wc_collector.clear();
				  Command::currentCommand.wc_collector.shrink_to_fit();
			   } else { /* part 5: handle multiple matches */
				  /* @todo appropriately handle multiple matches */
				  free(_complete_me); continue;
			   }

			   /* free resources and print */
			   write_with_error(1, _line.c_str(), _line.size());
			   free(_complete_me); continue;
			} else {
			   std::cout<<std::endl;
			   std::vector<std::string> _wcd = Command::currentCommand.wc_collector;
			   std::vector<std::string> cpyd = Command::currentCommand.wc_collector;
			   std::string longest_common((longest_substring(cpyd)));
		  
			   if (_wcd.size()) {
				  printEvenly(_wcd); char * _echo = strdup("echo");
				  Command::currentSimpleCommand->insertArgument(_echo);
				  Command::currentCommand.wc_collector.clear();
				  Command::currentCommand.wc_collector.shrink_to_fit();
				  Command::currentCommand.execute(); free(_echo);

				  /**
				   * Now we add the largest substring of
				   * the above to the current string so
				   * that the tab completion isn't "butts."
				   *
				   * ~ Ana-Gabriel Perez
				   */

				  if (longest_common.size()) {
					 char * to_add = strndup(longest_common.c_str() + strlen(_complete_me) - 1,
											 longest_common.size() - strlen(_complete_me) + 1);
					 _line += to_add; free(to_add);
					 m_current_line_copy = _line;
				  }
			   } else { free(_complete_me); continue; }
			} free(_complete_me);

			if (write(1, _line.c_str(), _line.size()) != (int)_line.size()) {
			   perror("write");
			   std::cerr<<"I.E. STAHP!\n"<<std::endl;
			}
		 } else if (input == 27) {	
			char ch1, ch2;
			// Read the next two chars
			if (!read(0, &ch1, 1)) {
			   perror("read");
			   continue;
			} else if (!read(0, &ch2, 1)) {
			   perror("read");
			   continue;
			}
		
			if (ch1 == 91 && ch2 == 49) {
			   /// Control + arrow key?
			   char ch3[4]; memset(ch3, 0, 4 * sizeof(char));
			   if (read(0, ch3, 3) != 3) {
				  perror("read");
				  continue;
			   }
			   if (ch3[0] == 59 && ch3[1] == 53 && ch3[2] == 67) {
				  /// control + right arrow
				  /// If the stack is empty, we are done.
				  if (!(m_buff.size())) continue;
				  for (;(m_buff.size() &&
						 ((_line += m_buff.top(), m_buff.pop(),_line.back()) != ' ') &&
						 (write(1, &_line.back(), 1) == 1)) ||
						  ((_line.back() == ' ') ? !(write(1, " ", 1)) : 0););
	    
			   } else if (ch3[0] == 59 && ch3[1] == 53 && ch3[2] == 68) {
				  /// control + left arrow
				  if (!_line.size()) continue;
				  for (;(_line.size() &&
						 ((m_buff.push(_line.back()),
						   _line.pop_back(), m_buff.top()) != ' ') &&
						 (write(1, "\b", 1) == 1)) ||
						  ((m_buff.top() == ' ') ? !(write(1, "\b", 1)) : 0););
			   }
			} else if (ch1 == 91 && ch2 == 51) {
			   // Maybe a delete key?
			   char ch3;

			   if (!read(0, &ch3, 1)) {
				  perror("read");
				  continue;
			   }
			   if (ch3 == 126) {
				  if (!m_buff.size()) continue;
				  if (!_line.size()) {
					 m_buff.pop();
					 std::stack<char> temp = m_buff;
					 for (char d = 0; temp.size();)
						if (write(1, &(d = (temp.top())), 1)) temp.pop();

					 if (!write(1, " ", 1)) std::cerr<<"WAT.\n";
					 for (int x = m_buff.size() + 1; x -= write(1, "\b", 1););
					 continue;
				  }

				  if (m_buff.size()) {
					 // Buffer!
					 std::stack<char> temp = m_buff;
					 temp.pop();
					 for (char d = 0; temp.size(); ) {
						d = temp.top(); temp.pop();
						if (!write(1, &d, 1)) {
						   perror("write");
						   continue;
						}
					 }
					 char b = ' ';
					 if (!write(1, &b, 1)) {
						perror("write");
						continue;
					 } b = '\b';
					 if (!write(1, &b, 1)) {
						perror("write");
						continue;
					 } m_buff.pop();
					 // Move cursor to current position.
					 for (size_t x = 0; x < m_buff.size(); ++x) {
						if (!write(1, &b, 1)) {
						   perror("write");
						   continue;
						}
					 }
				  } else continue;
				  if ((size_t) history_index == m_history.size()) m_current_line_copy.pop_back();
			   }
			} if (ch1 == 91 && ch2 == 65) {
			   // This was an up arrow.
			   // We will print the line prior from history.
			   if (!m_history.size()) continue;
			   // if (history_index == -1) continue;
			   // Clear input so far
			   char ch[_line.size() + 1]; char sp[_line.size() + 1];
			   memset(ch, '\b',_line.size()); memset(sp, ' ', _line.size());
			   if (write(1, ch, _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   } else if (write(1, sp, _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   } else if (write(1, ch, _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   }

			   if ((size_t) history_index == m_history.size()) --history_index;
			   // Only decrement if we are going beyond the first command (duh).
			   _line = m_history[history_index];
			   history_index = (!history_index) ? history_index : history_index - 1;
			   // Print the line
			   if (_line.size()) _line.pop_back();
			   if (write(1, _line.c_str(), _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   }
			} if (ch1 == 91 && ch2 == 66) {
			   // This was a down arrow.
			   // We will print the line prior from history.
	  
			   if (!m_history.size()) continue;
			   if ((size_t) history_index == m_history.size()) continue;
			   // Clear input so far
			   for (size_t x = 0, bsp ='\b'; x < _line.size(); ++x) {
				  if (!write(1, &bsp, 1)) {
					 perror("write");
					 continue;
				  }
			   }
			   for (size_t x = 0, sp = ' '; x < _line.size(); ++x) {
				  if (!write(1, &sp, 1)) {
					 perror("write");
					 continue;
				  }
			   }
			   for (size_t x = 0, bsp ='\b'; x < _line.size(); ++x) {
				  if (!write(1, &bsp, 1)) {
					 perror("write");
					 continue;
				  }
			   }
	  
			   history_index = ((size_t) history_index == m_history.size()) ? m_history.size()
				  : history_index + 1;
			   if ((size_t) history_index == m_history.size()) _line = m_current_line_copy;
			   else _line = m_history[history_index];
			   if (_line.size() && (size_t) history_index != m_history.size()) _line.pop_back();
			   // Print the line
			   if (write(1, _line.c_str(), _line.size()) != (int) _line.size()) {
				  perror("write");
				  continue;
			   }
			} if (ch1 == 91 && ch2 == 67) {
			   /* Right Arrow Key */
			   if (!m_buff.size()) continue;

			   char wrt = m_buff.top();
			   if (!write(1, &wrt, 1)) {
				  perror("write");
				  continue;
			   }
			   _line += wrt; m_buff.pop();
			} if (ch1 == 91 && ch2 == 68) {
			   if (!_line.size()) continue;
			   /* Left Arrow Key */

			   /* grab width of terminal */
			   struct winsize w;
			   ioctl(1, TIOCGWINSZ, &w);
			   size_t term_width = w.ws_col;

			   /* check if we need to go up a line */
			   /*  The plus 2 comes from the "$ "  */
			   if ((_line.size() == (term_width - 2))) {
				  /* need to go up a line */		   
				  const size_t p_len = strlen("\033[1A\x1b[33;1m$ \x1b[0m");

				  /* now we print the string */
				  if (!write(1, "\033[1A\x1b[33;1m$ \x1b[0m", p_len)) {
					 /**
					  * @todo Make sure you print the correct prompt!
					  */
					 perror("write");
					 continue;
				  } else if (!write(1, _line.c_str(), term_width - 2)) {
					 perror("write");
					 continue;
				  }
			   } else if (!((_line.size() + 2) % (term_width))) {
				  /* This case is for more than one line of backtrack */
				  if (!write(1, "\033[1A \b", strlen("\033[1A \b"))) perror("write");
				  else if (!write(1, _line.c_str() - 2 + (term_width  * ((_line.size() - 2) / term_width)), term_width)) {
					 perror("write");
					 continue;
				  }
			   }
			   m_buff.push(_line.back());
			   char bsp = '\b';
			   if (!write(1, &bsp, 1)) {
				  perror("write");
				  continue;
			   }
			   _line.pop_back();
			}
		 }
      
	  }

	  _line += (char) 10 + '\0';
	  m_current_line_copy.clear();
	  m_history.push_back(_line);
   }

   char * getStashed() {
	  char * ret = (char*) calloc(m_stashed.size() + 1, sizeof(char));
	  strncpy(ret, m_stashed.c_str(), m_stashed.size());
	  ret[m_stashed.size()] = '\0';
	  m_get_mode = 1;
	  std::cerr<<"get returning: "<<ret<<std::endl;
	  return ret;
   }
  
   char * get() {
	  if (m_get_mode == 2) {
		 char * ret = (char*) calloc(m_stashed.size() + 1, sizeof(char));
		 strncpy(ret, m_stashed.c_str(), m_stashed.size());
		 ret[m_stashed.size()] = '\0';
		 m_get_mode = 1;
		 std::cerr<<"get returning: "<<ret<<std::endl;
		 return ret;
	  }
      
	  std::string returning;
	  returning = m_history[m_history.size() - 1];
	  m_history.pop_back();
	  char * ret = (char*) calloc(returning.size() + 1, sizeof(char));
	  strncpy(ret, returning.c_str(), returning.size());
	  ret[returning.size()] = '\0';
	  return ret;
   }

   void tty_raw_mode() {
	  tcgetattr(0,&oldtermios);
	  termios tty_attr = oldtermios;
	  /* Set raw mode. */
	  tty_attr.c_lflag &= (~(ICANON|ECHO));
	  tty_attr.c_cc[VTIME] = 0;
	  tty_attr.c_cc[VMIN] = 1;
    
	  tcsetattr(0,TCSANOW,&tty_attr);
   }

   void unset_tty_raw_mode() {
	  tcsetattr(0, TCSAFLUSH, &oldtermios);
   }

   void setFile(const std::string & _filename) {
	  std::string expanded_home = tilde_expand(_filename.c_str());

	  char * file = strndup(expanded_home.c_str(), expanded_home.size());

	  yyin = fopen(file, "r"); free(file);

	  if (yyin != NULL) {
		 Command::currentCommand.printPrompt = false;
		 yyrestart(yyin); yyparse();
		 fclose(yyin);

		 yyin = stdin;
		 yyrestart(yyin);
		 Command::currentCommand.printPrompt = true;
	  } Command::currentCommand.prompt();
	  yyparse();
   }

private:
   std::vector<std::string> string_split(std::string s, char delim) {
	  std::vector<std::string> elems; std::stringstream ss(s);
	  std::string item;
	  for (;std::getline(ss, item, delim); elems.push_back(std::move(item)));
	  return elems;
   }
   std::string m_current_path;
   std::string m_current_line_copy;
   std::string m_stashed;
   std::stack<char> m_buff;
   std::vector<std::string> m_path;
   std::vector<std::string> m_history;
   ssize_t history_index = 0;
   int fdin = 0;
   std::string m_filename;
   std::ifstream * m_ifstream;
   int m_get_mode;
   bool m_show_line = false;
   termios oldtermios;
};
#endif
