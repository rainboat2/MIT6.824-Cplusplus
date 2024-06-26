/**
 * Autogenerated by Thrift Compiler (0.20.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef ShardKVRaft_H
#define ShardKVRaft_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include <memory>
#include "KVRaft_types.h"
#include "KVRaft.h"



#ifdef _MSC_VER
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class ShardKVRaftIf : virtual public KVRaftIf {
 public:
  virtual ~ShardKVRaftIf() {}
  virtual void pullShardParams(PullShardReply& _return, const PullShardParams& params) = 0;
};

class ShardKVRaftIfFactory : virtual public KVRaftIfFactory {
 public:
  typedef ShardKVRaftIf Handler;

  virtual ~ShardKVRaftIfFactory() {}

  virtual ShardKVRaftIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) override = 0;
  virtual void releaseHandler(RaftIf* /* handler */) override = 0;
  };

class ShardKVRaftIfSingletonFactory : virtual public ShardKVRaftIfFactory {
 public:
  ShardKVRaftIfSingletonFactory(const ::std::shared_ptr<ShardKVRaftIf>& iface) : iface_(iface) {}
  virtual ~ShardKVRaftIfSingletonFactory() {}

  virtual ShardKVRaftIf* getHandler(const ::apache::thrift::TConnectionInfo&) override {
    return iface_.get();
  }
  virtual void releaseHandler(RaftIf* /* handler */) override {}

 protected:
  ::std::shared_ptr<ShardKVRaftIf> iface_;
};

class ShardKVRaftNull : virtual public ShardKVRaftIf , virtual public KVRaftNull {
 public:
  virtual ~ShardKVRaftNull() {}
  void pullShardParams(PullShardReply& /* _return */, const PullShardParams& /* params */) override {
    return;
  }
};

typedef struct _ShardKVRaft_pullShardParams_args__isset {
  _ShardKVRaft_pullShardParams_args__isset() : params(false) {}
  bool params :1;
} _ShardKVRaft_pullShardParams_args__isset;

class ShardKVRaft_pullShardParams_args {
 public:

  ShardKVRaft_pullShardParams_args(const ShardKVRaft_pullShardParams_args&) noexcept;
  ShardKVRaft_pullShardParams_args& operator=(const ShardKVRaft_pullShardParams_args&) noexcept;
  ShardKVRaft_pullShardParams_args() noexcept {
  }

  virtual ~ShardKVRaft_pullShardParams_args() noexcept;
  PullShardParams params;

  _ShardKVRaft_pullShardParams_args__isset __isset;

  void __set_params(const PullShardParams& val);

  bool operator == (const ShardKVRaft_pullShardParams_args & rhs) const
  {
    if (!(params == rhs.params))
      return false;
    return true;
  }
  bool operator != (const ShardKVRaft_pullShardParams_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ShardKVRaft_pullShardParams_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class ShardKVRaft_pullShardParams_pargs {
 public:


  virtual ~ShardKVRaft_pullShardParams_pargs() noexcept;
  const PullShardParams* params;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _ShardKVRaft_pullShardParams_result__isset {
  _ShardKVRaft_pullShardParams_result__isset() : success(false) {}
  bool success :1;
} _ShardKVRaft_pullShardParams_result__isset;

class ShardKVRaft_pullShardParams_result {
 public:

  ShardKVRaft_pullShardParams_result(const ShardKVRaft_pullShardParams_result&) noexcept;
  ShardKVRaft_pullShardParams_result& operator=(const ShardKVRaft_pullShardParams_result&) noexcept;
  ShardKVRaft_pullShardParams_result() noexcept {
  }

  virtual ~ShardKVRaft_pullShardParams_result() noexcept;
  PullShardReply success;

  _ShardKVRaft_pullShardParams_result__isset __isset;

  void __set_success(const PullShardReply& val);

  bool operator == (const ShardKVRaft_pullShardParams_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const ShardKVRaft_pullShardParams_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const ShardKVRaft_pullShardParams_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _ShardKVRaft_pullShardParams_presult__isset {
  _ShardKVRaft_pullShardParams_presult__isset() : success(false) {}
  bool success :1;
} _ShardKVRaft_pullShardParams_presult__isset;

class ShardKVRaft_pullShardParams_presult {
 public:


  virtual ~ShardKVRaft_pullShardParams_presult() noexcept;
  PullShardReply* success;

  _ShardKVRaft_pullShardParams_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

class ShardKVRaftClient : virtual public ShardKVRaftIf, public KVRaftClient {
 public:
  ShardKVRaftClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) :
    KVRaftClient(prot, prot) {}
  ShardKVRaftClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) :    KVRaftClient(iprot, oprot) {}
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void pullShardParams(PullShardReply& _return, const PullShardParams& params) override;
  void send_pullShardParams(const PullShardParams& params);
  void recv_pullShardParams(PullShardReply& _return);
};

class ShardKVRaftProcessor : public KVRaftProcessor {
 protected:
  ::std::shared_ptr<ShardKVRaftIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext) override;
 private:
  typedef  void (ShardKVRaftProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_pullShardParams(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  ShardKVRaftProcessor(::std::shared_ptr<ShardKVRaftIf> iface) :
    KVRaftProcessor(iface),
    iface_(iface) {
    processMap_["pullShardParams"] = &ShardKVRaftProcessor::process_pullShardParams;
  }

  virtual ~ShardKVRaftProcessor() {}
};

class ShardKVRaftProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  ShardKVRaftProcessorFactory(const ::std::shared_ptr< ShardKVRaftIfFactory >& handlerFactory) noexcept :
      handlerFactory_(handlerFactory) {}

  ::std::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo) override;

 protected:
  ::std::shared_ptr< ShardKVRaftIfFactory > handlerFactory_;
};

class ShardKVRaftMultiface : virtual public ShardKVRaftIf, public KVRaftMultiface {
 public:
  ShardKVRaftMultiface(std::vector<std::shared_ptr<ShardKVRaftIf> >& ifaces) : ifaces_(ifaces) {
    std::vector<std::shared_ptr<ShardKVRaftIf> >::iterator iter;
    for (iter = ifaces.begin(); iter != ifaces.end(); ++iter) {
      KVRaftMultiface::add(*iter);
    }
  }
  virtual ~ShardKVRaftMultiface() {}
 protected:
  std::vector<std::shared_ptr<ShardKVRaftIf> > ifaces_;
  ShardKVRaftMultiface() {}
  void add(::std::shared_ptr<ShardKVRaftIf> iface) {
    KVRaftMultiface::add(iface);
    ifaces_.push_back(iface);
  }
 public:
  void pullShardParams(PullShardReply& _return, const PullShardParams& params) override {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->pullShardParams(_return, params);
    }
    ifaces_[i]->pullShardParams(_return, params);
    return;
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class ShardKVRaftConcurrentClient : virtual public ShardKVRaftIf, public KVRaftConcurrentClient {
 public:
  ShardKVRaftConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot, std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync) :
    KVRaftConcurrentClient(prot, prot, sync) {}
  ShardKVRaftConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot, std::shared_ptr< ::apache::thrift::async::TConcurrentClientSyncInfo> sync) :    KVRaftConcurrentClient(iprot, oprot, sync) {}
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void pullShardParams(PullShardReply& _return, const PullShardParams& params) override;
  int32_t send_pullShardParams(const PullShardParams& params);
  void recv_pullShardParams(PullShardReply& _return, const int32_t seqid);
};

#ifdef _MSC_VER
  #pragma warning( pop )
#endif



#endif
