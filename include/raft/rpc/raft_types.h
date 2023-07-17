/**
 * Autogenerated by Thrift Compiler (0.18.1)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef raft_TYPES_H
#define raft_TYPES_H

#include <iosfwd>

#include <thrift/Thrift.h>
#include <thrift/TApplicationException.h>
#include <thrift/TBase.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TTransport.h>

#include <functional>
#include <memory>




struct ServerState {
  enum type {
    FOLLOWER = 0,
    CANDIDAE = 1,
    LEADER = 2
  };
};

extern const std::map<int, const char*> _ServerState_VALUES_TO_NAMES;

std::ostream& operator<<(std::ostream& out, const ServerState::type& val);

std::string to_string(const ServerState::type& val);

typedef int32_t TermId;

class RaftAddr;

class RequestVoteParams;

class RequestVoteResult;

class LogEntry;

class AppendEntriesParams;

class AppendEntriesResult;

class RaftState;

class StartResult;

typedef struct _RaftAddr__isset {
  _RaftAddr__isset() : ip(false), port(false) {}
  bool ip :1;
  bool port :1;
} _RaftAddr__isset;

class RaftAddr : public virtual ::apache::thrift::TBase {
 public:

  RaftAddr(const RaftAddr&);
  RaftAddr& operator=(const RaftAddr&);
  RaftAddr() noexcept
           : ip(),
             port(0) {
  }

  virtual ~RaftAddr() noexcept;
  std::string ip;
  int16_t port;

  _RaftAddr__isset __isset;

  void __set_ip(const std::string& val);

  void __set_port(const int16_t val);

  bool operator == (const RaftAddr & rhs) const
  {
    if (!(ip == rhs.ip))
      return false;
    if (!(port == rhs.port))
      return false;
    return true;
  }
  bool operator != (const RaftAddr &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const RaftAddr & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot) override;
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const override;

  virtual void printTo(std::ostream& out) const;
};

void swap(RaftAddr &a, RaftAddr &b);

std::ostream& operator<<(std::ostream& out, const RaftAddr& obj);

typedef struct _RequestVoteParams__isset {
  _RequestVoteParams__isset() : term(false), candidateId(false), lastLogIndex(false), LastLogTerm(false) {}
  bool term :1;
  bool candidateId :1;
  bool lastLogIndex :1;
  bool LastLogTerm :1;
} _RequestVoteParams__isset;

class RequestVoteParams : public virtual ::apache::thrift::TBase {
 public:

  RequestVoteParams(const RequestVoteParams&);
  RequestVoteParams& operator=(const RequestVoteParams&);
  RequestVoteParams() noexcept
                    : term(0),
                      lastLogIndex(0),
                      LastLogTerm(0) {
  }

  virtual ~RequestVoteParams() noexcept;
  TermId term;
  RaftAddr candidateId;
  int32_t lastLogIndex;
  TermId LastLogTerm;

  _RequestVoteParams__isset __isset;

  void __set_term(const TermId val);

  void __set_candidateId(const RaftAddr& val);

  void __set_lastLogIndex(const int32_t val);

  void __set_LastLogTerm(const TermId val);

  bool operator == (const RequestVoteParams & rhs) const
  {
    if (!(term == rhs.term))
      return false;
    if (!(candidateId == rhs.candidateId))
      return false;
    if (!(lastLogIndex == rhs.lastLogIndex))
      return false;
    if (!(LastLogTerm == rhs.LastLogTerm))
      return false;
    return true;
  }
  bool operator != (const RequestVoteParams &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const RequestVoteParams & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot) override;
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const override;

  virtual void printTo(std::ostream& out) const;
};

void swap(RequestVoteParams &a, RequestVoteParams &b);

std::ostream& operator<<(std::ostream& out, const RequestVoteParams& obj);

typedef struct _RequestVoteResult__isset {
  _RequestVoteResult__isset() : term(false), voteGranted(false) {}
  bool term :1;
  bool voteGranted :1;
} _RequestVoteResult__isset;

class RequestVoteResult : public virtual ::apache::thrift::TBase {
 public:

  RequestVoteResult(const RequestVoteResult&) noexcept;
  RequestVoteResult& operator=(const RequestVoteResult&) noexcept;
  RequestVoteResult() noexcept
                    : term(0),
                      voteGranted(0) {
  }

  virtual ~RequestVoteResult() noexcept;
  TermId term;
  bool voteGranted;

  _RequestVoteResult__isset __isset;

  void __set_term(const TermId val);

  void __set_voteGranted(const bool val);

  bool operator == (const RequestVoteResult & rhs) const
  {
    if (!(term == rhs.term))
      return false;
    if (!(voteGranted == rhs.voteGranted))
      return false;
    return true;
  }
  bool operator != (const RequestVoteResult &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const RequestVoteResult & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot) override;
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const override;

  virtual void printTo(std::ostream& out) const;
};

void swap(RequestVoteResult &a, RequestVoteResult &b);

std::ostream& operator<<(std::ostream& out, const RequestVoteResult& obj);

typedef struct _LogEntry__isset {
  _LogEntry__isset() : term(false), command(false), index(false) {}
  bool term :1;
  bool command :1;
  bool index :1;
} _LogEntry__isset;

class LogEntry : public virtual ::apache::thrift::TBase {
 public:

  LogEntry(const LogEntry&);
  LogEntry& operator=(const LogEntry&);
  LogEntry() noexcept
           : term(0),
             command(),
             index(0) {
  }

  virtual ~LogEntry() noexcept;
  TermId term;
  std::string command;
  int32_t index;

  _LogEntry__isset __isset;

  void __set_term(const TermId val);

  void __set_command(const std::string& val);

  void __set_index(const int32_t val);

  bool operator == (const LogEntry & rhs) const
  {
    if (!(term == rhs.term))
      return false;
    if (!(command == rhs.command))
      return false;
    if (!(index == rhs.index))
      return false;
    return true;
  }
  bool operator != (const LogEntry &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const LogEntry & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot) override;
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const override;

  virtual void printTo(std::ostream& out) const;
};

void swap(LogEntry &a, LogEntry &b);

std::ostream& operator<<(std::ostream& out, const LogEntry& obj);

typedef struct _AppendEntriesParams__isset {
  _AppendEntriesParams__isset() : term(false), leaderId(false), prevLogIndex(false), prevLogTerm(false), entries(false), leaderCommit(false) {}
  bool term :1;
  bool leaderId :1;
  bool prevLogIndex :1;
  bool prevLogTerm :1;
  bool entries :1;
  bool leaderCommit :1;
} _AppendEntriesParams__isset;

class AppendEntriesParams : public virtual ::apache::thrift::TBase {
 public:

  AppendEntriesParams(const AppendEntriesParams&);
  AppendEntriesParams& operator=(const AppendEntriesParams&);
  AppendEntriesParams() noexcept
                      : term(0),
                        prevLogIndex(0),
                        prevLogTerm(0),
                        leaderCommit(0) {
  }

  virtual ~AppendEntriesParams() noexcept;
  TermId term;
  RaftAddr leaderId;
  int32_t prevLogIndex;
  TermId prevLogTerm;
  std::vector<LogEntry>  entries;
  int32_t leaderCommit;

  _AppendEntriesParams__isset __isset;

  void __set_term(const TermId val);

  void __set_leaderId(const RaftAddr& val);

  void __set_prevLogIndex(const int32_t val);

  void __set_prevLogTerm(const TermId val);

  void __set_entries(const std::vector<LogEntry> & val);

  void __set_leaderCommit(const int32_t val);

  bool operator == (const AppendEntriesParams & rhs) const
  {
    if (!(term == rhs.term))
      return false;
    if (!(leaderId == rhs.leaderId))
      return false;
    if (!(prevLogIndex == rhs.prevLogIndex))
      return false;
    if (!(prevLogTerm == rhs.prevLogTerm))
      return false;
    if (!(entries == rhs.entries))
      return false;
    if (!(leaderCommit == rhs.leaderCommit))
      return false;
    return true;
  }
  bool operator != (const AppendEntriesParams &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const AppendEntriesParams & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot) override;
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const override;

  virtual void printTo(std::ostream& out) const;
};

void swap(AppendEntriesParams &a, AppendEntriesParams &b);

std::ostream& operator<<(std::ostream& out, const AppendEntriesParams& obj);

typedef struct _AppendEntriesResult__isset {
  _AppendEntriesResult__isset() : term(false), success(false) {}
  bool term :1;
  bool success :1;
} _AppendEntriesResult__isset;

class AppendEntriesResult : public virtual ::apache::thrift::TBase {
 public:

  AppendEntriesResult(const AppendEntriesResult&) noexcept;
  AppendEntriesResult& operator=(const AppendEntriesResult&) noexcept;
  AppendEntriesResult() noexcept
                      : term(0),
                        success(0) {
  }

  virtual ~AppendEntriesResult() noexcept;
  TermId term;
  bool success;

  _AppendEntriesResult__isset __isset;

  void __set_term(const TermId val);

  void __set_success(const bool val);

  bool operator == (const AppendEntriesResult & rhs) const
  {
    if (!(term == rhs.term))
      return false;
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const AppendEntriesResult &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const AppendEntriesResult & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot) override;
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const override;

  virtual void printTo(std::ostream& out) const;
};

void swap(AppendEntriesResult &a, AppendEntriesResult &b);

std::ostream& operator<<(std::ostream& out, const AppendEntriesResult& obj);

typedef struct _RaftState__isset {
  _RaftState__isset() : currentTerm(false), votedFor(false), commitIndex(false), lastApplied(false), state(false), peers(false), logs(false) {}
  bool currentTerm :1;
  bool votedFor :1;
  bool commitIndex :1;
  bool lastApplied :1;
  bool state :1;
  bool peers :1;
  bool logs :1;
} _RaftState__isset;

class RaftState : public virtual ::apache::thrift::TBase {
 public:

  RaftState(const RaftState&);
  RaftState& operator=(const RaftState&);
  RaftState() noexcept
            : currentTerm(0),
              commitIndex(0),
              lastApplied(0),
              state(static_cast<ServerState::type>(0)) {
  }

  virtual ~RaftState() noexcept;
  TermId currentTerm;
  RaftAddr votedFor;
  int32_t commitIndex;
  int32_t lastApplied;
  /**
   * 
   * @see ServerState
   */
  ServerState::type state;
  std::vector<RaftAddr>  peers;
  std::vector<LogEntry>  logs;

  _RaftState__isset __isset;

  void __set_currentTerm(const TermId val);

  void __set_votedFor(const RaftAddr& val);

  void __set_commitIndex(const int32_t val);

  void __set_lastApplied(const int32_t val);

  void __set_state(const ServerState::type val);

  void __set_peers(const std::vector<RaftAddr> & val);

  void __set_logs(const std::vector<LogEntry> & val);

  bool operator == (const RaftState & rhs) const
  {
    if (!(currentTerm == rhs.currentTerm))
      return false;
    if (!(votedFor == rhs.votedFor))
      return false;
    if (!(commitIndex == rhs.commitIndex))
      return false;
    if (!(lastApplied == rhs.lastApplied))
      return false;
    if (!(state == rhs.state))
      return false;
    if (!(peers == rhs.peers))
      return false;
    if (!(logs == rhs.logs))
      return false;
    return true;
  }
  bool operator != (const RaftState &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const RaftState & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot) override;
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const override;

  virtual void printTo(std::ostream& out) const;
};

