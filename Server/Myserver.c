#include "Myserver.h"
char helo[100]="";
int validate_email(const char* email)//数据库查询
{
    MYSQL* conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "127.0.0.1", "root", "123456", "db_Mailserver", 0, NULL, 0))//连接数据库
    {
        fprintf(stderr, "数据库连接失败: %s\n", mysql_error(conn));
        return 0;
    }
  //  printf("数据库查询中\n");
    char query[256];
    snprintf(query, sizeof(query), "SELECT m_mail FROM tb_users WHERE m_mail='%s'", email);

    if (mysql_query(conn, query)) {
        fprintf(stderr, "查询失败: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 0;
    }
    //从查询结果中取出行数
    MYSQL_RES* result = mysql_store_result(conn);
    int exists = (mysql_num_rows(result) > 0);
    mysql_free_result(result);
    mysql_close(conn);
    printf("查询成功\n");
    return exists;
}

//验证域名
int SMTP_command(int cfd,const char*prefix,const char*suc,const char*err)
{
    char buf[BUFFER_SIZE];
    //接收客户端信息
    int bytes=recv(cfd,buf,sizeof(buf)-1,0);
    buf[bytes]='\0';
    //验证协议
    if(strncmp(buf,prefix,strlen(prefix))==0)
    {
        sscanf(buf+strlen("HELO"),"%s",helo);
        printf("指定域名为:%s\n",helo);
        send(cfd,suc,strlen(suc),0);

        return 1;
    }
    else
    {
        send(cfd,err,strlen(err),0);
        return 0;
    }

}
//发送方验证
int mail_from(int cfd, char* sender)
{
    printf("接收发件人\n");
    char buf[BUFFER_SIZE];
    int bytes = recv(cfd, buf, sizeof(buf) - 1, 0);
    buf[bytes]='\0';
    //从buf中按指定格式取出邮箱地址存储在sender中
    if (sscanf(buf, "MAIL FROM:<%[^>]>", sender) != 1) {
        send(cfd, "501参数格式错误\r\n", 15, 0);
        return 0;
    }
    //数据库中查询邮箱地址
    if (!validate_email(sender)) {
        send(cfd, "550邮箱不存在\r\n", 20, 0);
        return 0;
    }
    //向客户端返回确认信息
    send(cfd, "250 OK\r\n", 8, 0);
    return 1;
}
//接收方验证
int rcpt_to(int cfd,char* receiver)
{

    char buf[BUFFER_SIZE];
    int bytes=recv(cfd,buf,sizeof(buf)-1,0);
    buf[bytes]='\0';
    //读取信息存储到receiver中
    if(sscanf(buf,"RCPT TO:<%[^>]>",receiver)!=1)
    {
        send(cfd,"501参数格式错误\r\n",15,0);
        return 0;
    }
    //数据库验证
    if(!validate_email(receiver))
    {
        send(cfd,"550邮箱不存在\r\n",14,0);
        return 0;
    }

    send(cfd,"250 Ok\r\n",8,0);
    return 1;
}

void store(int cfd,const char* sender, const char* receiver) 
{
    unsigned long timeSec;
    timeSec=time(NULL);
    char base_dir[100] = "./";
    char emails_dir[128];
    snprintf(emails_dir, sizeof(emails_dir), "%semails", base_dir);

    if (access(emails_dir, F_OK) == -1) 
    {
        if (mkdir(emails_dir, 0777) == -1) 
        {
            perror("无法创建 emails 文件夹");
            send(cfd, "550 Error\r\n", strlen("550 Error\r\n"), 0);
            return;

        }
    }
    FILE* file;
    char filename[128];
    snprintf(filename, sizeof(filename), "%s/%s_%lu.txt", emails_dir, receiver,timeSec);
    file = fopen(filename, "w");
    if (file == NULL) 
    {
        perror("无法打开文件");
        send(cfd, "550 Error\r\n", strlen("550 Error\r\n"), 0);
        return;
    }

    time_t now=time(NULL);
    char* time_str=ctime(&now);

    fprintf(file,"MAIL FROM:%s\n",sender);
    fprintf(file,"TO:%s\n",receiver);
    fprintf(file,"DATE:%s\n",time_str);
    char buf[BUFFER_SIZE];
    while (1)
    {
        int bytes = recv(cfd, buf, sizeof(buf) - 1, 0);
        if (bytes <= 0) 
        {
            break;
        }
        buf[bytes] = '\0';
        if (strcmp(buf, ".\r\n") == 0 || strcmp(buf, ".\n") == 0) 
        {
            break;
        }
        printf("接收到的邮件内容:%s",buf);
        fwrite(buf, 1, bytes, file);
    }
    fclose(file);
    send(cfd, "250 OK\r\n", strlen("250 OK\r\n"), 0);
    printf("接收完成\n");
}


