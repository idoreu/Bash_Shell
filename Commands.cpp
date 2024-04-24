#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <sys/wait.h>
#include <iomanip>
#include <algorithm>
#include "Commands.h"
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <cctype>
#include <fcntl.h>

#define WHITESPACE " \t\n\r\f\v"
using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

pid_t foreGroundProcces_id;

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

//memory needs to be cleand;
int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

void _cleanArgs(char** args, int n = 20){
  for(int i = 0; i < n; ++i){
    if(args[i] == nullptr){
      break;
    }
    free(args[i]);
  }
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  size_t lastNotWhite = str.find_last_not_of(WHITESPACE);
  if(lastNotWhite!= string::npos && str[lastNotWhite] == '&'){
    return true;
  }
  return false;
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool _is_arg_digit(char c, int* value){
  int value_check = c - '0';
  if(value_check < 0 || 9 < value_check){
    *value = -1;
    return false;
  }
  *value = value_check;
  return true;
}

std::string _sanitize_command(const std::string& cmd) {
  const std::string unsafe_chars = ";`$()<>\\\"";
  std::string sanitized_cmd = _trim(cmd);
  for (char& c : sanitized_cmd) {
    if (unsafe_chars.find(c) != std::string::npos) {
        c = ' ';
    }
  }
  return sanitized_cmd;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : m_jobs(new JobsList()) {
  this->DEFAULT_PROMPT = string("smash> ");
  this->m_prompt = this->DEFAULT_PROMPT;
  this->m_plastPwd = new char*[2];
  for(int i = 0; i < 2; ++i){
    this->m_plastPwd[i] = nullptr;
  }
}

SmallShell::~SmallShell() {
  if(m_jobs != nullptr){
    delete m_jobs;
  }
  for(int i = 0; i < 2; ++i){
    if(this->m_plastPwd[i] != nullptr){
      delete[] this->m_plastPwd[i];
    }
  }
  delete[] this->m_plastPwd;
}

std::string SmallShell::get_prompt(){
  return m_prompt;
}
/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
  string cmd_s = string(cmd_line);
  cmd_s = _trim(cmd_line);
  string first_word = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  //___Simple commands, none jobs:
  if(first_word.compare("chprompt") == 0){
    if(cmd_s.compare("chprompt") == 0){
      this->m_prompt = string("smash> ");
    } else {
      string new_prompt = _trim(cmd_s.substr(8));
      first_word = new_prompt.substr(0, new_prompt.find_first_of(" \n"));
      first_word += string("> ");
      this->m_prompt = first_word;
    }
    return nullptr;
  } else if(first_word.compare("^C") == 0){
    pid_t pid = getpid();
    kill(pid, SIGINT);
  }//___Redirect commands:
  else if(cmd_s.find(">") != string::npos){
    //assuming this function doesn't alternate cmd_line
    return new RedirectionCommand(cmd_line, this->m_jobs);
  }//___Piped commands:
  else if(cmd_s.find("|") != string::npos){
    return new PipeCommand(cmd_line, this->m_jobs);
  }
  // Simpler commands
  if(first_word.compare("showpid") == 0){
    return new ShowPidCommand(cmd_line);
  } else if(first_word.compare("pwd") == 0){
    return new GetCurrDirCommand(cmd_line);
  } else if(first_word.compare("cd") == 0){
    return new ChangeDirCommand(cmd_line, this->m_plastPwd);
  }//___Jobs commands: 
  else if(first_word.compare("jobs") == 0){
    return new JobsCommand(cmd_line, this->m_jobs);
  } else if(first_word.compare("fg") == 0){
    return new ForegroundCommand(cmd_line, this->m_jobs);
  } else if(first_word.compare("quit") == 0){
    return new QuitCommand(cmd_line, this->m_jobs);
  } else if(first_word.compare("kill") == 0){
    return new KillCommand(cmd_line, this->m_jobs);
  }
  //___change mod command:
  else if(first_word.find("chmod") != string::npos){
    return new ChmodCommand(cmd_line);
  // } else if(first_word.find("timeout") != string::npos){
  //   pass;//the timeout handeling should be here;
  } else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;// I don't know how to refer to that
}

void SmallShell::executeCommand(const char *cmd_line) {
  Command *command = this->CreateCommand(cmd_line);
  if(command == nullptr){
    return;
  }
  //dynamic cast to find out what type of command it is
  BuiltInCommand *buildType = dynamic_cast<BuiltInCommand*>(command);
  if(buildType){
    buildType->execute();
    foreGroundProcces_id = buildType->get_pid();
    return;
  }
  ExternalCommand *externCommand = dynamic_cast<ExternalCommand*>(command);
  if(externCommand){
    externCommand->execute();
    foreGroundProcces_id = externCommand->get_pid();
    if(externCommand->is_bg_command()){
      foreGroundProcces_id = externCommand->get_pid();
      this->m_jobs->addJob(externCommand);
    }
    return;
  }
  PipeCommand *pipeCommand = dynamic_cast<PipeCommand*>(command);
  if(pipeCommand){
    ///p/ipeCo/mm/and->setJobsList(this->m_jobs);
    pipeCommand->execute();
    foreGroundProcces_id = pipeCommand->get_pid();
    return;
  }
  RedirectionCommand *redircommand = dynamic_cast<RedirectionCommand*>(command);
  if(redircommand){
    //redircommand->setJobsList(this->m_jobs);
    redircommand->execute();
    foreGroundProcces_id = redircommand->get_pid();
    return;
  }
}

//_________________ Built in commands Section ______________________
// ShowPid::
ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute(){
  cout << "smash pid is " << getpid() <<endl;
}

// GetCurrentDir::
GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) :BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute(){
  char *pwd;
  int BUFFER = 1024;

  while(true){
    pwd = new char[BUFFER];
    if(getcwd(pwd, BUFFER) != nullptr){
      break;
    }
    if(errno == ERANGE){
      BUFFER *= 2;
    } else {
      perror("smash error: getcwd failed");
      break;
    }
  }
  cout << pwd << endl;
  delete[] pwd;
}

