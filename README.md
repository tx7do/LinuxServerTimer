# LinuxServerTimer

本项目使用VS2022+WSL2(Ubuntu)开发调试,主要是为了实验一下使用VS2022开发一下Linux程序是什么感觉,总的来说,感觉还行.

本项目封装了4种方式实现:

1. 使用usleep的低精度低性能定时器;  
2. 使用timerfd和epoll的高性能定时器;  
3. 使用libevent库实现的高性能定时器;
4. 使用boost::asio实现的高性能定时器.

## 使用方法

```c++
// 创建
auto pITimer = CreateServerTimer(ServerTimerType_Epollfd);
// 注册监听器
pITimer->RegisterListener(this);
// 下一个无限循环的定时器
pITimer->SetTimer(88, 1000);
// 下一个超时一次的定时器
pITimer->SetTimer(99, 1000, false);
// 杀掉一个定时器
pITimer->KillTimer(88);
// 杀掉所有的定时器
pITimer->KillAllTimer();
// 销毁定时器
DestoryServerTimer(pITimer);
```
