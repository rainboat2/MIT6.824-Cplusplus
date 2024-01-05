# ShardKV实现

基于mult-raft架构，实现数据的分片、迁移等操作。

## 总体设计

如下图给出了ShardKV的总体架构图：

![ShardKV架构](img/shardkv.png)



## KV操作流程

1. 用户发起请求
2. ShardGroup获得请求，调用raft日志同步，同时等待结果
3. raft完成同步后，调用ShardManger的apply方法
4. apply完成请求后，通知等待线程，将结果返回给用户



