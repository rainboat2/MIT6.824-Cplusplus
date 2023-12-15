/**
 * Autogenerated by Thrift Compiler (0.18.1)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef KVRaft_H
#define KVRaft_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include <memory>
#include "KVRaft_types.h"
#include "Raft.h"



#ifdef _MSC_VER
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class KVRaftIf : virtual public RaftIf {
 public:
  virtual ~KVRaftIf() {}
  virtual void putAppend(PutAppendReply& _return, const PutAppendParams& params) = 0;
  virtual void get(GetReply& _return, const GetParams& params) = 0;
};

class KVRaftIfFactory : virtual public RaftIfFactory {
 public:
  typedef KVRaftIf Handler;

  virtual ~KVRaftIfFactory() {}

  virtual KVRaftIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) override = 0;
  virtual void releaseHandler(RaftIf* /* handler */) override = 0;
  };

class KVRaftIfSingletonFactory : virtual public KVRaftIfFactory {
 public:
  KVRaftIfSingletonFactory(const ::std::shared_ptr<KVRaftIf>& iface) : iface_(iface) {}
  virtual ~KVRaftIfSingletonFactory() {}

  virtual KVRaftIf* getHandler(const ::apache::thrift::TConnectionInfo&) override {
    return iface_.get();
  }
  virtual void releaseHandler(RaftIf* /* handler */) override {}

 protected:
  ::std::shared_ptr<KVRaftIf> iface_;
};

class KVRaftNull : virtual public KVRaftIf , virtual public RaftNull {
 public:
  virtual ~KVRaftNull() {}
  void putAppend(PutAppendReply& /* _return */, const PutAppendParams& /* params */) override {
    return;
  }
  void get(GetReply& /* _return */, const GetParams& /* params */) override {
    return;
  }
};

typedef struct _KVRaft_putAppend_args__isset {
  _KVRaft_putAppend_args__isset() : params(false) {}
  bool params :1;
} _KVRaft_putAppend_args__isset;

class KVRaft_putAppend_args {
 public:

  KVRaft_putAppend_args(const KVRaft_putAppend_args&);
  KVRaft_putAppend_args& operator=(const KVRaft_putAppend_args&);
  KVRaft_putAppend_args() noexcept {
  }

  virtual ~KVRaft_putAppend_args() noexcept;
  PutAppendParams params;

  _KVRaft_putAppend_args__isset __isset;

  void __set_params(const PutAppendParams& val);

