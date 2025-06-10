Myserver:Myserver.o Myserver_main.o
	gcc -g Myserver.o Myserver_main.o -o Myserver -L /usr/include/mysql -lmysqlclient -lpthread
Myserver.o:Myserver.c
	gcc -g -c Myserver.c
Myserver_main.o:Myserver_main.c
	gcc -g -c Myserver_main.c
