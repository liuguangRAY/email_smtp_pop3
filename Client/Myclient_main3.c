#include "common.h"
#define BUFFER_SIZE 1024

//用于检测服务器返回值
int command(int sfd, const char* cmd, const char* res)
{
    send(sfd, cmd, strlen(cmd), 0);
    char buf[BUFFER_SIZE];
    int rec = recv(sfd, buf, sizeof(buf) - 1, 0);
    buf[rec] = '\0';
    printf("%s\n", buf);
    if (strncmp(buf, res, strlen(res)) == 0) 
    {
        return 1;
    }
    return 0;
}
void Client_load(int sfd)
{
    char file_addr[200] = "./File/";
    char sBuf[1024] = "";
    char rBuf[1024] = "";

    printf("请输入想要下载的文件名: ");
    if(fgets(sBuf, sizeof(sBuf), stdin) == NULL) {
        printf("读取输入失败\n");
        return;
    }
    sBuf[strcspn(sBuf, "\n")] = 0;

    ssize_t bytes_sent = send(sfd, sBuf, strlen(sBuf), 0);
    if(bytes_sent == -1) {
        perror("发送文件名失败");
        return;
    }

    if(strlen(file_addr) + strlen(sBuf) >= sizeof(file_addr)) {
        printf("文件路径过长\n");
        return;
    }
    strcat(file_addr, sBuf);

    FILE*file_fd = fopen(file_addr,"w+");
    if(file_fd == NULL) {
        perror("文件创建失败");
        return;
    }
   int res=0; 
    while ((res = recv(sfd, rBuf, sizeof(rBuf), 0)) > 0) {
        if (strncmp(rBuf, "!quit!", 6) == 0) break;
        printf("%s",rBuf);
        fwrit(rBuf, 1, res, file_fd);
        bzero(rBuf,sizeof(rBuf));
        if (fflush(file_fd) == EOF) { 
            perror("缓冲区刷新失败");
            fclose(file_fd);
            return ;
        }
    }
    fclose(file_fd);


    


    
}


