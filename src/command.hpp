#ifndef command_h
#define command_h
#include <functional>
#include <typeinfo>
#include <vector>
#include <memory>

//#include "shell-readline.hpp"

// Command Data Structure
struct SimpleCommand
{
  SimpleCommand();

  std::vector<std::shared_ptr<char> > arguments;
  void insertArgument(char * argument);
  ssize_t numOfArguments = 0;
  void release() {
    for (int x = 0; x < arguments.size(); ++x) delete arguments[x].get();
    arguments.clear();
    arguments.shrink_to_fit();
    numOfArguments = 0;
  };
};

class Command
{
public:
  Command();

  void prompt();
  void print();
  void execute();
  void clear();

  void ctrlc_handler(int);
  void sigchld_handler(int);
	       
  void insertSimpleCommand(std::shared_ptr<SimpleCommand> simpleCommand);

  void setInFile(char * fd)  { inFile  = std::unique_ptr<char>(fd);  inSet = true; }
  void setOutFile(char * fd) { outFile = std::unique_ptr<char>(fd); outSet = true; }
  void setErrFile(char * fd) { errFile = std::unique_ptr<char>(fd); errSet = true; }

  void subShell(char * arg);

  const bool & outIsSet() { return outSet; }
  const bool & errIsSet() { return errSet; }
  const bool & inIsSet()  { return inSet; }

  void setAppend(const bool & ap) { append = ap; }
  void setBackground(bool && bg) { background = bg; }

  static Command currentCommand;
  static std::shared_ptr<SimpleCommand> currentSimpleCommand;
  
  std::vector<std::string> wc_collector;// Wild card collection tool
  
private:
  std::unique_ptr<char> outFile;
  std::unique_ptr<char> inFile;
  std::unique_ptr<char> errFile;

  bool append = false;
  bool background = false;
  int numOfSimpleCommands = 0;

  bool outSet = false;
  bool errSet = false;
  bool inSet  = false;

  std::vector<std::shared_ptr<SimpleCommand> > simpleCommands;
  std::vector<std::string> m_history;
};

/**
 * For the sorting of the wild card output
 */
struct Comparator {
  char toLower(char c) { return ('A' <= c && c <= 'Z') ? 'a' + (c - 'A') : c; }
  bool isAlnum(char c) { return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')
      || ('0' <= c && c <= '9'); }

  bool operator() (const std::string & s1, const std::string & s2) {
    const char * temp1 = s1.c_str(), *temp2 = s2.c_str();
    if (s1 == "files" && s2 == "file-list") return false;
    for (; *temp1 && *temp2; ++temp1, ++temp2) {
      if (*temp1 != *temp2) {
	if (!(isAlnum(*temp1) && isAlnum(*temp2))) {
	  if (!isAlnum(*temp1) && isAlnum(*temp2)) return false;
	  else if (isAlnum(*temp1) && !isAlnum(*temp2)) return true;
	  else return *temp1 < *temp2;
	}
	if (toLower(*temp1) != toLower(*temp2)) return toLower(*temp1) < toLower(*temp2);
      }
    }
    return s1.size() < s2.size();
  }
};
#endif
