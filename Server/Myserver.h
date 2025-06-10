#ifndef _MY_SERVER_H_
#define _MY_SERVER_H_
#include "common.h"
#define BUFFER_SIZE 1024

int validate_email(const char*email);
int SMTP_command(int cfd,const char*prefix,const char*suc,const char*err);
int mail_from(int cfd,char*sender);
int rcpt_to(int cfd,char*receiver);
void store(int cfd,const char*sender,const char*receiver);
void connection(int cfd);
int User(int cfd,const char* buf);
int List(int cfd,char *prefix);
int Quit(int cfd,const char* buf);
void Server_load(int cfd);
void POP3_connection(int cfd);
int Pass(int cfd, const char* buf, const char* u_email);

#endif
