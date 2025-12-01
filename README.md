# selfServer

### 流程

#### webserver

创建出定时器（timer）， 线程池（ThreadPool）， IO复用（epoller）

1. 先初始化监听的socket，绑定监听端口，设置成非阻塞，实现高并发，使用epoll监听请求
2. 收到连接请求，完成tcp三次握手，为客服端生成文件描述符fd，使用httpconn管理客服端，然后把fd注册进epoll的监听队列。
3. 收到网页请求，把epollin的标识位发生变化，epoll_wait,从中取出请求，主线程分发到任务队列，工作线程轮询任务队列，从中取出网页请求处理，工作线程调用httpconn的read，读出请求，处理，然后构造respond，改变epollout标志，把回复时间注册进epoll。
4. epoll观察到epollout变化，通知主线程，主线程分发到任务队列，工作线程取出任务，调用httpconn的write，写回socket中，客户端就可以解析网页了

### EPOLL

```c++
epoll_create1 (int __flags); //创建epoll的文件描述符

//控制对应epoll里面的event，EPOLL_CTL_ADD 添加， EPOLL_CTL_MOD 修改 EPOLL_CTL_DEL删除
epoll_ctl (int __epfd, int __op, int __fd,
		      struct epoll_event *__event); 
		      
//传入一个任务队列，和时间，但时间到达，或者有任务请求时，把时间加入到events里面
epoll_wait (int __epfd, struct epoll_event *__events,
		       int __maxevents, int __timeout);

//ET边缘触发 内核一次通知，剩下的就交给上层了
//LT水平触发，只要还有数据，就一直通知
//EPOLLONESHOT 保证任何时刻，一个socket只能被一个线程拥有，一定要记得重置不然这个soket就只能等死了
```

