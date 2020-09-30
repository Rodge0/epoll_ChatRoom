#include "utility.h"

#define error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

/**
 * TCP Server
 * 1.use socket() to create TCP socket sockfd
 * 2.bind sockfd to a local address and port (bind())
 * 3.set sockfd to listen mode, ready for recving client's request (listen())
 * 4.waiting for client's request, accept request when it comes, return a new socket (accept())
 * 5.use above new socket to communicate with client (write()/send() or send()/recv())
 * 6.back, waiting for another client's request
 * 7.close socket
 */
int main(int argc, char *argv[]) {
  /**
   * 1.create TCP socket
   * param1: address family -> IPv4
   * param2: transmission type -> sequenced,reliable,bidirectional,connetion-mode byte streams, and may provide a transmission mechanism for out-of-band data
   * param3: transmission protocol -> TCP protocol, use default protocol when set it 0
   */
  int listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenfd < 0) {
    error("create socket error");
  }
  printf("listen socket created\n");

  // address multiplexing
  int on = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) <0) {
    error("setsockopt error");
  }

  struct sockaddr_in serverAddr;
  serverAddr.sin_family = PF_INET;
  serverAddr.sin_port = htons(SERVER_PORT); // convert host byte order to network byte order
  serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

  // bind address
  if (bind(listenfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
    error("bind error");
  }

  // SOMAXCONN is the max length of the listen queue for every port defined by OS
  // listen socket, SOMAXCONN -> backlog will be a maximun reasonable value
  if (listen(listenfd, SOMAXCONN) < 0) {
    error("listen error");
  }
  printf("Start to listen: %s\n", SERVER_IP);

  // Event-driven programming.
  // create epoll instance in kernel, informe the kernel of the number of file descriptors that the caller expected to add to the epoll instance. The kernel used this information as a hint for the amount of space to initially allocate in internal data structures describing events.
  int epfd = epoll_create(EPOLL_SIZE);
  if (epfd <0) {
    error("epoll_create error");
  }
  printf("epoll instance created, epollfd = %d\n", epfd);
  static struct epoll_event events[EPOLL_SIZE];
  // add events to kernel events table
  addfd(epfd, listenfd, true);

  // main process
  while(1) {
    // the number of ready events
    // epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
    // timeout : set -1 means epoll_wait() to block indefinitely, set 0 to return imediately, even if no events are availabe
    int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
    if (epoll_events_count < 0) {
      perror("epoll failure, epoll_wait error");
      break;
    }

    printf("\nepoll_events_count = %d\n", epoll_events_count);

    // handle these epoll_events_count ready events
    for (int i =0; i <epoll_events_count; ++i) {
      int sockfd = events[i].data.fd;
      // connection request of new client
      if (sockfd == listenfd) {
        struct sockaddr_in client_address;
	socklen_t client_addrLength = sizeof(struct sockaddr_in);
	int clientfd = accept(listenfd, (struct sockaddr*) &client_address, &client_addrLength);

	// inet_ntoa : convert 32 bits ip to ip address of format a.b.c.d
	// ntohs : convert network byte order to host byte order, s-short,h-host
	printf("Client connection from: %s : %d, clientfd = %d\n",
			inet_ntoa(client_address.sin_addr),
			ntohs(client_address.sin_port),
			clientfd);
	
	addfd(epfd, clientfd, true);

	// save connection in list
	clients_list.push_back(clientfd);
	printf("Add new clientfd = %d to epoll\n", clientfd);
	printf("Now there are %d clients in the chat room\n", (int)clients_list.size());

	// send welcome message to new client
	printf("send welcome message\n");
	char message[BUF_SIZE];
	bzero(message, BUF_SIZE);
	sprintf(message, SERVER_WELCOME, clientfd); //??
	int ret = send(clientfd, message, BUF_SIZE, 0);
	if (ret < 0) {
	  error("send welcome message to clientfd error");
	}
      } else {
	printf("Call...............................sendBroadcastmessage\n");
        int ret = sendBroadcastmessage(sockfd);
	if (ret < 0){
	  error("send broadcast message error");
	}
      }
    }
  }


  close(listenfd);
  close(epfd);

  return 0;
}
