# 分布式系统（MIT6.824) C++实现

本项目是对MIT6.824课程中分布式系统的C++实现。虽然6.824课程提供的是Go相关的资料，但出于深入理解C++的目的，本项目采用C++来完成课程的实验。为了确保正确的实现，本项目参考了6.824课程提供的测试代码，并使用`gtest`编写了大量的测试用例来验证程序功能。然而，考虑到系统的复杂性，难免可能会存在一些问题。如果发现任何问题，请尽情指出。

### 参考资料

1. [课程链接](https://pdos.csail.mit.edu/6.824/schedule.html)，分布式系统官方课程资料
2. [logcabin](https://github.com/logcabin/logcabin.git)，raft原作者在论文里面提供的raft协议C++实现
3. [Lab4实现思路](https://github.com/OneSizeFitsQuorum/MIT6.824-2021/blob/master/docs/lab4.md)

### 实现思路

1. [MapReduce](doc/MapReduce.md)
2. [Raft](doc/Raft.md)
3. [KVRaft](doc/KVRaft.md)

## 项目依赖

本项目所依赖的第三方库

| 名称       | 说明                                | 版本   | 链接                                 |
| ---------- | ----------------------------------- | ------ | ------------------------------------ |
| thrift     | 一个轻量级，跨语言的RPC库           | 0.18.1 | https://github.com/apache/thrift     |
| fmt        | 提供字符串格式化功能的库            | 10.0.0 | https://github.com/fmtlib/fmt        |
| googletest | C++测试框架                         | 1.13.0 | https://github.com/google/googletest |
| glog       | C++日志库，实现了应用级别的日志功能 | 0.6.0  | https://github.com/google/glog       |
| gflags     | C++命令行参数解析工具               | 2.2.2  | https://github.com/gflags/gflags     |


### MacOS

运行如下命令安装本项目所需依赖

```shell
brew install thrift fmt gtest glog gflags
```

### Linux
目前尝试过在ubuntu20.04环境下安装这些依赖，需要手动编译安装第三方库，流程不难但是比较繁琐。因此提供了一个预安装好所有依赖的docker镜像，配合VSCode进行远程开发，免去后续配置环境的麻烦，一举解决所有Linux平台上依赖的问题。

```shell
# 拉取镜像
docker pull 1049696130/kvraft-dev

# 运行docker容器（注意我的代码放在了~/Documents/MIT6.824这个目录下）
docker run -dit -v ~/Documents/MIT6.824:/root/MIT6.824 --name mit-build 1049696130/kvraft-dev

# 进入docker容器
docker exec -it mit-build bash
```

## 构建项目

本项目使用`make`来作为构建构建，下面分别给出了每个部分的编译
### MapReduce

执行如下命令构建MapReduce程序，并运行word count任务
```shell
make MapReduce

# 执行word count任务
cd test/mapreduce
bash test.sh
```

### Raft

执行如下命令构建Raft静态库，运行raft程序参见后续Test章节内容，以及KVRaft相关的内容。
```shell
make raft
```

### KVRaft

执行如下命令构建KVRaft静态库，以及KVRaft二进制文件
```shell
make kvraft
```

### shardkv

执行如下命令构建shardkv静态库
```shell
make shardkv
```


## Test

Raft的整体逻辑较为容易理解，但是实现的时候就会发现细节超多，因此实现raft最为痛苦的地方在于debug，本项目使用gtest来编写测试用例，运行如下的命令来运行测试用例
```shell
# 运行raft测试用例
cd test/raft
make run-test

# 运行kvraft测试用例
cd test/kvraft
make run-test

# 运行shardkv测试用例
cd test/shardkv
make run-test
```

运行指定的测试用例

```shell
make run_test cmd_args="--gtest_list_tests"
make run_test cmd_args="--gtest_filter=*testCaseName"
```

分布式系统无法直接使用gdb来debug，只能通过分析运行日志来找出错误的原因。因此在tools文件夹下，提供了一个`analyze_log.py`脚本，该脚本从`logs`文件夹下读取每个进程对应的日志，按时间排序将日志一种比较友好的方式输出到tsv文件中，用execl打开，调好列宽和自动换行就能很清晰的分析日志了。