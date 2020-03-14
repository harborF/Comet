1\分为三个模块client（客服机）logic（逻辑），公共模块为common、server；
2\server支持http、tcp、websocket三种，默认端口分别为8888,8886；
3\实现逻辑：client连接logic，logic通过http连接status服务，由status保存状态到redis。如果重复
	登录，则由status通知logic相应session退出连接。
4\如果已经登录，可由朴实服务推送消息，转发到logic，通过tcp推送。推送地址如下：
	http://localhost:8880/pushMsg?member_id=&channel=&type=&logic_thread=&msg_body=
5\日志输出，默认为/tmp/master.log /tmp/client.log
6\增加配置文件，默认 配置文件名=程序名+.cf，如master_server.cf

7\src/thrift_protos目录下为服务端和客户端 协议定义文件，通过
	thrift -r --gen cpp -o ./ StateSvr.thrift 生成代码文件
8\src/thrift_srv为服务端代码文件，修改svr_job.h svr_job.cpp即可编译生成thrift服务
9\src/thrift_protos目录下存放ThriftProxy.cpp 用户请求thrift服务

cmd seq
reload config
cmdid ip port ha
thrift模块添加进去

libgo

