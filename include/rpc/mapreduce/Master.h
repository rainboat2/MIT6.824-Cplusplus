/**
 * Autogenerated by Thrift Compiler (0.20.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef Master_H
#define Master_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include <memory>
#include "MapReduce_types.h"



#ifdef _MSC_VER
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class MasterIf {
 public:
  virtual ~MasterIf() {}
  virtual int32_t assignId() = 0;
  virtual void assignTask(TaskResponse& _return) = 0;
  virtual void commitTask(const TaskResult& result) = 0;
};

class MasterIfFactory {
 public:
  typedef MasterIf Handler;

  virtual ~MasterIfFactory() {}

  virtual MasterIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(MasterIf* /* handler */) = 0;
  };

class MasterIfSingletonFactory : virtual public MasterIfFactory {
 public:
  MasterIfSingletonFactory(const ::std::shared_ptr<MasterIf>& iface) : iface_(iface) {}
  virtual ~MasterIfSingletonFactory() {}

  virtual MasterIf* getHandler(const ::apache::thrift::TConnectionInfo&) override {
    return iface_.get();
  }
  virtual void releaseHandler(MasterIf* /* handler */) override {}

 protected:
  ::std::shared_ptr<MasterIf> iface_;
};

class MasterNull : virtual public MasterIf {
 public:
  virtual ~MasterNull() {}
  int32_t assignId() override {
    int32_t _return = 0;
    return _return;
  }
  void assignTask(TaskResponse& /* _return */) override {
    return;
  }
  void commitTask(const TaskResult& /* result */) override {
    return;
  }
};


class Master_assignId_args {
 public:

  Master_assignId_args(const Master_assignId_args&) noexcept;
  Master_assignId_args& operator=(const Master_assignId_args&) noexcept;
  Master_assignId_args() noexcept {
  }

  virtual ~Master_assignId_args() noexcept;