//smtp主控函数
void connection(int cfd)
{   
    char rBuf[100];
    char sender[256];
    char receiver[256];
    const char*ready="OK SMTP service ready";
    send(cfd,ready,strlen(ready),0);

    while(1)
    {
        int res=recv(cfd,rBuf,sizeof(rBuf)-1,0);
        send(cfd,"OK",2,0);
        rBuf[res]='\0';
        if(strcmp(rBuf,"1")==0)
        {
            //域名验证
            if(!SMTP_command(cfd,"HELO ","250\n","500\n"))
            {
                close(cfd);
                return;
            }
            //发送方验证
            if(!mail_from(cfd,sender))
            {
                close(cfd);
                return;
            }
            //接收方验证
            if(!rcpt_to(cfd,receiver))
            {
                close(cfd);
                return;
            }


            char buf[BUFFER_SIZE];
            int bytes = recv(cfd, buf, sizeof(buf) - 1, 0);
            buf[bytes] = '\0';
            if (strncmp(buf, "DATA", strlen("DATA")) == 0)
            {
                send(cfd, "354 服务器准备接收邮件内容;请以单独的'.'行结束输入\r\n", strlen("354 服务器接收邮件内容;请以单独的'.'行结束输入\r\n"), 0);

                store(cfd,sender,receiver);
            }
            else
            {
                send(cfd, "500 Error\r\n", strlen("500 Error\r\n"), 0);
                close(cfd);
                return;
            }
        }
        else
        {
            printf("1\n");
            char quit_cmd[BUFFER_SIZE];
            int p=recv(cfd,quit_cmd,sizeof(quit_cmd),0);
            if(p<=0)
            {
                printf("fail\n");
            }
            if(strncmp(quit_cmd,"QUIT",4)==0)
            {
                send(cfd,"servic close\r\n",13,0);
            }
            close(cfd);
            break;
        }

    }
}

int User(int cfd,const char* buf){
    char u_email[BUFFER_SIZE];
    if (sscanf(buf, "USER %s", u_email) == 1)//检测是否成功执行sscanf
    {
        if (validate_email(u_email))//去数据库中查找地址是否存在 
        {
            if (send(cfd, "251 ", strlen("251 "), 0) == -1) 
            {
                perror("send");
                return -1;
            }
            return 1;
        } 
        else// 没有查找到
        {
            if (send(cfd, "504 ", strlen("504 "), 0) == -1) 
            {
                perror("send");
                return -1;
            }
            return -1;
        }
    }
    else 
    {
        if (send(cfd, "501 ", strlen("500 "), 0) == -1) {
            perror("send");
            return -1;
        }
        return -1;
    }
}

int Pass(int cfd, const char* buf, const char* u_email) 
{
    char password[BUFFER_SIZE];
    if (sscanf(buf, "PASS %s", password) == 1) {
        MYSQL* conn = mysql_init(NULL);
        if (!mysql_real_connect(conn, "127.0.0.1", "root", "123456", "db_Mailserver", 0, NULL, 0)) {
            fprintf(stderr, "数据库连接失败: %s\n", mysql_error(conn));
            if (send(cfd, "500 ", strlen("500 "), 0) == -1) {
                perror("send");
            }
            return -1;
        }
        char query[256];
        snprintf(query, sizeof(query), "SELECT m_passwd FROM tb_users WHERE m_mail='%s' AND m_passwd='%s'", u_email, password);
        if (mysql_query(conn, query)) {
            fprintf(stderr, "查询失败: %s\n", mysql_error(conn));
            mysql_close(conn);
            if (send(cfd, "500 ", strlen("500 "), 0) == -1) {
                perror("send");
            }
            return -1;
        }
        MYSQL_RES* result = mysql_store_result(conn);
        int valid = (mysql_num_rows(result) > 0);
        mysql_free_result(result);
        mysql_close(conn);
        if (valid) {
            if (send(cfd, "251 ", strlen("251 "), 0) == -1) {
                perror("send");
                return -1;
            }
            return 1;
        }
        else {
            if (send(cfd, "504 ", strlen("504 "), 0) == -1) {
                perror("send");
                return -1;
            }
            return -1;
        }
    }
    else {
        if (send(cfd, "500 ", strlen("500 "), 0) == -1) {
            perror("send");
            return -1;
        }
        return -1;
    }
}



