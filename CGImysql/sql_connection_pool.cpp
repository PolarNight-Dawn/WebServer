//
// Created by polarnight on 24-3-22, 上午11:18.
//

#include "sql_connection_pool.h"

SqlConnectionPool *SqlConnectionPool::conn_pool = nullptr;

SqlConnectionPool::SqlConnectionPool(std::string url, std::string user, std::string password, std::string database_name,
                                     int port, unsigned int max_conn) {
    this->url = url;
    this->port = port;
    this->password = password;
    this->database_name = database_name;

    lock.lock();
    for (int i = 0; i < max_conn; i++) {
        MYSQL *con = nullptr;
        con = mysql_init(con);

        if (con == nullptr) {
            cout << "Error:" << mysql_error(con);
            exit(1);
        }

        con = mysql_real_connect(con, url.c_str(), user.c_str(), password.c_str(), database_name.c_str(), port, nullptr,
                                 0);
        if (con == nullptr) {
            cout << "Error:" << mysql_error(con);
            exit(1);
        }

        conn_list.push_back(con);
        ++free_conn;
    }

    this->max_conn = max_conn;
    this->cur_conn = 0;
    lock.unlock();
}

SqlConnectionPool *
SqlConnectionPool::GetInstance(std::string url, std::string user, std::string password, std::string data_name, int post,
                               unsigned int max_conn) {
    if (conn_pool == nullptr) {
        conn_pool = new SqlConnectionPool(url, user, password, data_name, post, max_conn);
    }

    return conn_pool;
}

MYSQL *SqlConnectionPool::GetConnection() {
    MYSQL *con = nullptr;
    lock.lock();
    if (conn_list.size() > 0) {
        con = conn_list.front();
        conn_list.pop_front();

        --free_conn;
        ++cur_conn;

        lock.unlock();
        return con;
    }

    return nullptr;
}

bool SqlConnectionPool::ReleaseConnection(MYSQL *conn) {
    lock.lock();
    if (conn != nullptr) {
        conn_list.push_back(conn);
        ++free_conn;
        --cur_conn;

        lock.unlock();
        return true;
    }

    return false;
}

void SqlConnectionPool::DestroyPool() {
    lock.lock();
    if (conn_list.size() > 0) {
        list<MYSQL *>::iterator it;
        for (it = conn_list.begin(); it != conn_list.end(); it++) {
            MYSQL *con = *it;
            mysql_close(con);
        }
        cur_conn = 0;
        free_conn = 0;
        conn_list.clear();
    }
}

int SqlConnectionPool::GetFreeConn() {
    return this->free_conn;
}

SqlConnectionPool::~SqlConnectionPool() {
    DestroyPool();
}