void swap(RaftState &a, RaftState &b);

std::ostream& operator<<(std::ostream& out, const RaftState& obj);

typedef struct _StartResult__isset {
  _StartResult__isset() : expectedLogIndex(false), term(false), isLeader(false) {}
  bool expectedLogIndex :1;
  bool term :1;
  bool isLeader :1;
} _StartResult__isset;

class StartResult : public virtual ::apache::thrift::TBase {
 public:

  StartResult(const StartResult&) noexcept;
  StartResult& operator=(const StartResult&) noexcept;
  StartResult() noexcept
              : expectedLogIndex(0),
                term(0),
                isLeader(0) {
  }

  virtual ~StartResult() noexcept;
  int32_t expectedLogIndex;
  TermId term;
  bool isLeader;

  _StartResult__isset __isset;

  void __set_expectedLogIndex(const int32_t val);

  void __set_term(const TermId val);

  void __set_isLeader(const bool val);

  bool operator == (const StartResult & rhs) const
  {
    if (!(expectedLogIndex == rhs.expectedLogIndex))
      return false;
    if (!(term == rhs.term))
      return false;
    if (!(isLeader == rhs.isLeader))
      return false;
    return true;
  }
  bool operator != (const StartResult &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const StartResult & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot) override;
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const override;

  virtual void printTo(std::ostream& out) const;
};

void swap(StartResult &a, StartResult &b);

std::ostream& operator<<(std::ostream& out, const StartResult& obj);



#endif