int List(int cfd,char *prefix) {
    int res;
    DIR *dir;
    struct dirent* entry;
    char file_list[50*20]="";
    char rBuf[100]="";

    dir = opendir("./emails");
    if(dir == NULL) {
        printf("打开文件夹失败\n");
        send(cfd, "504", 4, 0);
        return 0;
    } else {
        while((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
                strncat(file_list, entry->d_name, strlen(entry->d_name));
                strncat(file_list, "\n", 1);
            }
        }
        closedir(dir);
        send(cfd, file_list, strlen(file_list), 0);
        return 1;
    }
}



int Quit(int cfd,const char* buf){
    if (strcmp(buf,"QUIT") == 0) {
        if (send(cfd, "BYE", strlen("BYE"), 0) == -1) {
            perror("send");
            return -1;
        }
        return 1;
    } else {
        if (send(cfd, "501 ", strlen("500 "), 0) == -1) {
            perror("send");
        }
        return -1;
    }
}
void Server_load(int cfd)
{
    printf("进入下载函数\n");
    char rBuf[50] = "";
    ssize_t res = recv(cfd, rBuf, sizeof(rBuf) - 1, 0);
    if(res <= 0)
    {
        if(res == -1) perror("recv failed");
        close(cfd);
        return;
    }
    

    printf("打开文件地址\n");
    char addr[100] = "./emails/";
    if(strlen(rBuf) + strlen(addr) >= sizeof(addr)) {
        close(cfd);
        return;
    }
    strcat(addr, rBuf);

    FILE*file_fd =fopen(addr, "r+");
    if(file_fd == NULL)
    {
        perror("Failed to open file");
        close(cfd);
        return;
    }
    printf("打开文件成功\n");


    char sBuf[1024] = "";

    while (fgets(sBuf, sizeof(sBuf), file_fd) != NULL)
    {

        send(cfd,sBuf,strlen(sBuf),0);
        
    }
    sleep(1);
    send(cfd,"!quit!",7,0);
    printf("关闭文件描述符\n");
    fclose(file_fd);
    printf("文件发送完成，保持连接开放\n");
}

void POP3_connection(int cfd){

    const char* ready = "OK POP3 service ready";
    send(cfd, ready, strlen(ready), 0);
    char buf[BUFFER_SIZE];
    int rec;
    char u_email[100]="";
    char rBuf[20];
    while(1)
    {
        int res=recv(cfd,rBuf,sizeof(rBuf)-1,0);

        send(cfd,"OK",2,0);

        if(res<0)
        {
            printf("选择错误\n");
        }
        rBuf[res]='\0';

        if(strcmp(rBuf,"1")==0)
        {
            rec = recv(cfd, buf, sizeof(buf) - 1, 0);
            if (rec <= 0) {
                if (rec == 0) {
                    printf("客户端已断开连接\n");
                } else {
                    perror("recv");
                }
                close(cfd);
                return;
            }
            buf[rec] = '\0';
            if (User(cfd, buf) == -1) {
                close(cfd);
                return;
            }
            strcpy(u_email, strtok(buf + 5, "\r\n"));


            rec = recv(cfd, buf, sizeof(buf) - 1, 0);
            if (rec <= 0) {
                if (rec == 0) {
                    printf("客户端已断开连接\n");
                }
                else {
                    perror("recv");
                }
                close(cfd);
                return;
            }
            buf[rec] = '\0';
            if (Pass(cfd, buf, u_email) == -1) {
                close(cfd);
                return;
            }


            rec = recv(cfd, buf, sizeof(buf) - 1, 0);
            if (rec <= 0) {
                if (rec == 0) {
                    printf("客户端已断开连接\n");
                }
                else {
                    perror("recv");
                }
                close(cfd);
                return;
            }
            char name[100]="";
            strcpy(name,u_email);
            name[strcspn(u_email,"@")]='\0';
            buf[rec] = '\0';
            if (strcmp(buf, "LIST") == 0) {
                List(cfd,u_email);
                if (List(cfd,u_email) == 0) {
                    close(cfd);
                    return;
                }
            }


            Server_load(cfd);
        }

        else if(strcmp(rBuf,"2")==0)
        {
            printf("执行退出\n");
            rec=recv(cfd,buf,sizeof(buf)-1,0);
            if(rec<=0){
                if(rec==0){
                    printf("客户端已断开连接\n");}
                else{perror("recv");}
                close(cfd);
                return;
            }
            buf[rec]='\0';
            if(Quit(cfd,buf)==1){
                close(cfd);
                break;
            }
        }
    }
}