//ChangeDirCommand::
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line),
        m_plastPwd(plastPwd) {}

void ChangeDirCommand::switch_cstr(char *&str1, char *&str2){
  char *temp = str1;
  str1 = str2;
  str2 = temp;
}

void ChangeDirCommand::execute(){
  const char *change_to_last_working_directory = "-";
  const char *change_to_parent_directory = "..";
  //
  if(!this->argument_num_test(2)){
    cerr << "smash error: cd: too many arguments" << endl;
    return;
  }
  if(this->m_plastPwd[1] == nullptr){
    m_plastPwd[1] = new char[1024];
    getcwd(m_plastPwd[1], 1024);
  }
  std::string s_cmd = _trim(string(this->get_cmd()));
  std::string symbol = _trim(s_cmd.substr(2));
  if(symbol.compare(change_to_last_working_directory) == 0){//the cmd_line is "cd -"
    if(m_plastPwd[0] == nullptr){ //m_plastPwd[0] is the last pwd and m_plastPwd[1] is the current
      cerr << "smash error: cd: OLDPWD not set" << endl;
    } else {
      if(chdir(m_plastPwd[0]) != 0){
        perror("smash error: chdir failed");
      }
      this->switch_cstr(m_plastPwd[0], m_plastPwd[1]);
    }
  } else if(symbol.compare(change_to_parent_directory) == 0){ // system got "cd .."
    if(chdir("..") == 0 ){
      string new_path = string(m_plastPwd[1]);
      size_t lastSlash = new_path.find_last_of('/');
      if(lastSlash != string::npos){
        new_path = new_path.substr(0, lastSlash);
      } else {
        new_path = string("/");
      }
      this->switch_cstr(m_plastPwd[0], m_plastPwd[1]);
      if(m_plastPwd[1] != nullptr){
        delete[] m_plastPwd[1];
      }
      m_plastPwd[1] = new char[new_path.length() +1];
      strcpy(m_plastPwd[1], new_path.c_str());
    } else {
      perror("smash error: chdir failed");
    }
    return;
  } else {
    string new_path = _trim(s_cmd.substr(3));
    if(chdir(new_path.c_str()) == 0){
      this->switch_cstr(m_plastPwd[0], m_plastPwd[1]);
      if(m_plastPwd[1] != nullptr){
        delete[] m_plastPwd[1];
      }
      m_plastPwd[1] = new char[new_path.length() +1];
      strcpy(m_plastPwd[1], new_path.c_str());
    } else {
      perror("smash error: chdir failed");
    }
    }
}

