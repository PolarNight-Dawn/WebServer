//
// Created by polarnight on 24-3-18, 上午12:22.
//
/* 任务类函数实现 */
#include "http_conn.h"

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file from this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the requested file.\n";
static const char *doc_root = "/home/polarnight/Code/CLionProject/WebServer/root";

/* 创建数据库连接池 */
SqlConnectionPool *conn_pool = SqlConnectionPool::GetInstance("localhost", "root", "root", "webdb", 3306, 5);

map<string, string> users;

void http_conn::initmysql_result() {
    MYSQL *mysql = conn_pool->GetConnection();
    if (mysql_query(mysql, "SELECT username, passwd FROM user"))
        LOG_ERROR("SELECT error: %s\n", mysql_error(mysql));

    MYSQL_RES *result = mysql_store_result(mysql);
    int num_fields = mysql_num_fields(result);
    MYSQL_FIELD  *fields = mysql_fetch_fields(result);

    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }

    conn_pool->ReleaseConnection(mysql);
}

/* 设置非阻塞 */
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

/* 注册事件到 epoll 指定的内核事件表 */
void addfd(int epollfd, int fd, bool one_shot) {
    struct epoll_event event;
    event.data.fd = fd;

#ifdef ET
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
#endif

#ifdef LT
    event.events = EPOLLIN | EPOLLRDHUP;
#endif

    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

/* 注销 epoll 内核事件表上的事件 */
void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
}

/* 修改 epoll 内核事件表上的事件 */
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;

#ifdef ET
    event.events = ev | EPOLLIN | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
#endif

#ifdef LT
    event.events = ev | EPOLLIN | EPOLLONESHOT | EPOLLRDHUP;
#endif

    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

/* 处理客户请求 */
void http_conn::process() {
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST) {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }

    bool write_ret = process_write(read_ret);
    if (!write_ret) {
        close_conn();
    }
    modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

int http_conn::m_epollfd = -1;
int http_conn::m_user_count = 0;

/* 初始化客户连接 */
void http_conn::init(int sockfd, const sockaddr_in &address) {
    m_sockfd = sockfd;
    m_address = address;
    int error = 0;
    socklen_t len = sizeof(error);
    getsockopt(m_sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    addfd(m_epollfd, m_sockfd, true);
    m_user_count++;

    init();
}

/* 关闭客户连接 */
void http_conn::close_conn(bool real_close) {
    if (real_close && (m_sockfd != -1)) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

/* 初始化连接 */
void http_conn::init() {
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = nullptr;
    m_host = nullptr;
    m_version = nullptr;
    m_content_length = 0;
    m_start_line = 0;
    m_check_idx = 0;
    m_read_idx = 0;
    m_file_address = nullptr;
    m_write_idx = 0;
    cgi = 0;
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

/* 读取客户数据 */
bool http_conn::read() {
    if (m_read_idx >= READ_BUFFER_SIZE) {
        return false;
    }
    int byte_read = 0;
    while (true) {
        byte_read = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        if (byte_read == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            return false;
        } else if (byte_read == 0) {
            return false;
        }
        m_read_idx += byte_read;
    }
    return true;
}

http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char *text = nullptr;

    while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) ||
           ((line_status = parse_line()) == LINE_OK)) {
        text = get_line();
        m_start_line = m_check_idx;

        switch (m_check_state) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                break;
            }
            case CHECK_STATE_HEADER: {
                ret = parse_headers(text);
                if (ret == BAD_REQUEST)
                    return BAD_REQUEST;
                else if (ret == GET_REQUEST)
                    return do_request();
                break;
            }
            case CHECK_STATE_CONTENT: {
                ret = parse_content(text);
                if (ret == GET_REQUEST)
                    return do_request();

                line_status = LINE_OPEN;
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }
    return NO_REQUEST;
}

