/**
 * Autogenerated by Thrift Compiler (0.20.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef Raft_H
#define Raft_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include <memory>
#include "KVRaft_types.h"



#ifdef _MSC_VER
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class RaftIf {
 public:
  virtual ~RaftIf() {}
  virtual void requestVote(RequestVoteResult& _return, const RequestVoteParams& params) = 0;
  virtual void appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params) = 0;
  virtual void getState(RaftState& _return) = 0;
  virtual void start(StartResult& _return, const std::string& command) = 0;
  virtual TermId installSnapshot(const InstallSnapshotParams& params) = 0;
};

class RaftIfFactory {
 public:
  typedef RaftIf Handler;

  virtual ~RaftIfFactory() {}

  virtual RaftIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(RaftIf* /* handler */) = 0;
  };

class RaftIfSingletonFactory : virtual public RaftIfFactory {
 public:
  RaftIfSingletonFactory(const ::std::shared_ptr<RaftIf>& iface) : iface_(iface) {}
  virtual ~RaftIfSingletonFactory() {}

  virtual RaftIf* getHandler(const ::apache::thrift::TConnectionInfo&) override {
    return iface_.get();
  }
  virtual void releaseHandler(RaftIf* /* handler */) override {}

 protected:
  ::std::shared_ptr<RaftIf> iface_;
};

class RaftNull : virtual public RaftIf {
 public:
  virtual ~RaftNull() {}
  void requestVote(RequestVoteResult& /* _return */, const RequestVoteParams& /* params */) override {
    return;
  }
  void appendEntries(AppendEntriesResult& /* _return */, const AppendEntriesParams& /* params */) override {
    return;
  }
  void getState(RaftState& /* _return */) override {
    return;
  }
  void start(StartResult& /* _return */, const std::string& /* command */) override {
    return;
  }
  TermId installSnapshot(const InstallSnapshotParams& /* params */) override {
    TermId _return = 0;
    return _return;
  }
};

typedef struct _Raft_requestVote_args__isset {
  _Raft_requestVote_args__isset() : params(false) {}
  bool params :1;
} _Raft_requestVote_args__isset;

class Raft_requestVote_args {
 public:

  Raft_requestVote_args(const Raft_requestVote_args&);
  Raft_requestVote_args& operator=(const Raft_requestVote_args&);
  Raft_requestVote_args() noexcept {
  }

  virtual ~Raft_requestVote_args() noexcept;
  RequestVoteParams params;

  _Raft_requestVote_args__isset __isset;

  void __set_params(const RequestVoteParams& val);

