// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "match_server/Match.h"
#include "save_client/Save.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TSocket.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <unistd.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace ::match_service;
using namespace ::save_service;
using namespace std;

struct Task{
    User user;
    string type;
};

struct MessageQueue{
    queue<Task> q;
    mutex m;
    condition_variable cv;
}message_queue;

class Pool{
    public:
        void save_result(int a,int b){
            printf("Match Result: %d %d\n",a,b);

            std::shared_ptr<TTransport> socket(new TSocket("123.57.67.128", 9090));
            std::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            SaveClient client(protocol);

            try {
                transport->open();

                int res=client.save_data("acs_9992","87d5a224",a,b);
                if(!res) puts("success");
                else puts("failed");

                transport->close();
            } catch (TException& tx) {
                cout << "ERROR: " << tx.what() << endl;
            }
        }

        void match(){
            while(users.size()>1){
                sort(users.begin(),users.end(),[&](User& a,User& b){
                    return a.score < b.score;
                });

                bool flag=true;         //保证while循环不能死循环，如果分差1000且只有两个人的话就会死循环

                for(uint32_t i=1;i<users.size();i++){
                    auto a=users[i-1], b=users[i];
                    if(b.score-a.score<=50){
                        users.erase(users.begin()+i-1,users.begin()+i+1);           //左闭右开的区间
                        save_result(a.id,b.id);

                        flag=false;
                        break;
                    }
                }

                if(flag) break;         //flag为true说明没有任何玩家可以匹配，跳出while
            }
        }

        void add(User user){
            users.push_back(user);
        }

        void remove(User user){
            for(uint32_t i=0;i<users.size();i++){
                if(users[i].id==user.id){
                    users.erase(users.begin()+i);
                    break;
                }
            }
        }
    private:

        vector<User> users;         //玩家users用vector存
}pool;

class MatchHandler : virtual public MatchIf {
    public:
        MatchHandler() {
            // Your initialization goes here
        }

        int32_t add_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("add_user\n");

            unique_lock<mutex> lck(message_queue.m);    //这样上锁可以不用手动解锁，函数执行完后会自动销毁
            message_queue.q.push({user, "add"});
            message_queue.cv.notify_all();      //通知所有被环境变量卡住的线程，all/one代表所有线程或者随机一个线程

            return 0;
        }

        int32_t remove_user(const User& user, const std::string& info) {
            // Your implementation goes here
            printf("remove_user\n");

            unique_lock<mutex> lck(message_queue.m);
            message_queue.q.push({user, "remove"});
            message_queue.cv.notify_all();

            return 0;
        }

};

void consume_task(){
    while(true){
        unique_lock<mutex> lck(message_queue.m);
        if(message_queue.q.empty()){
            //message_queue.cv.wait(lck);     如果线程是空的，那就应该蚌住，先将锁释放掉然后卡死在这，直到环境变量被其他地方唤醒
            lck.unlock();       //解锁
            pool.match();            //匹配
            sleep(1);           //睡眠一秒然后继续
        }
        else{
            auto task=message_queue.q.front();
            message_queue.q.pop();
            lck.unlock();       //共享变量处理完后记得解锁，不然持有锁时间太长卡死另外add跟remove两个线程

            //do task
            if(task.type=="add") pool.add(task.user);
            else if(task.type=="remove") pool.remove(task.user);

            pool.match();
        }
    }
}

int main(int argc, char **argv) {
    int port = 9090;
    ::std::shared_ptr<MatchHandler> handler(new MatchHandler());
    ::std::shared_ptr<TProcessor> processor(new MatchProcessor(handler));
    ::std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    ::std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    ::std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);

    cout << "Start Match Server" << endl;

    thread matching_thread(consume_task);

    server.serve();
    return 0;
}