http_conn::LINE_STATUS http_conn::parse_line() {
    char temp;
    for (; m_check_idx < m_read_idx; m_check_idx++) {
        temp = m_read_buf[m_check_idx];

        if (temp == '\r') {
            if ((m_check_idx + 1) == m_read_idx) {
                return LINE_OPEN;
            } else if (m_read_buf[m_check_idx + 1] == '\n') {
                m_read_buf[m_check_idx++] = '\0';
                m_read_buf[m_check_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            if ((m_check_idx > 1) && (m_read_buf[m_check_idx - 1] == '\r')) {
                m_read_buf[m_check_idx - 1] = '\0';
                m_read_buf[m_check_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {
    m_url = strpbrk(text, " \t");
    if (!m_url) {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
        m_method = GET;
    else
        return BAD_REQUEST;

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if (!m_version)
        return BAD_REQUEST;
    *m_version++ = '\0';
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_version, "HTTP/1.1") != 0)
        return BAD_REQUEST;

    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    if (strncasecmp(m_url, "https://", 8) == 0) {
        m_url += 8;
        m_url = strchr(m_url, '/');
    }

    if (!m_url || m_url[0] != '/')
        return BAD_REQUEST;

    if (strlen(m_url) == 1)
        strcat(m_url, "default.html");

    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_headers(char *text) {
    if (text[0] == '\0') {
        if (m_content_length != 0) {
            /* POST 请求 需要跳到消息体处理状态 */
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    } else if (strncasecmp(text, "Content-Length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    } else {
        printf("unknown header %s\n", text);
    };
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_content(char *text) {
    if (m_read_idx >= (m_content_length + m_check_idx)) {
        text[m_content_length] = '\0';

        /* POST 请求中最后输入为用户名和密码 */
        m_string = text;

        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_request() {
    strcpy(m_real_file, doc_root);
    int len = strlen(doc_root);

    const char *ptr = strrchr(m_url, '/');

    if(cgi==1 && (*(ptr+1) == '2' || *(ptr+1) == '3'))
    {

        //根据标志判断是登录检测还是注册检测
        char flag = m_url[1];
        //printf("====+++====+++%c\n", flag);

        char *m_url_real = (char *)malloc(sizeof(char) * 200);
        strcpy(m_url_real, "/");
        strcat(m_url_real, m_url+2);
        strncpy(m_real_file+len,m_url_real,FILENAME_LEN-len-1);
        free(m_url_real);

        //将用户名和密码提取出来
        //user=123&passwd=123
        char name[100],password[100];
        int i;
        for(i=5;m_string[i]!='&';++i)
            name[i-5]=m_string[i];
        name[i-5]='\0';
        int j=0;
        for(i=i+10;m_string[i]!='\0';++i,++j)
            password[j]=m_string[i];
        password[j]='\0';

//CGI多进程登录校验
#if 0

        //fd[0]:读管道，fd[1]:写管道
        pid_t pid;
        int pipefd[2];
        if(pipe(pipefd)<0)
        {
            LOG_ERROR("pipe() error:%d",4);
            return BAD_REQUEST;
        }
        if((pid=fork())<0)
        {
            LOG_ERROR("fork() error:%d",3);
            return BAD_REQUEST;
        }

        if(pid==0)
        {
	    //标准输出，文件描述符是1，然后将输出重定向到管道写端
            dup2(pipefd[1],1);
	    //关闭管道的读端
            close(pipefd[0]);
	    //父进程去执行cgi程序，m_real_file,name,password为输入
	    //./check.cgi name password

            execl(m_real_file,&flag,name,password, NULL);
        }
        else{
	    //printf("子进程\n");
	    //子进程关闭写端，打开读端，读取父进程的输出
            close(pipefd[1]);
            char result;
            int ret=read(pipefd[0],&result,1);

            if(ret!=1)
            {
                LOG_ERROR("管道read error:ret=%d",ret);
                return BAD_REQUEST;
            }
	    if(flag == '2'){
		    //printf("登录检测\n");
		    LOG_INFO("%s","登录检测");
    		    Log::get_instance()->flush();
		    //当用户名和密码正确，则显示welcome界面，否则显示错误界面
		    if(result=='1')
			strcpy(m_url, "/welcome.html");
		        //m_url="/welcome.html";
		    else
			strcpy(m_url, "/logError.html");
		        //m_url="/logError.html";
	    }
	    else if(flag == '3'){
		    //printf("注册检测\n");
		    LOG_INFO("%s","注册检测");
    		    Log::get_instance()->flush();
		    //当成功注册后，则显示登陆界面，否则显示错误界面
		    if(result=='1')
			strcpy(m_url, "/log.html");
			//m_url="/log.html";
		    else
			strcpy(m_url, "/registerError.html");
			//m_url="/registerError.html";
	    }
	    //printf("m_url:%s\n", m_url);
	    //回收进程资源
            waitpid(pid,NULL,0);
	    //waitpid(pid,0,NULL);
	    //printf("回收完成\n");
        }
#endif
    }

    if (*(ptr + 1) == '0') {
        char *m_real_url = (char *) malloc(sizeof(char) * 200);
        strcpy(m_real_url, "/register.html");

        strncpy(m_real_file + len, m_real_url, strlen(m_real_url));

        free(m_real_url);
    } else if (*(ptr + 1) == '1') {
        char *m_real_url = (char *) malloc(sizeof(char) * 200);
        strcpy(m_real_url, "/log.html");

        strncpy(m_real_file + len, m_real_url, strlen(m_real_url));

        free(m_real_url);
    } else {
        strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
    }

    if (stat(m_real_file, &m_file_stat) < 0)
        return NO_RESOURCE;
    if (!(m_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;
    if (S_ISDIR(m_file_stat.st_mode))
        return BAD_REQUEST;

    int fd = open(m_real_file, O_RDONLY);
    m_file_address = (char *) mmap(nullptr, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close(fd);
    return FILE_REQUEST;
}

void http_conn::unmap() {
    if (m_file_address) {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = nullptr;
    }
}

/* 写入客户数据 */
bool http_conn::write() {
    int temp = 0;
    int newadd = 0;
    int byte_to_send = m_write_idx;
    int byte_have_send = 0;
    if (byte_to_send == 0) {
        modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }
    while (true) {
        temp = writev(m_sockfd, m_iv, m_iv_count);
        if (temp > 0) {
            byte_have_send += temp;
            newadd = byte_have_send - m_write_idx;
        }
        if (temp <= -1) {
            if (errno == EAGAIN) {
                /* 对于大文件传输 bug的 修复 */
                if (byte_have_send >= m_iv[0].iov_len) {
                    m_iv[0].iov_len = 0;
                    m_iv[1].iov_base = m_file_address + newadd;
                    m_iv[1].iov_len = byte_to_send;
                } else {
                    m_iv[0].iov_base = m_write_buf + byte_to_send;
                    m_iv[0].iov_len = m_iv[0].iov_len - byte_have_send;
                }
                modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        byte_to_send -= temp;
        if (byte_to_send <= byte_have_send) {
            unmap();
            modfd(m_epollfd, m_sockfd, EPOLLIN);
            if (m_linger) {
                init();
                return true;
            } else {
                return false;
            }
        }
    }
}

bool http_conn::add_response(const char *format, ...) {
    if (m_write_idx >= WRITE_BUFFER_SIZE) {
        return false;
    }

    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx)) {
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);

    return true;
}

bool http_conn::add_status_line(int status, const char *title) {
    return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_header(int content_len) {
    add_content_length(content_len);
    add_linger();
    add_blank_line();
    return true;
}

bool http_conn::add_content_length(int content_len) {
    return add_response("Content-Length: %d\r\n", content_len);
}

bool http_conn::add_content_type() {
    return add_response("Content-Type: %s\r\n", "text/html");
}

bool http_conn::add_linger() {
    return add_response("Connection: %s\r\n", m_linger ? "keep-alive" : "close");
}

bool http_conn::add_blank_line() {
    return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char *content) {
    return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret) {
    switch (ret) {
        case INTERNAL_ERROR: {
            add_status_line(500, error_500_title);
            add_header(strlen(error_500_form));
            if (!add_content(error_500_form)) {
                return false;
            }
            break;
        }
        case BAD_REQUEST: {
            add_status_line(400, error_400_title);
            add_header(strlen(error_400_form));
            if (!add_content(error_400_form)) {
                return false;
            }
            break;
        }
        case NO_RESOURCE: {
            add_status_line(404, error_404_title);
            add_header(strlen(error_404_form));
            if (!add_content(error_404_form)) {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST: {
            add_status_line(403, error_403_title);
            add_header(strlen(error_403_form));
            if (!add_content(error_403_form)) {
                return false;
            }
            break;
        }
        case FILE_REQUEST: {
            add_status_line(200, ok_200_title);
            if (m_file_stat.st_size != 0) {
                add_header(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                return true;
            } else {
                const char *ok_string = "";
                add_header(strlen(ok_string));
                if (!add_content(ok_string)) {
                    return false;
                }
            }
        }
        default: {
            return false;
        }
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    return true;
}