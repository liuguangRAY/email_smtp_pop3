#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <mysql/mysql.h>
#include "common.h"
#define BUFFER_SIZE 1024
char helo[100]="";

sem_t sem;

void doChild(int No)
{
    if(No == SIGCHLD)
    {
        while(waitpid(-1, NULL, WNOHANG) > 0);
    }
}

int validate_email(const char* email) {
    MYSQL* conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "127.0.0.1", "root", "123456", "db_Mailserver", 0, NULL, 0)) {
        fprintf(stderr, "数据库连接失败: %s\n", mysql_error(conn));
        return 0;
    }
    char query[256];
    snprintf(query, sizeof(query), "SELECT m_mail FROM tb_users WHERE m_mail='%s'", email);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "查询失败: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 0;
    }
    MYSQL_RES* result = mysql_store_result(conn);
    int exists = (mysql_num_rows(result) > 0);
    mysql_free_result(result);
    mysql_close(conn);
    return exists;
}

int command(int cfd, const char* cmd, const char* prefix, const char* suc, const char* err)
{
    char buf[BUFFER_SIZE];
    int bytes = recv(cfd, buf, sizeof(buf) - 1, 0);
    buf[bytes] = '\0';
    if(strncmp(buf, prefix, strlen(prefix)) == 0)
    {
        if(strcmp(cmd, "HELO") == 0)
        {
            sscanf(buf + strlen("HELO"), "%s", helo);
        }
        send(cfd, suc, strlen(suc), 0);
        return 1;
    }
    else
    {
        send(cfd, err, strlen(err), 0);
        return 0;
    }
}

int mail_from(int cfd, char* sender)
{
    char buf[BUFFER_SIZE];
    int bytes = recv(cfd, buf, sizeof(buf) - 1, 0);
    buf[bytes] = '\0';

    if (sscanf(buf, "MAIL FROM:<%[^>]>", sender) != 1) {
        send(cfd, "501参数格式错误\r\n", 15, 0);
        return 0;
    }
    if (!validate_email(sender)) {
        send(cfd, "550邮箱不存在\r\n", 14, 0);
        return 0;
    }
    send(cfd, "250 OK\r\n", 8, 0);
    return 1;
}

int rcpt_to(int cfd, char* receiver)
{
    char buf[BUFFER_SIZE];
    int bytes = recv(cfd, buf, sizeof(buf) - 1, 0);
    buf[bytes] = '\0';
    if(sscanf(buf, "RCPT TO:<%[^>]>", receiver) != 1)
    {
        send(cfd, "501参数格式错误\r\n", 15, 0);
        return 0;
    }

    if(!validate_email(receiver))
    {
        send(cfd, "550邮箱不存在\r\n", 14, 0);
        return 0;
    }

    send(cfd, "250 Ok\r\n", 8, 0);
    return 1;
}

void store(int cfd, const char* receiver)
{
    FILE* file;
    char filename[100];
    snprintf(filename, sizeof(filename), "%s.eml", receiver);
    file = fopen(filename, "w");
    if(file == NULL)
    {
        perror("无法打开文件");
        send(cfd, "500 Error\r\n", strlen("550 Error\r\n"), 0);
        return;
    }

    char buf[BUFFER_SIZE];
    while(1)
    {
        int bytes = recv(cfd, buf, sizeof(buf) - 1, 0);
        if(bytes <= 0)
        {
            break;
        }
        buf[bytes] = '\0';
        if(strcmp(buf, ".\r\n") == 0 || strcmp(buf, ".\n") == 0)
        {
            break;
        }
        fwrite(buf, 1, bytes, file);
    }
    fclose(file);
    send(cfd, "250 OK\r\n", strlen("250 OK\r\n"), 0);
}