  bool operator == (const KVRaft_putAppend_args & rhs) const
  {
    if (!(params == rhs.params))
      return false;
    return true;
  }
  bool operator != (const KVRaft_putAppend_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const KVRaft_putAppend_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class KVRaft_putAppend_pargs {
 public:


  virtual ~KVRaft_putAppend_pargs() noexcept;
  const PutAppendParams* params;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _KVRaft_putAppend_result__isset {
  _KVRaft_putAppend_result__isset() : success(false) {}
  bool success :1;
} _KVRaft_putAppend_result__isset;

class KVRaft_putAppend_result {
 public:

  KVRaft_putAppend_result(const KVRaft_putAppend_result&) noexcept;
  KVRaft_putAppend_result& operator=(const KVRaft_putAppend_result&) noexcept;
  KVRaft_putAppend_result() noexcept {
  }

  virtual ~KVRaft_putAppend_result() noexcept;
  PutAppendReply success;

  _KVRaft_putAppend_result__isset __isset;

  void __set_success(const PutAppendReply& val);

  bool operator == (const KVRaft_putAppend_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const KVRaft_putAppend_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const KVRaft_putAppend_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _KVRaft_putAppend_presult__isset {
  _KVRaft_putAppend_presult__isset() : success(false) {}
  bool success :1;
} _KVRaft_putAppend_presult__isset;

class KVRaft_putAppend_presult {
 public:


  virtual ~KVRaft_putAppend_presult() noexcept;
  PutAppendReply* success;

  _KVRaft_putAppend_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _KVRaft_get_args__isset {
  _KVRaft_get_args__isset() : params(false) {}
  bool params :1;
} _KVRaft_get_args__isset;

class KVRaft_get_args {
 public:

  KVRaft_get_args(const KVRaft_get_args&);
  KVRaft_get_args& operator=(const KVRaft_get_args&);
  KVRaft_get_args() noexcept {
  }

  virtual ~KVRaft_get_args() noexcept;
  GetParams params;

  _KVRaft_get_args__isset __isset;

  void __set_params(const GetParams& val);

  bool operator == (const KVRaft_get_args & rhs) const
  {
    if (!(params == rhs.params))
      return false;
    return true;
  }
  bool operator != (const KVRaft_get_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const KVRaft_get_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class KVRaft_get_pargs {
 public:


  virtual ~KVRaft_get_pargs() noexcept;
  const GetParams* params;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _KVRaft_get_result__isset {
  _KVRaft_get_result__isset() : success(false) {}
  bool success :1;
} _KVRaft_get_result__isset;

class KVRaft_get_result {
 public:

  KVRaft_get_result(const KVRaft_get_result&);
  KVRaft_get_result& operator=(const KVRaft_get_result&);
  KVRaft_get_result() noexcept {
  }

  virtual ~KVRaft_get_result() noexcept;
  GetReply success;

  _KVRaft_get_result__isset __isset;

  void __set_success(const GetReply& val);

  bool operator == (const KVRaft_get_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const KVRaft_get_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const KVRaft_get_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _KVRaft_get_presult__isset {
  _KVRaft_get_presult__isset() : success(false) {}
  bool success :1;
} _KVRaft_get_presult__isset;

class KVRaft_get_presult {
 public:


  virtual ~KVRaft_get_presult() noexcept;
  GetReply* success;

  _KVRaft_get_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class KVRaftClient : virtual public KVRaftIf, public RaftClient {
 public:
  KVRaftClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) :
    RaftClient(prot, prot) {}
  KVRaftClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) :    RaftClient(iprot, oprot) {}
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void putAppend(PutAppendReply& _return, const PutAppendParams& params) override;
  void send_putAppend(const PutAppendParams& params);
  void recv_putAppend(PutAppendReply& _return);
  void get(GetReply& _return, const GetParams& params) override;
  void send_get(const GetParams& params);
  void recv_get(GetReply& _return);
};

class KVRaftProcessor : public RaftProcessor {
 protected:
  ::std::shared_ptr<KVRaftIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext) override;
 private:
  typedef  void (KVRaftProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_putAppend(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_get(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  KVRaftProcessor(::std::shared_ptr<KVRaftIf> iface) :
    RaftProcessor(iface),
    iface_(iface) {
    processMap_["putAppend"] = &KVRaftProcessor::process_putAppend;
    processMap_["get"] = &KVRaftProcessor::process_get;
  }

  virtual ~KVRaftProcessor() {}
};

class KVRaftProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  KVRaftProcessorFactory(const ::std::shared_ptr< KVRaftIfFactory >& handlerFactory) noexcept :
      handlerFactory_(handlerFactory) {}

  ::std::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo) override;

 protected:
  ::std::shared_ptr< KVRaftIfFactory > handlerFactory_;
};

class KVRaftMultiface : virtual public KVRaftIf, public RaftMultiface {
 public:
  KVRaftMultiface(std::vector<std::shared_ptr<KVRaftIf> >& ifaces) : ifaces_(ifaces) {
    std::vector<std::shared_ptr<KVRaftIf> >::iterator iter;
    for (iter = ifaces.begin(); iter != ifaces.end(); ++iter) {
      RaftMultiface::add(*iter);
    }
  }
  virtual ~KVRaftMultiface() {}
 protected:
  std::vector<std::shared_ptr<KVRaftIf> > ifaces_;
  KVRaftMultiface() {}
  void add(::std::shared_ptr<KVRaftIf> iface) {
    RaftMultiface::add(iface);
    ifaces_.push_back(iface);
  }
 public:
  void putAppend(PutAppendReply& _return, const PutAppendParams& params) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->putAppend(_return, params);
    }
    ifaces_[i]->putAppend(_return, params);
    return;
  }

  void get(GetReply& _return, const GetParams& params) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->get(_return, params);
    }
    ifaces_[i]->get(_return, params);
    return;
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class KVRaftConcurrentClient : virtual public KVRaftIf, public RaftConcurrentClient {
 public:
  KVRaftConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot, std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync) :
    RaftConcurrentClient(prot, prot, sync) {}
  KVRaftConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot, std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync) :    RaftConcurrentClient(iprot, oprot, sync) {}
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void putAppend(PutAppendReply& _return, const PutAppendParams& params) override;
  int32_t send_putAppend(const PutAppendParams& params);
  void recv_putAppend(PutAppendReply& _return, const int32_t seqid);
  void get(GetReply& _return, const GetParams& params) override;
  int32_t send_get(const GetParams& params);
  void recv_get(GetReply& _return, const int32_t seqid);
};

#ifdef _MSC_VER
  #pragma warning( pop )
#endif



#endif
