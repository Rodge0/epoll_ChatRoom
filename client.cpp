#include "utility.h"

#define error(msg) \
	do {perror(msg); exit(EXIT_FAILURE); } while (0)

/**
 * TCP client communication
 * 1.create socket
 * 2.use connect() to create a connection with server
 * 3.use write()/send() or send()/recv() to communicate with server
 * 4.use close() to close connection
 */
int main(int argc, char *argv[]) {
  /**
   * 1. create socket
   */
  int clientfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (clientfd < 0) { error("create socket error"); }

  // create struct sockaddr with server ip and port
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(SERVER_PORT);
  serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

  // 2.connect to server
  if (connect(clientfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
    error("connect to server error");
  }

  // create pipe, pipefd[0] use for father process to read, pipefd[1] for sub-process to write
  // return array pipefd of file descriptor, pipefd[0] is pipe read, pipefd[1] is pipe write
  // success : return 0, otherwise :return -1
  // must call pipe() when you call fork()
  int pipefd[2];
  if (pipe(pipefd) < 0) { error("pipe error"); }

  /**
   * use of epoll
   * 1.call epoll_create() to create event table in Linux kernel
   * 2.add file descriptor to created event table
   * 3.in main loop, call epoll_wait() to wait return ready file descriptor event table
   * 4.handle ready file descriptor event table separately
   */
  // create epoll
  int epfd = epoll_create(EPOLL_SIZE);
  if (epfd < 0) { error("create epoll error"); }

  static struct epoll_event events[2];

  // add clientfd and pipefd in kernel event table
  addfd(epfd, clientfd, true);
  addfd(epfd, pipefd[0], true);

  // whether the client work normally
  bool isClientWork = true;

  // chat message buffer
  char message[BUF_SIZE];

  // fork, success : return 0, failure : return -1
  int pid = fork();
  if (pid < 0) {
    error("fork error");
  } else if (pid == 0) { // sub-process
    // close pipe read point because sub-process is responsible for write pipe
    close(pipefd[0]);
    printf("Please input 'exit' or 'EXIT' to exit the chat room!\n");

    while(isClientWork) {
      bzero(&message, BUF_SIZE);
      fgets(message, BUF_SIZE, stdin); // at most read BUF_SIZE-1 characters from stdin to message and add '\0' to the end
     // exit from chat room
     // strncasecmp(const char *s1, const char *s2, size_t n); performs a byte-to-byte comparison of thestring s1 and s2, ignoring the case of the characters
      if (strncasecmp(message, EXIT, strlen(EXIT)) == 0) {
        isClientWork = 0;
      } else {
        // sub-process write message to pipe
        if ( write(pipefd[1], message, strlen(message)-1) <0) {
	  error("write message to pipe error");		
	}
      }
    }
  } else { // father process
    // close the write pipe end because father process is responsible for read pipe data
    close(pipefd[1]);

    // main loop(epoll_wait)
    while (isClientWork) {
      // wait for events, return the amount of ready events for handle
      int epoll_events_count = epoll_wait(epfd, events, 2, -1);

      printf("..............%d events to handle.............\n", epoll_events_count);
      // handle ready events
      for (int i =0; i <epoll_events_count; ++i) {
        bzero(&message, BUF_SIZE);

	// receive message from server
	if (events[i].data.fd == clientfd) {
	  int ret = recv(clientfd, message, BUF_SIZE, 0);

	  // ret = 0 -> server closed socket
	  if (0 == ret) {
	    printf("Server closed the connection: %d\n", clientfd);
	    close(clientfd);
	    isClientWork = 0;
	  }
	  else printf("%s\n", message);
	}
	else {
	  // father process read data from pipe
	  // 
	  int ret = read(events[i].data.fd, message, BUF_SIZE);

	  if (0 == ret) {
	    isClientWork = 0;
	  } else {
	    send(clientfd, message, BUF_SIZE, 0);
	  }

	}
      }//for
    }//while
  }

  if (pid) {
    close(pipefd[0]);
    close(clientfd);
  } else {
    close(pipefd[1]);
  }

  return 0;
}



























