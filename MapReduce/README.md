# MapReduce实现

## 运行程序

```shell
# 安装依赖
brew install thrift fmt glog 

# 编译程序
make

# 运行word count的map reduce任务
bash test.sh
```





### 

## Master设计

> 主要代码逻辑在src/master.cpp里面

master主要负责map和reduce任务的分配，主要包括如下的内容：

1. 用一个数据结构维护map任务，记录每个任务对应的完成状态（idle, in_process, completed)
2. 用一个数据结构维护reduce任务，记录每个任务对应的完成状态（idle, in_process, completed)
3. 负责相应worker节点的任务请求：
   1. 如果尚有未分配的map任务，将任务指定给相应的节点
   2. 如果所有map任务都已经分配，但是有一部分尚未完成，告诉节点等一会
   3. 如果所有map任务都已经完成，分配reduce任务
   4. 如果所有reduce任务都已经分配完成，告诉节点等一会
   5. 如果reduce都已经完成，告诉节点已经结束，可以退出
   6. 设定一个线程，定时检查所有正在处理的任务状态，如果处理时间过长，将任务列为idle状态
4. 负责接收每个节点任务完成的情况

## worker设计

> 主要逻辑在src/woker.cpp里面

worker进程单线程运行，主要负责不断向Master请求任务，按照master的响应执行对应的动作

