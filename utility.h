#ifndef CHAT_UTILITY_H
#define CHAT_UTILITY_H

#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

// clients_list save all the clients' socket
list<int> clients_list;

/**** macro definition ****/
// server ip and server port
//#define SERVER_IP "127.0.0.1" // local loop ip
//#define SERVER_IP "192.168.1.103"
#define SERVER_IP "192.168.110.129"
#define SERVER_PORT 38888 

// epoll size
#define EPOLL_SIZE 5000

// message buffer size
#define BUF_SIZE 0xFFFF  // 2^16-1

#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is : Client #%d"
#define SERVER_MESSAGE "[%d] said : %s"

// exit
#define EXIT "EXIT"

#define CAUTION "There is only you in the chat room!"

/******* some function ******/
/**
 * set non-blocking -- int fcntl(int fd, int cmd, long arg); 
 * F_GETFD(void) get the file access mode and the file status flags; arg is ignored
 * F_SETFL(int) set the file status flags to the value specified by arg. File access mode (O_RDONLY, O_WRONLY, O_RDWR) and file creation flags (i.e.,O_CREAT, O_EXCL, O_NOCTTY, O_TRUNC) in arg are ignored. On linux this command can change only the O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, and O_NONBLOCK flags
 */
int setnonblocking(int sockfd){
  fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);
  return 0;
}

/**
 * 将文件描述符 fd 添加到 epollfd 标志的内和事件表中，
 * 并注册 EPOLLIN（读） 和 EPOLLET（边缘触发） 事件,
 * 最后将文件描述符设置为非阻塞方式
 * @ param epollfd : epoll句柄
 * @ param fd : file descriptor
 * @ param enable_et : true -> set epoll work in edge triggered mode, otherwise level triggered
 */
void addfd(int epollfd, int fd, bool enable_et) {
  struct epoll_event ev; // 告诉内核需要监听什么事件
  ev.data.fd = fd;
  ev.events = EPOLLIN;   // 数据可读事件
  if (enable_et) {
    ev.events = EPOLLIN | EPOLLET;
  }
  // 对指定描述符执行操作
  // ADD DEL MOD
  epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev);
  setnonblocking(fd);
  printf("fd added to epoll!\n");
}

// 发送广播
int sendBroadcastmessage(int clientfd) {
  char buf[BUF_SIZE];
  char message[BUF_SIZE];
  // void bzero(void *s, int nbyte); use in linux
  // the bzero function places nbyte null bytes in the string s. 
  // This function is used to set all the socket structures with null values.
  // In other OS, your can use #define bzero(a, b) memset(a, 0, b)
  bzero(buf, BUF_SIZE);
  bzero(buf, BUF_SIZE);

  printf("read from client(ClientID = %d)\n", clientfd);
  // copy clientfd's data from buffer with size of BUF_SIZE to buf 
  // success : return acutual size of copyed data
  // failure : return -1 and errno set to indicate the error
  // client close socket : return 0
  int len = recv(clientfd, buf, BUF_SIZE, 0);

  // peer has performed an orderly shutdown
  if (0 == len) {
    close(clientfd);
    clients_list.remove(clientfd);
    printf("ClientID = %d closed.\n now there are %d client in the chat room\n", clientfd, (int)clients_list.size());
  } else if (len > 0) {
    if ( 1 == clients_list.size()) {
      printf("Only one....., message : %s len : %d\n", buf, len);
      send(clientfd, CAUTION, strlen(CAUTION), 0);
      return 0;
    }
    printf("Send message to other client except the client who send the message to server.......\n");
    sprintf(message, SERVER_MESSAGE, clientfd, buf);
    list<int>::iterator it;
    for (it = clients_list.begin(); it != clients_list.end(); ++it) {
      if (*it != clientfd) {
        if (send(*it, message, BUF_SIZE, 0) < 0) {
	  perror("send error");
	  exit(-1);
	}
      }
    }
  }
  return len;
}

#endif // CHAT_UTILITY_H



























