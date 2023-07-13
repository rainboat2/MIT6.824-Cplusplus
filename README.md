# 分布式系统（MIT6.824) C++实现

[课程链接](https://pdos.csail.mit.edu/6.824/schedule.html)

实现思路：

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

运行Raft测试用例
```shell
cd test/raft
make run-test
```

运行指定的测试用例

```shell
make run_test cmd_args="--gtest_list_tests"
make run_test cmd_args="--gtest_filter=*testCaseName"
```