//__________________ JobsList commands ____________

//JobsLis::
JobsList::JobsList() : max_job_id(0), m_jobsList() {}

void JobsList::addJob(Command* cmd, bool isStopped){
  this->max_job_id += 1;
  this->m_jobsList.push_back(JobsList::JobEntry(cmd, isStopped, this->max_job_id ,cmd->get_pid()));
}

void JobsList::printJobsList(){
  for(const auto& job : this->m_jobsList){
    cout << "[" <<job.getJobId() <<"] " << (job.get_cmd()) << endl;
  }
}

void JobsList::removeJobById(int jobId) {
  if(jobId == this->max_job_id){
    this->max_job_id -= 1;
  }
  for (auto it = m_jobsList.begin(); it != m_jobsList.end(); ++it) {
    if (it->getJobId() == jobId) {
      m_jobsList.erase(it);
      return;
    }
  }
  //this->setMaxId();
}

bool JobsList::is_stopped(int jobId){
  int status;
  JobsList::JobEntry &job = m_jobsList[jobId];
  pid_t job_pid = job.getpid();
  bool check = job.isJobStopped();
  pid_t pid = waitpid(job_pid, &status, WNOHANG);
  if(pid == job_pid || check){
    return true;
  }
  return false;
}

int JobsList::getMaxId(){
  if(this->m_jobsList.empty()){
    return 0;
  }
  JobsList::JobEntry &job = this->m_jobsList.back();
  return job.getJobId();
}

void JobsList::removeFinishedJobs() {
  //int num_of_jobs = this->m_jobsList.size();
  for(int jobId = this->m_jobsList.size() -1; jobId >= 0; --jobId){
    if(this->is_stopped(jobId)){
      this->m_jobsList.erase(m_jobsList.begin()+ jobId);
    }
  }
  this->max_job_id = this->getMaxId();
}

void JobsList::killAllJobs(){
  for(auto& job :m_jobsList){
    pid_t pid = job.getpid();
    if(pid > 0){
      kill(pid, SIGKILL);
      job.setStop();
      cout << job.getpid() << ": " << job.get_cmd() <<endl;
    }
  }
  this->removeFinishedJobs();
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
  for (auto& job : m_jobsList) {
    if (job.getJobId() == jobId) {
      return &job;
    }
  }
  return nullptr;
}


JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
  if (m_jobsList.empty()) {
    *lastJobId = -1;
    return nullptr;
  }
  JobsList::JobEntry* lastJob = this->getJobById(this->max_job_id);
  *lastJobId = lastJob->getJobId();
  return lastJob;
}

bool JobsList::is_empty() {
  return m_jobsList.empty();
}

bool JobsList::job_is_valid(int Id){
  if(Id >= this->MAX_JOBS_NUM || Id < 0){
    return false;
  }
  JobsList::JobEntry *job = this->getJobById(Id);
  if(job == nullptr){
    return false;
  }
  return true;
}

int JobsList::working_jobs_num(){
  int counter = 0;
  for(auto&job : m_jobsList){
    if(!job.isJobStopped()){
      counter +=1;
    }
  }
  return counter;
}


//JobsCommand::
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), m_jobs(jobs) {}


void JobsList::JobEntry::printJob(){
  cout << this->get_cmd() << " " << this->getpid() <<endl;
}

void JobsCommand::execute(){
  m_jobs->removeFinishedJobs();
  m_jobs->printJobsList();
}


///MULtipule problems in forground

//ForegroundCommand::
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), m_jobs(jobs) {}

