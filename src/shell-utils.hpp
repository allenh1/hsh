#ifndef __SHELL_UTILS_HPP__
#define __SHELL_UTILS_HPP__
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <pwd.h>

#include <sstream>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

#include "scope_exit.hpp"

std::string longest_substring(const std::vector<std::string> & _vct);
size_t size_of_longest(const std::vector<std::string> & _vct);
void printEvenly(std::vector<std::string> & _vct);
std::string tilde_expand(std::string input);
std::string replace(std::string str, const char * sb, const char * rep);
std::string env_expand(std::string s);
std::vector<std::string> vector_split(std::string s, char delim);
bool changedir(std::string & s);
bool is_directory(std::string s);

/**
 * Adapted from:
 *  https://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 */
timeval operator - (timeval & t1, timeval & t2);

/**
 * Stolen from: bash
 *  This function is how bash subtracts times
 */
timeval & difftimeval(timeval & d, timeval & t1, timeval & t2);

/**
 * Stolen from: bash
 *  This is how bash adds times
 */
timeval & addtimeval(timeval & d, timeval & t1, timeval & t2);

/**
 * Stolen from: bash
 *  This converts a timeval * to seconds and thousandths of a second.
 */
void timeval_to_secs(timeval & tvp, time_t & sp, int & sfp);

struct Lensort {
	bool operator () (char*& ch1, char*& ch2) { return strlen(ch1) < strlen(ch2); }
};
#endif