int main()
{
    while(1)
    {
        char rBuf[100]="";
        int sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd >= 0)
        {
            printf("socket OK\n");
        }
        struct sockaddr_in svr_addr;
        svr_addr.sin_family = AF_INET;
        svr_addr.sin_port = htons(8848);
        svr_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        int sLen = sizeof(svr_addr);

        int res = connect(sfd, (struct sockaddr*)&svr_addr, sLen);
        if (res == 0)
        {
            printf("主服务器连接服务器成功\n");
        }

        printf("*****请选择服务类型*****\n");
        printf("1.发送邮件\n");
        printf("2.接收邮件\n");
        printf("3.退出\n");

        scanf("%s",rBuf);
        getchar();

        if(strcmp(rBuf,"1")==0)
        {

            sfd=socket(AF_INET,SOCK_STREAM,0);
            if(sfd>=0)
            {
                printf("sock OK\n");
            }
            struct sockaddr_in svr_addr1;
            sLen=sizeof(svr_addr1);
            bzero(&svr_addr,sLen);

            svr_addr1.sin_family=AF_INET;
            svr_addr1.sin_port=htons(2525);
            svr_addr1.sin_addr.s_addr=inet_addr("127.0.0.1");
            res=connect(sfd,(struct sockaddr*)&svr_addr1,sLen);
            if(res==0)
            {
                printf("smtp服务器连接成功\n");
            }


            char buf[BUFFER_SIZE];
            int rec = recv(sfd, buf, sizeof(buf) - 1, 0);
            buf[rec] = '\0';
            if (strncmp(buf, "OK SMTP service ready", strlen("OK SMTP service ready")) == 0)
            {
                printf("%s\n", buf);
            }
            while(1)
            {

                //菜单选择
                printf("*****请选择服务类型*****\n");
                printf("*****1.发送邮件*****\n");
                printf("*****2.退出*****\n");
                /*
                   char choose[20];
                   scanf("%s",choose);
                   getchar();
                   */

                char choose[20] = {0};
                if (fgets(choose, sizeof(choose), stdin) == NULL) {
                    printf("输入错误\n");
                    continue;
                }
                choose[strcspn(choose, "\n")] = '\0'; // 移除换行符

                send(sfd,choose,strlen(choose),0);
                
                char arr[100];
                recv(sfd,arr,sizeof(arr),0);

                if(strcmp(choose,"1")==0)
                {


                    char helo_name[100];
                    printf("请输入服务器域名：");
                    if (fgets(helo_name, sizeof(helo_name), stdin) != NULL)
                    {
                        helo_name[strcspn(helo_name, "\n")] = '\0';
                    }
                    char helo[BUFFER_SIZE];
                    snprintf(helo, sizeof(helo), "HELO %s\r\n", helo_name);
                    if (!command(sfd, helo, "250")) 
                    {
                        close(sfd);
                        return 1;
                    }
                    char sender[256];
                    printf("输入发件人邮箱: ");
                    fgets(sender, sizeof(sender), stdin);
                    sender[strcspn(sender, "\n")] = '\0';//取\n之前部分
                    strcat(sender,helo_name);
                    char mail_from_cmd[256];
                    snprintf(mail_from_cmd, sizeof(mail_from_cmd), "MAIL FROM:<%s>\r\n", sender); 
                    if (!command(sfd, mail_from_cmd, "250")) {
                        close(sfd); 
                        return 1;
                    }
                    //处理RCPT TO命令
                    char receiver[256];
                    printf("输入收件人邮箱: ");
                    fgets(receiver, sizeof(receiver), stdin);
                    receiver[strcspn(receiver, "\n")] = '\0';
                    strcat(receiver,helo_name);
                    char rcpt_to_cmd[256];
                    snprintf(rcpt_to_cmd, sizeof(rcpt_to_cmd), "RCPT TO:<%s>\r\n", receiver);
                    if (!command(sfd, rcpt_to_cmd, "250")) {
                        close(sfd); 
                        return 1;
                    }
                    if (!command(sfd, "DATA\r\n", "354")) {
                        close(sfd); 
                        return 1;
                    }





                    char email_content[BUFFER_SIZE];
                    printf("\n");
                    while (fgets(email_content, sizeof(email_content), stdin) != NULL)
                    {

                        size_t len = strlen(email_content);
                        if (len > 0 && email_content[len - 1] == '\n') {
                            email_content[len - 1] = '\0';
                            if (len > 1 && email_content[len - 2] == '\r') {
                                email_content[len - 2] = '\0';
                            }
                        }

                        if (strcmp(email_content, ".") == 0) {
                            break;
                        }                                                                         // 恢复换行符并发送
                        strncat(email_content, "\r\n", sizeof(email_content) - strlen(email_content) - 1);
                        send(sfd, email_content, strlen(email_content), 0);
                    }
                    send(sfd, ".\r\n", strlen(".\r\n"), 0);
                    
                    char arr[100]; 
                    recv(sfd,arr,sizeof(arr),0);
                
                }

                else if(strcmp(choose,"2")==0)
                {
                    printf("执行退出\n");
                    send(sfd,"QUIT",4,0);
                    printf("发送成功\n");
                    recv(sfd,rBuf,sizeof(rBuf),0);
                    close(sfd);
                    break;
                }
            }
        }

        else if(strcmp(rBuf,"2")==0)
        {
            int sfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sfd >= 0)
            {
                printf("socket OK\n");
            }


            struct sockaddr_in svr_addr;
            svr_addr.sin_family = AF_INET;
            svr_addr.sin_port = htons(9950);
            svr_addr.sin_addr.s_addr = inet_addr("127.0.0.1");


            int sLen = sizeof(svr_addr);
            int res = connect(sfd, (struct sockaddr*)&svr_addr, sLen);
            if (res == 0)
            {
                printf("连接服务器成功\n");
            }
            char buf[BUFFER_SIZE];

            int rec=recv(sfd,buf,sizeof(buf)-1,0);
            buf[rec]='\0';
            if(strncmp(buf, "OK POP3 service ready", strlen("OK POP3 service ready"))==0){
                printf("%s\n", buf);
            }
            bzero(buf,sizeof(buf));
            while(1)
            {
                printf("*****请选择服务*****\n");
                printf("*****1.下载邮件*****\n");
                printf("*****2.退出*****\n");
                
                char choose3[20]="";
                scanf("%s",choose3);
                getchar();
                send(sfd,choose3,strlen(choose3),0);
                
                char p[100];
                recv(sfd,p,sizeof(p),0);

                if(strcmp(choose3,"1")==0)
                {
                    char u_email[100];
                    printf("请输入邮箱地址:");
                    fgets(u_email,sizeof(u_email),stdin);
                    u_email[strcspn(u_email, "\n")] = '\0';
                    char user_cmd[BUFFER_SIZE];
                    snprintf(user_cmd, sizeof(user_cmd), "USER %s", u_email);
                    if (!command(sfd, user_cmd, "251 ")) {
                        close(sfd);
                        return 1;
                    }
                    //PASS命令
                    char password[100];
                    printf("请输入密码:");
                    fgets(password, sizeof(password), stdin);
                    password[strcspn(password, "\n")] = '\0';
                    char pass_cmd[BUFFER_SIZE];
                    snprintf(pass_cmd, sizeof(pass_cmd), "PASS %s\r\n", password);
                    if (!command(sfd, pass_cmd, "251 ")) {
                        close(sfd);
                        return 1;
                    }
                    //LIST命令

                    printf("列出邮件表单\n");
                    send(sfd,"LIST",strlen("LIST"),0);
                    rec=recv(sfd,buf,sizeof(buf)-1,0);
                    printf("%s",buf);

                    //接收邮件内容
                    printf("输入需要下载的文件名\n");
                    Client_load(sfd);
                    
                }
                else
                {
                    printf("执行退出\n");
                    //QUIT命令
                    if(!command(sfd,"QUIT","BYE")){
                        close(sfd);
                        return 1;
                    }
                    close(sfd);
                    break;
                }
            }
        }
        else
        {
            close(sfd);
            printf("客户端退出\n");
            break;
        }

    }
    return 0;
}  
