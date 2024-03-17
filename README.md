# WebServer

[![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)

## 核心技术实现

- 使用半同步/半异步线程池池提高并发度，并降低频繁创建线程的开销
- 使用有限状态机解析 HTTP 请求，目前支持 GET 方法
- 使用 epoll + 非阻塞IO + ET实现高并发处理请求，使用模拟 Proactor 模型
- 使用 EPOLLONESHOT 事件确保一个 socket 连接在任一时刻都只被一个线程处理

## 优化计划