  bool operator == (const Master_assignId_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const Master_assignId_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Master_assignId_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Master_assignId_pargs {
 public:


  virtual ~Master_assignId_pargs() noexcept;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Master_assignId_result__isset {
  _Master_assignId_result__isset() : success(false) {}
  bool success :1;
} _Master_assignId_result__isset;

class Master_assignId_result {
 public:

  Master_assignId_result(const Master_assignId_result&) noexcept;
  Master_assignId_result& operator=(const Master_assignId_result&) noexcept;
  Master_assignId_result() noexcept
                         : success(0) {
  }

  virtual ~Master_assignId_result() noexcept;
  int32_t success;

  _Master_assignId_result__isset __isset;

  void __set_success(const int32_t val);

  bool operator == (const Master_assignId_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Master_assignId_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Master_assignId_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Master_assignId_presult__isset {
  _Master_assignId_presult__isset() : success(false) {}
  bool success :1;
} _Master_assignId_presult__isset;

class Master_assignId_presult {
 public:


  virtual ~Master_assignId_presult() noexcept;
  int32_t* success;

  _Master_assignId_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};


class Master_assignTask_args {
 public:

  Master_assignTask_args(const Master_assignTask_args&) noexcept;
  Master_assignTask_args& operator=(const Master_assignTask_args&) noexcept;
  Master_assignTask_args() noexcept {
  }

  virtual ~Master_assignTask_args() noexcept;

  bool operator == (const Master_assignTask_args & /* rhs */) const
  {
    return true;
  }
  bool operator != (const Master_assignTask_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Master_assignTask_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Master_assignTask_pargs {
 public:


  virtual ~Master_assignTask_pargs() noexcept;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Master_assignTask_result__isset {
  _Master_assignTask_result__isset() : success(false) {}
  bool success :1;
} _Master_assignTask_result__isset;

class Master_assignTask_result {
 public:

  Master_assignTask_result(const Master_assignTask_result&);
  Master_assignTask_result& operator=(const Master_assignTask_result&);
  Master_assignTask_result() noexcept {
  }

  virtual ~Master_assignTask_result() noexcept;
  TaskResponse success;

  _Master_assignTask_result__isset __isset;

  void __set_success(const TaskResponse& val);

  bool operator == (const Master_assignTask_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Master_assignTask_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Master_assignTask_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Master_assignTask_presult__isset {
  _Master_assignTask_presult__isset() : success(false) {}
  bool success :1;
} _Master_assignTask_presult__isset;

class Master_assignTask_presult {
 public:


  virtual ~Master_assignTask_presult() noexcept;
  TaskResponse* success;

  _Master_assignTask_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _Master_commitTask_args__isset {
  _Master_commitTask_args__isset() : result(false) {}
  bool result :1;
} _Master_commitTask_args__isset;

class Master_commitTask_args {
 public:

  Master_commitTask_args(const Master_commitTask_args&);
  Master_commitTask_args& operator=(const Master_commitTask_args&);
  Master_commitTask_args() noexcept {
  }

  virtual ~Master_commitTask_args() noexcept;
  TaskResult result;

  _Master_commitTask_args__isset __isset;

  void __set_result(const TaskResult& val);

  bool operator == (const Master_commitTask_args & rhs) const
  {
    if (!(result == rhs.result))
      return false;
    return true;
  }
  bool operator != (const Master_commitTask_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Master_commitTask_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Master_commitTask_pargs {
 public:


  virtual ~Master_commitTask_pargs() noexcept;
  const TaskResult* result;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Master_commitTask_result {
 public:

  Master_commitTask_result(const Master_commitTask_result&) noexcept;
  Master_commitTask_result& operator=(const Master_commitTask_result&) noexcept;
  Master_commitTask_result() noexcept {
  }

  virtual ~Master_commitTask_result() noexcept;

  bool operator == (const Master_commitTask_result & /* rhs */) const
  {
    return true;
  }
  bool operator != (const Master_commitTask_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Master_commitTask_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Master_commitTask_presult {
 public:


  virtual ~Master_commitTask_presult() noexcept;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class MasterClient : virtual public MasterIf {
 public:
  MasterClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  MasterClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
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
  int32_t assignId() override;
  void send_assignId();
  int32_t recv_assignId();
  void assignTask(TaskResponse& _return) override;
  void send_assignTask();
  void recv_assignTask(TaskResponse& _return);
  void commitTask(const TaskResult& result) override;
  void send_commitTask(const TaskResult& result);
  void recv_commitTask();
 protected:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class MasterProcessor : public ::apache::thrift::TDispatchProcessor {
 protected:
  ::std::shared_ptr<MasterIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext) override;
 private:
  typedef  void (MasterProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_assignId(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_assignTask(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_commitTask(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  MasterProcessor(::std::shared_ptr<MasterIf> iface) :
    iface_(iface) {
    processMap_["assignId"] = &MasterProcessor::process_assignId;
    processMap_["assignTask"] = &MasterProcessor::process_assignTask;
    processMap_["commitTask"] = &MasterProcessor::process_commitTask;
  }

  virtual ~MasterProcessor() {}
};

class MasterProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  MasterProcessorFactory(const ::std::shared_ptr< MasterIfFactory >& handlerFactory) noexcept :
      handlerFactory_(handlerFactory) {}

  ::std::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo) override;

 protected:
  ::std::shared_ptr< MasterIfFactory > handlerFactory_;
};

class MasterMultiface : virtual public MasterIf {
 public:
  MasterMultiface(std::vector<std::shared_ptr<MasterIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~MasterMultiface() {}
 protected:
  std::vector<std::shared_ptr<MasterIf> > ifaces_;
  MasterMultiface() {}
  void add(::std::shared_ptr<MasterIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  int32_t assignId() override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->assignId();
    }
    return ifaces_[i]->assignId();
  }

  void assignTask(TaskResponse& _return) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->assignTask(_return);
    }
    ifaces_[i]->assignTask(_return);
    return;
  }

  void commitTask(const TaskResult& result) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->commitTask(result);
    }
    ifaces_[i]->commitTask(result);
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class MasterConcurrentClient : virtual public MasterIf {
 public:
  MasterConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot, std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync) : sync_(sync)
{
    setProtocol(prot);
  }
  MasterConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot, std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync) : sync_(sync)
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
  int32_t assignId() override;
  int32_t send_assignId();
  int32_t recv_assignId(const int32_t seqid);
  void assignTask(TaskResponse& _return) override;
  int32_t send_assignTask();
  void recv_assignTask(TaskResponse& _return, const int32_t seqid);
  void commitTask(const TaskResult& result) override;
  int32_t send_commitTask(const TaskResult& result);
  void recv_commitTask(const int32_t seqid);
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
