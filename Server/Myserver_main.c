#include "common.h"
#include "Myserver.h"

//#define BUFFER_SIZE 1024
//char helo[100]="";

sem_t sem;
//回收进程
void doChild(int No)
{
    if(No==SIGCHLD)
    {
        while(waitpid(-1,NULL,WNOHANG)==0);
        printf("回收子进程\n");
    }
}
//线程函数入口
void*smtp(void*arg)
{
    printf("new thread\n");
    int opt=1;
    int cfd,sfd;
    int res;
   // cfd=(int)arg; 
    char rBuf[100]="";
    char sBuf[100]="";
    struct sockaddr_in svr_addr,c_addr;
    int sLen,cLen;

    //socket
    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd>=0)
    {
        printf("socket OK\n");
    }

    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))){
        perror("setsockopt\n");
        exit(EXIT_FAILURE);
    }

    sLen=sizeof(svr_addr);
    bzero(&svr_addr,sLen);
    svr_addr.sin_family=AF_INET;
    svr_addr.sin_port=htons(2525);
    svr_addr.sin_addr.s_addr=INADDR_ANY;
    //bind
    res=bind(sfd,(struct sockaddr*)&svr_addr,sLen);
    if(res==0)
    {
        printf("bind OK\n");
    }
    //listen
    res=listen(sfd,5);
    if(res>=0)
    {
        printf("设置监听成功\n");
    }


    cLen=sizeof(c_addr);
    bzero(&c_addr,cLen);

    printf("准备连接\n");

    //accept
    cfd=accept(sfd,(struct sockaddr*)&c_addr,&cLen);
    printf("smtp服务器为您服务\n");
    if(cfd>0||cfd==0)
    {
        char ipaddr[128]="";
        inet_ntop(AF_INET,&c_addr.sin_addr,ipaddr,sizeof(ipaddr));
        printf("connect from %s\n",ipaddr);
    }

    connection(cfd);


    close(sfd);
    //分离线程
    pthread_detach(pthread_self());
    pthread_exit(NULL);

}



//pop3



void*pop3(void*arg)
{
    printf("new thread pop3\n");
    int opt=1;
    int cfd,sfd;
    int res;
    char rBuf[100]="";
    char sBuf[100]="";
    struct sockaddr_in svr_addr,c_addr;
    int sLen,cLen;

    //socket
    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd>=0)
    {
        printf("socket OK\n");
    }

    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))){
        perror("setsockopt\n");
        exit(EXIT_FAILURE);
    }

    sLen=sizeof(svr_addr);
    bzero(&svr_addr,sLen);
    svr_addr.sin_family=AF_INET;
    svr_addr.sin_port=htons(9950);
    svr_addr.sin_addr.s_addr=INADDR_ANY;
    //bind
    res=bind(sfd,(struct sockaddr*)&svr_addr,sLen);
    if(res==0)
    {
        printf("bind OK\n");
    }
    //listen
    res=listen(sfd,5);
    if(res>=0)
    {
        printf("设置监听成功\n");
    }


    cLen=sizeof(c_addr);
    bzero(&c_addr,cLen);

    printf("准备连接\n");

    //accept
    cfd=accept(sfd,(struct sockaddr*)&c_addr,&cLen);
    printf("pop3服务器为您服务\n");
    if(cfd>0||cfd==0)
    {
        char ipaddr[128]="";
        inet_ntop(AF_INET,&c_addr.sin_addr,ipaddr,sizeof(ipaddr));
        printf("connect from %s\n",ipaddr);
    }

        POP3_connection(cfd);

    close(sfd);
    return 0;

}

int main()
{                    

    int pid;
    int opt=1;
    int choose;
    int num=0;//进程数
    //线程id
    pthread_t pt_smtp,pt_pop3;
    //注册信号
    signal(SIGCHLD,doChild);
    //创建文件描述符号
    int sfd,cfd;
    struct sockaddr_in svr_addr,c_addr;
    int sLen,cLen;
    int res;
    char rBuf[100]="";
    char sBuf[100]="";
    
    sLen=sizeof(svr_addr);
    cLen=sizeof(c_addr);

    bzero(&svr_addr,sLen);
    bzero(&c_addr,cLen);


    //socket
    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd>=0)
    {
        printf("socket OK\n");
    }
    //可以即使套接字还处于 TIME_WAIT 状态，也能够立即重新绑定到相同的地址和端口，避免了因地址和端口被占用而无法立即重启服务器的问题。
    if(setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))){
        perror("setsockopt\n");
        exit(EXIT_FAILURE);
    }
    



    //地址绑定
    svr_addr.sin_family=AF_INET;
    svr_addr.sin_port=htons(8848);
    svr_addr.sin_addr.s_addr=INADDR_ANY;
    //bind
    res=bind(sfd,(struct sockaddr*)&svr_addr,sLen);
    if(res==0)
    {
        printf("bind OK\n");
    }
    //listen
    res=listen(sfd,5);
    if(res>0)
    {
        printf("监听成功\n");
    }
    

    sem_init(&sem, 0, 0);
    //init_mail_storage();

    //accept
    while(1)
    {
        printf("父进程等待新的客户端\n");
        cfd=accept(sfd,(struct sockaddr*)&c_addr,&cLen);
        if(cfd<0)
        {
            printf("连接失败\n");
            continue;
        }
        printf("创建新进程%d 为客户端提供服务\n",num++); 
        pid=fork();
        if(pid==0)
        {
            close(sfd);
            int i;
            
            pthread_create(&pt_smtp,NULL,smtp,NULL);
            pthread_create(&pt_pop3,NULL,pop3,NULL);


            for(i=0;i<2;i++)
            {
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
