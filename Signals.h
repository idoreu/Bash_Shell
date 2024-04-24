#ifndef SMASH__SIGNALS_H_
#define SMASH__SIGNALS_H_
#include <map>
#include <sys/types.h>


void ctrlCHandler(int sig_num);
// void alarmHandler(int sig_num);

// struct TimedCommand {
//     int duration;
//     string s_cmdLine;
// };

// struct TimedList {
//     std::map<pid_t, TimedCommand> m_commands;
//     public:

// };

#endif //SMASH__SIGNALS_H_