void connection(int cfd)
{
    char sender[256];
    char receiver[256];
    const char* ready = "OK SMTP service ready";
    send(cfd, ready, strlen(ready), 0);
    if(!command(cfd, "HELO", "HELO ", "250\n", "500\n"))
    {
        close(cfd);
        return;
    }
    if(!mail_from(cfd, sender))
    {
        close(cfd);
        return;
    }
    if(!rcpt_to(cfd, receiver))
    {
        close(cfd);
        return;
    }
    char data_cmd[BUFFER_SIZE];
    recv(cfd, data_cmd, sizeof(data_cmd), 0);
    if(strncmp(data_cmd, "DATA", 4) == 0)
    {
        send(cfd, "354输入邮件内容,以‘.’结束\r\n", 23, 0);
        store(cfd, receiver);
    }
    char quit_cmd[BUFFER_SIZE];
    recv(cfd, quit_cmd, sizeof(quit_cmd), 0);
    if(strncmp(quit_cmd, "QUIT", 4) == 0)
    {
        send(cfd, "221服务关闭\r\n", 13, 0);
    }
    close(cfd);
}

void* smtp(void* arg)
{
    int opt = 1;
    int cfd, sfd;
    int res;
    char rBuf[100]="";
    char sBuf[100]="";
    struct sockaddr_in svr_addr, c_addr;
    int sLen, cLen;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd >= 0)
    {
        printf("socket OK\n");
    }

    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt\n");
        exit(EXIT_FAILURE);
    }

    sLen = sizeof(svr_addr);
    bzero(&svr_addr, sLen);
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(2525);
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sfd, (struct sockaddr*)&svr_addr, sLen);
    if(res == 0)
    {
        printf("bind OK\n");
    }
    res = listen(sfd, 5);
    if(res >= 0)
    {
        printf("设置监听成功\n");
    }

    cLen = sizeof(c_addr);
    bzero(&c_addr, cLen);

    printf("准备连接\n");

    cfd = accept(sfd, (struct sockaddr*)&c_addr, &cLen);
    printf("smtp服务器为您服务\n");
    if(cfd > 0 || cfd == 0)
    {
        char ipaddr[128]="";
        inet_ntop(AF_INET, &c_addr.sin_addr, ipaddr, sizeof(ipaddr));
        printf("connect from %s\n", ipaddr);
    }

    connection(cfd);

    close(sfd);
    pthread_detach(pthread_self());
    pthread_exit(NULL);
}

void* pop3(void* arg)
{
    return NULL;
}

int main()
{
    int pid;
    int opt = 1;
    int num = 0;
    pthread_t pt_smtp, pt_pop3;
    signal(SIGCHLD, doChild);
    int sfd, cfd;
    struct sockaddr_in svr_addr, c_addr;
    int sLen, cLen;
    int res;
    char rBuf[100]="";
    char sBuf[100]="";

    sLen = sizeof(svr_addr);
    cLen = sizeof(c_addr);

    bzero(&svr_addr, sLen);
    bzero(&c_addr, cLen);

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd >= 0)
    {
        printf("socket OK\n");
    }
    if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt\n");
        exit(EXIT_FAILURE);
    }

    svr_addr.sin_family = AF_INET;
    svr_addr.sin_port = htons(8848);
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    res = bind(sfd, (struct sockaddr*)&svr_addr, sLen);
    if(res == 0)
    {
        printf("bind OK\n");
    }
    res = listen(sfd, 5);
    if(res > 0)
    {
        printf("监听成功\n");
    }

    sem_init(&sem, 0, 0);

    while(1)
    {
        printf("父进程等待新的客户端\n");
        cfd = accept(sfd, (struct sockaddr*)&c_addr, &cLen);
        if(cfd < 0)
        {
            printf("连接失败\n");
            continue;
        }
        pid = fork();
        printf("创建新进程%d 为客户端提供服务\n", num++);
        if(pid == 0)
        {
            close(sfd);
            int i;

            pthread_create(&pt_smtp, NULL, smtp, NULL);
            pthread_create(&pt_pop3, NULL, pop3, NULL);

            for(i = 0; i < 2; i++) {
                sem_post(&sem);
            }

            for(i = 0; i < 2; i++) {
                sem_wait(&sem);
            }

            sem_destroy(&sem);
            close(cfd);
            exit(0);
        }
    }
    close(sfd);
    return 0;
}    