  bool operator == (const Raft_requestVote_args & rhs) const
  {
    if (!(params == rhs.params))
      return false;
    return true;
  }
  bool operator != (const Raft_requestVote_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_requestVote_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Raft_requestVote_pargs {
 public:


  virtual ~Raft_requestVote_pargs() noexcept;
  const RequestVoteParams* params;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_requestVote_result__isset {
  _Raft_requestVote_result__isset() : success(false) {}
  bool success :1;
} _Raft_requestVote_result__isset;

class Raft_requestVote_result {
 public:

  Raft_requestVote_result(const Raft_requestVote_result&) noexcept;
  Raft_requestVote_result& operator=(const Raft_requestVote_result&) noexcept;
  Raft_requestVote_result() noexcept {
  }

  virtual ~Raft_requestVote_result() noexcept;
  RequestVoteResult success;

  _Raft_requestVote_result__isset __isset;

  void __set_success(const RequestVoteResult& val);

  bool operator == (const Raft_requestVote_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Raft_requestVote_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_requestVote_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_requestVote_presult__isset {
  _Raft_requestVote_presult__isset() : success(false) {}
  bool success :1;
} _Raft_requestVote_presult__isset;

class Raft_requestVote_presult {
 public:


  virtual ~Raft_requestVote_presult() noexcept;
  RequestVoteResult* success;

  _Raft_requestVote_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _Raft_appendEntries_args__isset {
  _Raft_appendEntries_args__isset() : params(false) {}
  bool params :1;
} _Raft_appendEntries_args__isset;

class Raft_appendEntries_args {
 public:

  Raft_appendEntries_args(const Raft_appendEntries_args&);
  Raft_appendEntries_args& operator=(const Raft_appendEntries_args&);
  Raft_appendEntries_args() noexcept {
  }

  virtual ~Raft_appendEntries_args() noexcept;
  AppendEntriesParams params;

  _Raft_appendEntries_args__isset __isset;

  void __set_params(const AppendEntriesParams& val);

  bool operator == (const Raft_appendEntries_args & rhs) const
  {
    if (!(params == rhs.params))
      return false;
    return true;
  }
  bool operator != (const Raft_appendEntries_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_appendEntries_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Raft_appendEntries_pargs {
 public:


  virtual ~Raft_appendEntries_pargs() noexcept;
  const AppendEntriesParams* params;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_appendEntries_result__isset {
  _Raft_appendEntries_result__isset() : success(false) {}
  bool success :1;
} _Raft_appendEntries_result__isset;

class Raft_appendEntries_result {
 public:

  Raft_appendEntries_result(const Raft_appendEntries_result&) noexcept;
  Raft_appendEntries_result& operator=(const Raft_appendEntries_result&) noexcept;
  Raft_appendEntries_result() noexcept {
  }

  virtual ~Raft_appendEntries_result() noexcept;
  AppendEntriesResult success;

  _Raft_appendEntries_result__isset __isset;

  void __set_success(const AppendEntriesResult& val);

  bool operator == (const Raft_appendEntries_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Raft_appendEntries_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_appendEntries_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_appendEntries_presult__isset {
  _Raft_appendEntries_presult__isset() : success(false) {}
  bool success :1;
} _Raft_appendEntries_presult__isset;

class Raft_appendEntries_presult {
 public:


  virtual ~Raft_appendEntries_presult() noexcept;
  AppendEntriesResult* success;

  _Raft_appendEntries_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};


class Raft_getState_args {
 public:

  Raft_getState_args(const Raft_getState_args&) noexcept;
  Raft_getState_args& operator=(const Raft_getState_args&) noexcept;
  Raft_getState_args() noexcept {
  }

  virtual ~Raft_getState_args() noexcept;

  bool operator == (const Raft_getState_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const Raft_getState_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_getState_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Raft_getState_pargs {
 public:


  virtual ~Raft_getState_pargs() noexcept;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_getState_result__isset {
  _Raft_getState_result__isset() : success(false) {}
  bool success :1;
} _Raft_getState_result__isset;

class Raft_getState_result {
 public:

  Raft_getState_result(const Raft_getState_result&);
  Raft_getState_result& operator=(const Raft_getState_result&);
  Raft_getState_result() noexcept {
  }

  virtual ~Raft_getState_result() noexcept;
  RaftState success;

  _Raft_getState_result__isset __isset;

  void __set_success(const RaftState& val);

  bool operator == (const Raft_getState_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Raft_getState_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_getState_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_getState_presult__isset {
  _Raft_getState_presult__isset() : success(false) {}
  bool success :1;
} _Raft_getState_presult__isset;

class Raft_getState_presult {
 public:


  virtual ~Raft_getState_presult() noexcept;
  RaftState* success;

  _Raft_getState_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _Raft_start_args__isset {
  _Raft_start_args__isset() : command(false) {}
  bool command :1;
} _Raft_start_args__isset;

class Raft_start_args {
 public:

  Raft_start_args(const Raft_start_args&);
  Raft_start_args& operator=(const Raft_start_args&);
  Raft_start_args() noexcept
                  : command() {
  }

  virtual ~Raft_start_args() noexcept;
  std::string command;

  _Raft_start_args__isset __isset;

  void __set_command(const std::string& val);

  bool operator == (const Raft_start_args & rhs) const
  {
    if (!(command == rhs.command))
      return false;
    return true;
  }
  bool operator != (const Raft_start_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_start_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Raft_start_pargs {
 public:


  virtual ~Raft_start_pargs() noexcept;
  const std::string* command;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_start_result__isset {
  _Raft_start_result__isset() : success(false) {}
  bool success :1;
} _Raft_start_result__isset;

class Raft_start_result {
 public:

  Raft_start_result(const Raft_start_result&) noexcept;
  Raft_start_result& operator=(const Raft_start_result&) noexcept;
  Raft_start_result() noexcept {
  }

  virtual ~Raft_start_result() noexcept;
  StartResult success;

  _Raft_start_result__isset __isset;

  void __set_success(const StartResult& val);

  bool operator == (const Raft_start_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Raft_start_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_start_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_start_presult__isset {
  _Raft_start_presult__isset() : success(false) {}
  bool success :1;
} _Raft_start_presult__isset;

class Raft_start_presult {
 public:


  virtual ~Raft_start_presult() noexcept;
  StartResult* success;

  _Raft_start_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _Raft_installSnapshot_args__isset {
  _Raft_installSnapshot_args__isset() : params(false) {}
  bool params :1;
} _Raft_installSnapshot_args__isset;

class Raft_installSnapshot_args {
 public:

  Raft_installSnapshot_args(const Raft_installSnapshot_args&);
  Raft_installSnapshot_args& operator=(const Raft_installSnapshot_args&);
  Raft_installSnapshot_args() noexcept {
  }

  virtual ~Raft_installSnapshot_args() noexcept;
  InstallSnapshotParams params;

  _Raft_installSnapshot_args__isset __isset;

  void __set_params(const InstallSnapshotParams& val);

  bool operator == (const Raft_installSnapshot_args & rhs) const
  {
    if (!(params == rhs.params))
      return false;
    return true;
  }
  bool operator != (const Raft_installSnapshot_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_installSnapshot_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Raft_installSnapshot_pargs {
 public:


  virtual ~Raft_installSnapshot_pargs() noexcept;
  const InstallSnapshotParams* params;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_installSnapshot_result__isset {
  _Raft_installSnapshot_result__isset() : success(false) {}
  bool success :1;
} _Raft_installSnapshot_result__isset;

class Raft_installSnapshot_result {
 public:

  Raft_installSnapshot_result(const Raft_installSnapshot_result&) noexcept;
  Raft_installSnapshot_result& operator=(const Raft_installSnapshot_result&) noexcept;
  Raft_installSnapshot_result() noexcept
                              : success(0) {
  }

  virtual ~Raft_installSnapshot_result() noexcept;
  TermId success;

  _Raft_installSnapshot_result__isset __isset;

  void __set_success(const TermId val);

  bool operator == (const Raft_installSnapshot_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Raft_installSnapshot_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Raft_installSnapshot_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Raft_installSnapshot_presult__isset {
  _Raft_installSnapshot_presult__isset() : success(false) {}
  bool success :1;
} _Raft_installSnapshot_presult__isset;

class Raft_installSnapshot_presult {
 public:


  virtual ~Raft_installSnapshot_presult() noexcept;
  TermId* success;

  _Raft_installSnapshot_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class RaftClient : virtual public RaftIf {
 public:
  RaftClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  RaftClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void requestVote(RequestVoteResult& _return, const RequestVoteParams& params) override;
  void send_requestVote(const RequestVoteParams& params);
  void recv_requestVote(RequestVoteResult& _return);
  void appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params) override;
  void send_appendEntries(const AppendEntriesParams& params);
  void recv_appendEntries(AppendEntriesResult& _return);
  void getState(RaftState& _return) override;
  void send_getState();
  void recv_getState(RaftState& _return);
  void start(StartResult& _return, const std::string& command) override;
  void send_start(const std::string& command);
  void recv_start(StartResult& _return);
  TermId installSnapshot(const InstallSnapshotParams& params) override;
  void send_installSnapshot(const InstallSnapshotParams& params);
  TermId recv_installSnapshot();
 protected:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class RaftProcessor : public ::apache::thrift::TDispatchProcessor {
 protected:
  ::std::shared_ptr<RaftIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext) override;
 private:
  typedef  void (RaftProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_requestVote(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_appendEntries(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_getState(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_start(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_installSnapshot(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  RaftProcessor(::std::shared_ptr<RaftIf> iface) :
    iface_(iface) {
    processMap_["requestVote"] = &RaftProcessor::process_requestVote;
    processMap_["appendEntries"] = &RaftProcessor::process_appendEntries;
    processMap_["getState"] = &RaftProcessor::process_getState;
    processMap_["start"] = &RaftProcessor::process_start;
    processMap_["installSnapshot"] = &RaftProcessor::process_installSnapshot;
  }

  virtual ~RaftProcessor() {}
};

class RaftProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  RaftProcessorFactory(const ::std::shared_ptr< RaftIfFactory >& handlerFactory) noexcept :
      handlerFactory_(handlerFactory) {}

  ::std::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo) override;

 protected:
  ::std::shared_ptr< RaftIfFactory > handlerFactory_;
};

class RaftMultiface : virtual public RaftIf {
 public:
  RaftMultiface(std::vector<std::shared_ptr<RaftIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~RaftMultiface() {}
 protected:
  std::vector<std::shared_ptr<RaftIf> > ifaces_;
  RaftMultiface() {}
  void add(::std::shared_ptr<RaftIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  void requestVote(RequestVoteResult& _return, const RequestVoteParams& params) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->requestVote(_return, params);
    }
    ifaces_[i]->requestVote(_return, params);
    return;
  }

  void appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->appendEntries(_return, params);
    }
    ifaces_[i]->appendEntries(_return, params);
    return;
  }

  void getState(RaftState& _return) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->getState(_return);
    }
    ifaces_[i]->getState(_return);
    return;
  }

  void start(StartResult& _return, const std::string& command) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->start(_return, command);
    }
    ifaces_[i]->start(_return, command);
    return;
  }

  TermId installSnapshot(const InstallSnapshotParams& params) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->installSnapshot(params);
    }
    return ifaces_[i]->installSnapshot(params);
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class RaftConcurrentClient : virtual public RaftIf {
 public:
  RaftConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot, std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync) : sync_(sync)
{
    setProtocol(prot);
  }
  RaftConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot, std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync) : sync_(sync)
{
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void requestVote(RequestVoteResult& _return, const RequestVoteParams& params) override;
  int32_t send_requestVote(const RequestVoteParams& params);
  void recv_requestVote(RequestVoteResult& _return, const int32_t seqid);
  void appendEntries(AppendEntriesResult& _return, const AppendEntriesParams& params) override;
  int32_t send_appendEntries(const AppendEntriesParams& params);
  void recv_appendEntries(AppendEntriesResult& _return, const int32_t seqid);
  void getState(RaftState& _return) override;
  int32_t send_getState();
  void recv_getState(RaftState& _return, const int32_t seqid);
  void start(StartResult& _return, const std::string& command) override;
  int32_t send_start(const std::string& command);
  void recv_start(StartResult& _return, const int32_t seqid);
  TermId installSnapshot(const InstallSnapshotParams& params) override;
  int32_t send_installSnapshot(const InstallSnapshotParams& params);
  TermId recv_installSnapshot(const int32_t seqid);
 protected:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
  std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync_;
};

#ifdef _MSC_VER
  #pragma warning( pop )
#endif



#endif