void ForegroundCommand::execute(){
  this->m_jobs->removeFinishedJobs();
  //
  bool one_argument = argument_num_test(1);
  if (!argument_num_test(2) && !one_argument) {
    cerr << "smash error: fg: invalid arguments" << endl;
    return;
  }
  if(m_jobs->is_empty() && one_argument) {
    cerr << "smash error: fg: jobs list is empty" << endl;
    return;
  }
  int jobId = -1;
  if(one_argument){
    this->m_jobs->getLastJob(&jobId);
  } else {
    std::string cmd_s = _trim(string(this->get_cmd()));
    std::string jobId_s = _trim(cmd_s.substr(cmd_s.find_first_of(" \n")));
    try{
      jobId = std::stoi(jobId_s);
    }
    catch(...){
      jobId = -1;
    }
  }
  if (jobId < 0) {
    cerr << "smash error: fg: invalid arguments" << endl;
    return;
  }
  if (!m_jobs->job_is_valid(jobId)) {
    cerr << "smash error: fg: job-id " << jobId << " does not exist" << endl;
    return;
  }
  JobsList::JobEntry* job = m_jobs->getJobById(jobId);
  if(job != nullptr){
    job->printJob();
    pid_t pid = job->getpid();
    foreGroundProcces_id = pid;
    int status;
    pid_t wait_pid = waitpid(pid, &status, 0);
    if (wait_pid == -1) {
      perror("smash error: waitpid failed");
      return;
    }
    this->m_jobs->removeJobById(jobId);
  }
}

//QuitCommand::
QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), m_jobs(jobs) {}

void QuitCommand::execute(){
  this->m_jobs->removeFinishedJobs();
  //
  std::string s_cmd(get_cmd());
  if(s_cmd.find("kill") != string::npos){
    cout << "smash: sending SIGKILL signal to " << m_jobs->working_jobs_num() << " jobs:" <<endl;
    this->m_jobs->killAllJobs();
  }
  //
  exit(0);
}

//KillCommand::
KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line), m_jobs(jobs) {}

bool KillCommand::sig_is_valid(int *signal){
  char *cmd_line = new char[this->max_line_len()];
  if(cmd_line == nullptr){
    return false;
  }
  strcpy(cmd_line, this->get_cmd());
  _removeBackgroundSign(cmd_line);
  //
  char **args = new char*[this->max_args_num()];
  int temp;
  int signal_value;
  if(_parseCommandLine(cmd_line, args) == 3){
    if(args[1][0] == '-' && _is_arg_digit(args[1][1], &temp)){
      signal_value = temp;
      if(_is_arg_digit(args[1][2], &temp)){
        signal_value *= 10;
        signal_value += temp;
      }
      *signal = signal_value;
      _cleanArgs(args);
      delete[] cmd_line;
      delete[] args;
      return true;
    }
  }
  *signal = -1;
  _cleanArgs(args);
  delete[] cmd_line;
  delete[] args;
  return false;
}

void KillCommand::execute(){
  m_jobs->removeFinishedJobs();
  int sig;
  int jobId;
  int num_of_arguments = 3;
  if(this->argument_num_test(num_of_arguments)){
    if(this->sig_is_valid(&sig)){
      if(this->job_id_isInt(&jobId)){
        if(m_jobs->job_is_valid(jobId)){
          if(m_jobs->getJobById(jobId)->getpid() > 0){
            if(kill(m_jobs->getJobById(jobId)->getpid(), sig) == 0){
              cout << "signal number " << sig << " was sent to pid " << m_jobs->getJobById(jobId)->getpid() <<endl;
              return;
            }
          }
          perror("smash error: kill failed");
        } else {
          cerr << "smash error: kill: job-id " << jobId << " does not exist" <<endl;
        }
      } else{
        cerr << "smash error: kill: invalid arguments" << endl;
      }
    }else {
      cerr << "smash error: kill: invalid arguments" << endl;
    }
  } else {
    cerr << "smash error: kill: invalid arguments" << endl;
  }
}
//External Commands::________________________________________
ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line){}

bool ExternalCommand::is_simple_command(){
  string str(this->get_cmd());
  for(char c : str){
    if( c == '*' || c == '?'){
      return false;
    }
  }
  return true;
}

bool ExternalCommand::is_bg_command(){
  return _isBackgroundComamnd(this->get_cmd());
}

