//
// Created by polarnight on 24-3-25, 下午4:04.
//

#include <mysql/mysql.h>
#include <iostream>
#include <cstdio>
#include <map>
#include <cstring>

#include "sql_connection_pool.h"
#include "../lock/locker.h"

using namespace std;

int main(int argc, char *argv[]) {
    map<string, string> users;

    locker lock;
    SqlConnectionPool *conn_pool = SqlConnectionPool::GetInstance("localhost", "root", "root", "webdb", 3306, 5);

    MYSQL *mysql = conn_pool->GetConnection();
    if (mysql_query(mysql, "SELECT username, passwd FROM user")) {
        printf("INSERT error: %s\n", mysql_error(mysql));
        return -1;
    }

    MYSQL_RES *result = mysql_store_result(mysql);
    int num_fields = mysql_num_fields(result);
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }

    string name(argv[1]);
    const char *name_p = name.c_str();
    string passwd(argv[2]);
    const char *passwd_dp = passwd.c_str();
    char flag = *argv[0];

    char *sql_insert = (char *) malloc(sizeof(char) * 200);
    strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
    strcat(sql_insert, "'");
    strcat(sql_insert, name_p);
    strcat(sql_insert, "', '");
    strcat(sql_insert, passwd_dp);
    strcat(sql_insert, "')");

    if (flag == '3') {
        if (users.find(name) == users.end()) {

            lock.lock();
            int res = mysql_query(mysql, sql_insert);
            lock.unlock();

            if (!res)
                printf("1\n");
            else
                printf("0\n");
        } else
            printf("0\n");
    } else if (flag == '2') {
        if (users.find(name) != users.end() && users[name] == passwd)
            printf("1\n");
        else
            printf("0\n");
    } else
        printf("0\n");
    //释放结果集使用的内存
    mysql_free_result(result);

    conn_pool->DestroyPool();
}