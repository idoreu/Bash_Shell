#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <sys/types.h>
#include <memory>
#include <string>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

extern pid_t foreGroundProcces_id;
// extern timedout = false;


class Command {
  const static int MAX_LINE_LEN = 80;
  const static int MAX_ARG_NUM = 20;
  char *m_cmd_line;
  pid_t m_pid;
 public:
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  virtual pid_t get_pid();
  char* get_cmd();
  int max_args_num();
  int max_line_len();
  void set_pid(pid_t pid);
};

class BuiltInCommand : public Command {
  public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand(){}
  virtual bool argument_num_test(int n = 1); // by default if there is more then one argument return false
  bool job_id_isInt(int* Id);
  
};
///________________________
class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;

  bool is_simple_command();
  bool is_bg_command();
  //void prepare_for_complax(char** args);
  void prepare_for_simple(char** args);
};

class JobsList;

class PipeCommand : public Command {
  public:
  JobsList *m_jobsList;
  PipeCommand(const char* cmd_line , JobsList* jobsList);
  virtual ~PipeCommand() {}
  void execute() override;
  //
  //pid_t get_pid() override;
  Command *createCommand(bool firstCommand, bool regularPipe);
  //void setJobsList(JobsList *jobs);
  void redirectPipe(bool append, pid_t pid);
};

class RedirectionCommand : public Command {
  private:
  
 JobsList *m_jobsList;
 public:
  explicit RedirectionCommand(const char* cmd_line, JobsList* jobsList);
  virtual ~RedirectionCommand() {}
  void execute() override;

  Command *createCommand();
  // void setJobsList(JobsList *jobs);
  void redirect(Command* command, bool append, pid_t pid);
};

class ChangeDirCommand : public BuiltInCommand {
  private:
  char** m_plastPwd;
  public:
// TODO: Add your data members public:
  ChangeDirCommand(const char* cmd_line, char** plastPwd = nullptr);
  virtual ~ChangeDirCommand() {}
  void execute() override;
  //___
  bool compare_second_arg(char *cmpr);
  //bool argument_num_test();
  void switch_cstr(char *&str1, char *&str2);
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};
//___________________________________//

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class QuitCommand : public BuiltInCommand {
  private:
  JobsList *m_jobs;
  public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
  //bool argument_num_test();
  bool kill_is_specified();
};




class JobsList {
 public:
  class JobEntry {
    private:
    Command* m_cmd;
    bool m_isStopped;
    int m_jobId;
    pid_t m_pid;
    public:
    JobEntry(Command *cmd, bool isStopped, int jobId, pid_t pid= 0) : m_cmd(cmd), m_isStopped(isStopped),
          m_jobId(jobId), m_pid(pid) {}
    pid_t getpid() const{return m_pid;}
    int getJobId() const { return m_jobId;}
    //void setPid(pid_t pid){m_pid = pid;}
    char *get_cmd() const {return m_cmd->get_cmd();}
    void printJob();

    void setStop(){m_isStopped = true;}
    bool isJobStopped()const{return m_isStopped;}
  };
  private:
  static const int MAX_JOBS_NUM = 100;
  int max_job_id;
  std::vector<JobEntry> m_jobsList;
 // TODO: Add your data members
 public:
  JobsList();
  ~JobsList(){}
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  //JobEntry *getLastStoppedJob(int *jobId);
  //void getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
  bool is_empty();
  bool job_is_valid(int Id);
  //int working_jobs_num();
  void setMaxId();
  bool is_stopped(int jobId);
  int getMaxId();
  int working_jobs_num();
};

class JobsCommand : public BuiltInCommand {
 private:
  JobsList *m_jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
  //bool argument_num_test();
};

class KillCommand : public BuiltInCommand {
 private:
  JobsList *m_jobs;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
  //bool argument_num_test() override;
  bool sig_is_valid(int *signal);
  //bool argument_num_test();
};

class ForegroundCommand : public BuiltInCommand {
 private:
  JobsList *m_jobs;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
  //bool argument_num_test();
};

class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
  int get_mod(char* c_str);
  //bool argument_num_test();
};


class SmallShell {
 private:
  std::string DEFAULT_PROMPT;
  JobsList *m_jobs;
  std::string m_prompt;
  char** m_plastPwd;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  // TODO: add extra methods as needed
  std::string get_prompt();
  
};

#endif //SMASH_COMMAND_H_