void ExternalCommand::prepare_for_simple(char** args){
  std::string cmd_s(this->get_cmd());
  cmd_s = _trim(cmd_s);
  if(_isBackgroundComamnd(this->get_cmd())){
    cmd_s.pop_back();
  }
  _parseCommandLine(cmd_s.c_str(), args);
}

void ExternalCommand::execute(){
  char **args = new char*[this->max_args_num() +2]; // one for the termination of exec, the other for "-c"
  pid_t pid = fork();

  if(pid < 0 ){
    perror("smash error: fork failed");
  } else if(pid == 0){// child procces
    //
    setpgrp();
    if(this->is_simple_command()){
      this->prepare_for_simple(args);
      execvp(args[0], args);
      perror("smash error: execvp failed");
    } else {
      string s_cmd = _sanitize_command(string(get_cmd()));
      if(_isBackgroundComamnd(s_cmd.c_str())){
        s_cmd.pop_back();
      }
      execl("/bin/bash", "/bin/bash", "-c", s_cmd.c_str(), nullptr);
      perror("smash error: execl failed");
    }
    _cleanArgs(args, this->max_args_num() + 2);
    exit(EXIT_FAILURE);
    //
  } else { // Parent procces
    this->set_pid(pid);
    foreGroundProcces_id = pid;
    int status;
    int option = this->is_bg_command() ? WNOHANG : 0;
    if(waitpid(pid, &status, option) == -1){
      perror("smash error: waitpid failed");
    }
    
  }
  _cleanArgs(args, this->max_args_num() + 2);
}

//RedirectionCommand::
RedirectionCommand::RedirectionCommand(const char* cmd_line, JobsList* jobsList) : Command(cmd_line), m_jobsList(jobsList){}

// void RedirectionCommand::setJobsList(JobsList *jobs){
//   this->m_jobsList = jobs;
// }

Command *RedirectionCommand::createCommand() {
  std::string full_command = _trim(string(get_cmd()));
  string command;
  std::istringstream iss(full_command);
  iss >> command;
  if(_isBackgroundComamnd(command.c_str())){
    command = command.substr(0, command.find_first_of("&"));
  }  
  // Check for known commands
  if (command.find("showpid") != string::npos) {
    return new ShowPidCommand(get_cmd());
  } else if (command == "pwd") {
    return new GetCurrDirCommand(get_cmd());
  } else if (command == "cd") {
    return new ChangeDirCommand(get_cmd());
  } else if (command == "jobs") {
    return new JobsCommand(get_cmd(), this->m_jobsList);
  } else if (command == "fg") {
    return new ForegroundCommand(get_cmd(), this->m_jobsList);
  } else if (command == "quit") {
    return new QuitCommand(get_cmd(), this->m_jobsList);
  } else if (command == "kill") {
    return new KillCommand(get_cmd(), this->m_jobsList);
  } else if(command == "chprompt"){
    return nullptr;
  } else {
    return new ExternalCommand(get_cmd());
  }
}

