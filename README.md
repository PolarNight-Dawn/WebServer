# WebServer

[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)


## 核心技术实现

- 使用半同步/半反应堆线程池提高并发度，并降低频繁创建线程的开销
- 使用有限状态机解析 HTTP 请求，目前支持 GET 和 POST 方法
- 使用 IO复用epoll + 非阻塞IO + 事件处理模式ET 实现高并发处理请求，使用模拟 Proactor 模型
- 使用 EPOLLONESHOT 事件确保一个 socket 连接在任一时刻都只被一个线程处理
- 使用基于升序双向链表的定时器处理非活动连接

## 优化计划

- 实现非阻塞模式下的 ET 和 LT 触发