### Linux基础

### 6.thrift教程

Thrift是一种接口描述语言和二进制通讯协议，它被用来定义和创建跨语言的服务，它常被当作一个远程过程调用（RPC）框架来使用。

本项目使用thrift搭建起一个匹配服务系统，匹配系统的server端与数据存储服务器使用C++实现，请求的client端使用python实现。

作为匹配系统的server端有两个作用：1.对于来自Game请求的client端，它是一个接收请求的match-server，可以做匹配并返回结果；2.对于数据存储的服务器端，它是一个请求存储信息的save-client端。

1.首先通过 .thrift文件定义接口
以 `thrift -r --gen <language> <Thrift filename>` 自动生成接口代码的框架。

2.实现server端match_system
创建match_server文件夹，并以 `thrift -r --gen cpp <match.thrift的文件路径>` 生成 gen-cpp文件，将…skeleton.cpp重命名为main.cpp。

3.实现client端game
创建game文件夹，并以 `thrift -r --gen py <match.thrift的文件路径>` 生成gen-py，重命名为match_client。

4.实现save-client端
以 `thrift -r --gen cpp <save.thrift的地址>` 生成gen-cpp，重命名并且删除服务端文件，进入cpp教程，将client代码复制到pool中的result函数并删掉教程演示代码进行修改。

5.数据服务器端
save.thrift 文件(现成)。





**实现思想：**
- 建立结构体Task用来作为消息队列的最小操作单元，同时为实现进程的同步与互斥，需要建立结构体MessageQueue表示消息队列结构，里面有以queue做的存放操作消息的消息队列、锁 m 以及环境变量 cv 。

- 定义匹配池pool，新添加的用户进入匹配池，撤销的用户从匹配池删除，匹配玩家从匹配池中取出符合条件的两个用户。
  匹配池所持支持的操作：存储用户、添加用户、删除用户、记录结果

- 定义consume_task
  队列为空，没有任务，调用wait主动阻塞，等待消息队列不为空时由match_handle唤醒。防止死锁。
  队列不为空，有任务，执行任务–>添加、删除与匹配。

- 通过引入互斥锁对临界区进行管理，不管是进行执行匹配（pool.match）的操作，还是从game端向匹配系统端执行add_user或者是remove_user的操作时都需要以互斥锁mutex对其进行管理，具体操作为：unique_lock<mutex> lck(message_queue.m);

- 因为将mutex引入进来了，为了锁的管理以及不同线程之间上锁与解锁的通知，将环境变量 condition_variable cv 引入。以 cv 蚌住匹配循环，防止一直匹配循环一直空转而将cpu资源占满，每当有add_user或者remove_user的操作时，在add_user与remove_user增加一句 message_queue.cv.notify_all() 以通知所有被环境变量卡住的线程（all/one代表所有线程或者随机一个线程）。

- 使用thread开启多线程，原始的Simple_thread需要升级，把thrift的cpp教程中server的main和factor复制过来，把函数名改了，并将单独server模式改成多线程server模式：
```
std::make_shared<MatchProcessorFactory>(std::make_shared<MatchCloneFactory>()),
            std::make_shared<TServerSocket>(9090), //port
            std::make_shared<TBufferedTransportFactory>(),
            std::make_shared<TBinaryProtocolFactory>());
```











