# 分布式系统（MIT6.824) C++实现

本项目是对MIT6.824课程中分布式系统的C++实现。虽然6.824课程提供的是Go相关的资料，但出于深入理解C++的目的，本项目另辟蹊径，采用C++来完成课程的实验。不得不说，6.824课程弃用C++，转而使用Go是有一定道理的。C++在错误提示，内存管理，以及并发编程方面相比Go都要麻烦太多了。为了确保正确的实现，本项目参考了6.824课程提供的测试代码，并使用`gtest`编写了大量的测试用例来验证程序功能。然而，考虑到系统的复杂性，难免可能会存在一些问题。如果发现任何问题，请尽情指出。

### 参考资料

1. [课程链接](https://pdos.csail.mit.edu/6.824/schedule.html)，分布式系统官方课程资料
2. [logcabin](https://github.com/logcabin/logcabin.git)，raft原作者在论文里面提供的，一个raft的C++实现

### 实现思路

1. [MapReduce](https://github.com/rainboat2/MIT6.824-Cplusplus/blob/main/MapReduce.md)
2. [Raft](https://github.com/rainboat2/MIT6.824-Cplusplus/blob/main/Raft.md)

## 项目依赖

### MacOS

运行如下命令安装本项目所需依赖

```shell
brew install thrift fmt gtest glog gflags
```

## 构建项目

### MapReduce

执行如下命令构建MapReduce程序，并运行word count任务
```shell
cd MapReduce
make

# 执行word count任务
bash test.sh
```

### Raft

执行如下命令构建Raft静态库，运行raft程序参见后续Test章节内容
```shell
cd Raft
make
```

## Test

Raft的整体逻辑较为容易理解，但是实现的时候就会发现细节超多，因此实现raft最为痛苦的地方在于debug，本项目使用gtest来编写测试用例，运行如下的命令来运行测试用例
```shell
cd test/raft
make run-test
```

运行指定的测试用例

```shell
make run_test cmd_args="--gtest_list_tests"
make run_test cmd_args="--gtest_filter=*testCaseName"
```

分布式系统无法直接使用gdb来debug，只能通过分析运行日志来找出错误的原因。因此在tools文件夹下，提供了一个`analyze_log.py`脚本，该脚本从`logs`文件夹下读取每个进程对应的日志，按时间排序将日志一种比较友好的方式输出到tsv文件中，用execl打开，调好列宽和自动换行就能很清晰的分析日志了。