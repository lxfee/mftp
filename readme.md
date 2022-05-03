### mftp
基于TCP的xjb写的socket通信工具。分客户端和服务端。服务端多线程，客户端可以连接服务端，执行上传文件和下载文件等操作。

### 编译
linux平台`make server`编译服务端，`make client`编译客户端，`make all`同时编译服务端和客户端。

windows平台`mingw32-make platform=windows server`编译服务端，其余类似。

### 参考
使用了github上面的一个json库，[地址](https://github.com/nlohmann/json)