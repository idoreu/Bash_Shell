#include <iostream>
#include <csignal>
#include "signals.h"
#include "Commands.h"

using namespace std;


void ctrlCHandler(int sig_num) {
  cout <<"smash: got ctrl-C" << endl;
  if(foreGroundProcces_id != 0){
    if(kill(foreGroundProcces_id, SIGKILL) == 0){
      cout << "smash: process " << foreGroundProcces_id << " was killed" << endl;
    }else{
      perror("smash error: kill failed");
    }
  }
  foreGroundProcces_id = 0;
}

// void alarmHandler(int sig_num) {
//   cout << "smash: got an alarm" <<endl;
//   timedout = true;
// }

// struct TimedCommans {
//   std::string m_cmd_line;
//   int m_timeOut;

//   TimedCommans(std::string cmd_line, int timeOut) : m_cmd_line(cmd_line), m_timeOut(timeOut) {}

//   bool operator<=(const TimedCommans& otherCommand) const{
//     return this->m_timeOut <= otherCommand.m_timeOut;
//   }
// };