void RedirectionCommand::redirect(Command* command, bool append, pid_t pid){ // This is happening in a child proccess
  int fd;
  int originalOutput = dup(STDOUT_FILENO);
  string s_cmd = _trim(string(get_cmd()));
  std::string targetFile = _trim(s_cmd.substr(s_cmd.find_last_of(">") +1));
  //
  if(originalOutput == -1){
    perror("smash error: dup failed");
    exit(EXIT_FAILURE);
  }
  if(append){
    fd = open(targetFile.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
  } else {
    fd = open(targetFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  }
  //redirecting
  if(fd == -1){
    perror("smash error: open failed");
    exit(EXIT_FAILURE);
  }
  if(dup2(fd, STDOUT_FILENO) == -1){
    perror("smash error: dup2 failed");
    close(fd);
    exit(EXIT_FAILURE);
  }
  if(close(fd) == -1){
    perror("smash error: close failed");
    exit(EXIT_FAILURE);
  }
  if(command == nullptr){ // could be either an error or because it is a change prompt command
    std::istringstream iss(s_cmd);
    std::string word;
    iss >> word;
    if(word == "chprompt"){
      iss >> word;
      cout << word;
    }
  } else {
    ShowPidCommand* show_check = dynamic_cast<ShowPidCommand*>(command);
    if(show_check){
      cout << pid << endl;
    } else {
      command->execute();
    }
  }
  //closing the redirection
  if(dup2(originalOutput, STDOUT_FILENO) == -1){
    perror("smash error: dup2 failed");
    close(originalOutput);
    exit(EXIT_FAILURE);
  }
  if(close(originalOutput) == -1){
    perror("smash error: close failed");
    exit(EXIT_FAILURE);
  }
  exit(EXIT_SUCCESS);
}

void RedirectionCommand::execute(){

  string s_cmd = string(get_cmd());
  Command* command = this->createCommand();
  pid_t smash_pid = getpid();
  pid_t pid = fork();
  if(pid < 0){
    perror("smash error: fork failed");
  } else if(pid == 0){
    setpgrp();
    bool append = true;
    if(s_cmd.find(">>") != string::npos){ // appending the output to the target file
      this->redirect(command, append, smash_pid);
    } else {
      this->redirect(command, !append, smash_pid);
    }
  } else{ // father procces
    int input;
    this->set_pid(pid);
    if(waitpid(pid, &input, 0) == -1){
      perror("smash error: waitpid failed");
    }
  }
}//getpid

//PipeCommand::
PipeCommand::PipeCommand(const char* cmd_line, JobsList* jobsList) : Command(cmd_line), m_jobsList(jobsList) {}

Command *PipeCommand::createCommand(bool firstCommand, bool regularPipe) {
  std::string full_command = _trim(string(get_cmd()));
  std::string command;
  
  if(!firstCommand){
    size_t startPos = (regularPipe ) ? full_command.find_first_of("|") : full_command.find_first_of("&");
    command = _trim(full_command.substr(startPos +1));
  } else {
    command = _trim(full_command.substr(0, full_command.find("|")));
  }
  
  // Check for known commands
  if (command.find("showpid") != string::npos) {
    return new ShowPidCommand(get_cmd());
  } else if (command == "pwd") {
    return new GetCurrDirCommand(get_cmd());
  } else if (command == "cd") {
    return new ChangeDirCommand(get_cmd());
  } else if (command == "jobs") {
    return new JobsCommand(get_cmd(), this->m_jobsList);
  } else if (command == "fg") {
    return new ForegroundCommand(get_cmd(), this->m_jobsList);
  } else if (command == "quit") {
    return new QuitCommand(get_cmd(), this->m_jobsList);
  } else if (command == "kill") {
    return new KillCommand(get_cmd(), this->m_jobsList);
  } else if (command == "chprompt") {
    return nullptr;
  } else {
    return new ExternalCommand(get_cmd());
  }
}

void PipeCommand::redirectPipe(bool regular, pid_t pid){
  int pipefd[2];
  if(pipe(pipefd)){
    perror("smash error: pipe failed");
    return;
  }
  // regular pipe redirects pipe write (pipefd[1]) to stdout (1), and none regular does it to stderr (2)
  int redirectSource = regular ? 1 : 2;
  bool firstCommand = true;
  if(fork() == 0){// execute first command
    if(dup2(pipefd[1], redirectSource) == -1){
      perror("smash error: dups failed");
      exit(EXIT_FAILURE);
    }
    if(close(pipefd[0]) == -1){
      perror("smash error: close failed");
      exit(EXIT_FAILURE);
    }
    if(close(pipefd[1]) == -1){
      perror("smash error: close failed");
      exit(EXIT_FAILURE);
    }
    //
    Command *command = this->createCommand(firstCommand, regular);
    if(command == nullptr){ // could be either an error or because it is a change prompt command
      std::istringstream iss(this->get_cmd());
      std::string word;
      iss >> word;
      if(word == "chprompt"){
        iss >> word;
        cout << word;
      } else {
        cerr << "smash error: unexpected command" << endl;
      }
    } else {
      ShowPidCommand *show_check = dynamic_cast<ShowPidCommand*>(command);
      if(show_check){
        cout << pid << endl;
      } else{
        command->execute();
      }
      exit(EXIT_SUCCESS);
    }
    exit(EXIT_FAILURE);
  }
  if(fork() == 0){
    if(dup2(pipefd[0], 0) == -1){
      perror("smash error: dups failed");
      exit(EXIT_FAILURE);
    }
    if(close(pipefd[0]) != -1 && close(pipefd[1] != -1)){
      Command *command = this->createCommand(!firstCommand, regular);
      if(command != nullptr){
        ShowPidCommand *show_check = dynamic_cast<ShowPidCommand*>(command);
        if(show_check){
          cout << pid << endl;
        } else{
          command->execute();
        }
        exit(EXIT_SUCCESS);
      }
    }
    exit(EXIT_FAILURE);
  }
  close(pipefd[0]);
  close(pipefd[1]);
}


void PipeCommand::execute(){
  pid_t pid = getpid();
  string s_cmd = _trim(string(get_cmd()));
  size_t pipeSymbolPos = s_cmd.find_first_of("|") +1;
  string pipeType = _trim(s_cmd.substr(pipeSymbolPos, pipeSymbolPos + 2));
  bool regularPipe = false;
  if(pipeType == "|&"){// command1.err to commnad2.in
    this->redirectPipe(regularPipe, pid);
  } else {
    this->redirectPipe(!regularPipe, pid);
  }
}

//ChmodCommand::
ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

int ChmodCommand::get_mod(char* c_str){
  int mod = -1;
  try {
    mod = std::stoi(std::string("0") + std::string(c_str), nullptr, 8);
  }catch(const invalid_argument& e){
    return -1;
  }
  catch(const out_of_range& e){
    return -1;
  }
  return mod;
}

void ChmodCommand::execute(){
  char** args = new char*[this->max_args_num()];
  int args_num = _parseCommandLine(get_cmd(), args);
  if(args_num == 3){
    int mod = this->get_mod(args[1]);
    if(mod == -1 || chmod(args[2], mod) == -1){
      perror("smash error: chmod failed");
    }
  } else{ 
    cerr << "smash error: chmod: invalid aruments" <<endl;
  }
  _cleanArgs(args);
  delete[] args;
}

//Command::_________________________________________________
Command::Command(const char* cmd_line){
  this->m_cmd_line = new char[this->max_line_len()];
  strcpy(this->m_cmd_line, cmd_line);
  this->m_pid = 0;
}

Command::~Command(){
  if(this->m_cmd_line != nullptr){
    delete[] this->m_cmd_line;
  }
}

void Command::set_pid(pid_t pid){
  this->m_pid = pid;
}

char* Command::get_cmd(){
  return this->m_cmd_line;
}

pid_t Command::get_pid(){
  return m_pid;
}

int Command::max_args_num(){
  return this->MAX_ARG_NUM;
}

int Command::max_line_len(){
  return this->MAX_LINE_LEN;
}

//BuiltInCommand::
BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line){}

bool BuiltInCommand::argument_num_test(int n){ //By default the built in commands check for argument argument
  bool ans;
  char **args = new char*[this->max_args_num()];
  if(_parseCommandLine(get_cmd(), args) == n){
    ans = true;
  } else {
    //cerr << "smash error:> \"" << this->get_Cmd() <<"\"" << endl;
    ans = false;
  }
  _cleanArgs(args);
  delete[] args;
  return ans;
}

bool BuiltInCommand::job_id_isInt(int *Id){
  //
  char **args = new char*[this->max_args_num()];
  if(args != nullptr)
  if(_parseCommandLine(get_cmd(), args) < 2){
    _cleanArgs(args);
    delete[] args;
    return false;
  }
  if(args[1] == nullptr || strcmp(args[1], "\0") == 0){
    _cleanArgs(args);
    delete[] args;
    return false;
  }
  try{
    *Id = stoi(string(args[2])); /// This is false, the id is in args[2]
  }
  catch(const invalid_argument& e){
    *Id = -1;
    _cleanArgs(args);
    delete[] args;
    return false;
  }
  catch(const out_of_range& e){
    *Id = -1;
    _cleanArgs(args);
    delete[] args;
    return false;
  }
  _cleanArgs(args);
  delete[] args;
  return true;
}

//
