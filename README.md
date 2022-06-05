# LinuxServerTimer

本项目主要是为了实验一下使用WSL开发Linux程序。

一开始用的是VS2022，但是用的感觉不是特别好，调试起来不太稳定，因为没有装VA，所以编写代码起来也很难受。

后来转投JetBrian的CLion，编写代码比较舒服，支持Clang-Tidy，代码提示也比较精准。调试起来也很舒服，基本上没有出现什么问题。而且，在使用过程中，我发现它对Doxygen的支持也非常的好。

VS呢，我也用了十多年了，以前用VS+VA，用起来感觉还是很愉快的。但是在JetBrain全家桶面前，我发现它差的有点远。

本项目封装了4种实现方式：

1. 使用sleep实现的低精度低性能定时器；
2. 使用timerfd和epoll实现的高精度高性能定时器；  
3. 使用libevent2实现的高精度高性能定时器；
4. 使用boost::asio实现的高精度高性能定时器。

## 安装依赖库

```bash
sudo apt-get install libevent-dev
sudo apt-get install libboost-all-dev
```

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
DestroyServerTimer(pITimer);
```
