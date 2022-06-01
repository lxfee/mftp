# mftp

基于TCP的socket通信工具。分客户端和服务端。服务端多线程，客户端可以连接服务端，执行上传文件和下载文件等操作。

## 编译

linux平台`make server`编译服务端，`make client`编译客户端，`make all`同时编译服务端和客户端。

windows平台`mingw32-make platform=windows server`编译服务端，其余类似。

## 使用

服务端运行，客户端可以使用`open <ip:port>`指令连接服务端。

使用debug指令可以打开调试窗口

使用mode指令可以切换主动模式和被动模式

## 参考

使用了github上面的一个json库，[地址](https://github.com/nlohmann/json)

## TODO

+ [ ] 解决中文在不同平台下编码问题
+ [ ] 重构代码，规范协议通信过程，避免过度设计