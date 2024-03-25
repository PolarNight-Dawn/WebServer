//
// Created by polarnight on 24-3-22, 上午11:18.
//

#ifndef WEBSERVER_SQL_CONNECTION_POOL_H
#define WEBSERVER_SQL_CONNECTION_POOL_H

/* 数据库连接池类 */

#include <mysql/mysql.h>
#include <string>
#include <list>
#include <iostream>

#include "../lock/locker.h"

using namespace std;

class SqlConnectionPool {
public:
    /* 获取数据库连接 */
    MYSQL *GetConnection();

    /* 释放连接 */
    bool ReleaseConnection(MYSQL *conn);

    /* 销毁所有连接 */
    void DestroyPool();

    static SqlConnectionPool *
    GetInstance(string url, string user, string password, string data_name, int post, unsigned int max_conn);

    int GetFreeConn();

private:
    SqlConnectionPool(string url, string user, string password, string database_name, int port, unsigned int max_conn);

    ~SqlConnectionPool();

private:
    unsigned int max_conn;
    unsigned int cur_conn;
    unsigned int free_conn;

    locker lock;
    list<MYSQL *> conn_list;
    SqlConnectionPool *conn;
    MYSQL *con;
    static SqlConnectionPool *conn_pool;

    string url;
    string port;
    string user;
    string password;
    string database_name;
};


#endif //WEBSERVER_SQL_CONNECTION_POOL_H
