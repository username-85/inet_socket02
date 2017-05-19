#include "util.h"
#include "common.h"

#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define SRV_PORT_MIN 4322
#define SRV_PORT_MAX 5000

void process_request(int sfd);
void sigchld_handler(int unused);

int main(void)
{
	struct sigaction sa;
	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	unsigned port = SRV_PORT_MIN;

	socklen_t srv_addrlen = 0;
	int srv_sfd = inet_listen(PORT_SRV, BACKLOG, &srv_addrlen);
	if (srv_sfd < 0) {
		fprintf(stderr, "could not create socket\n");
		exit(EXIT_FAILURE);
	}

	printf("server: waiting for connections...\n");
	struct sockaddr_storage client_addr;
	while(1) {

		socklen_t ca_size = sizeof(client_addr);
		int client_fd = accept(srv_sfd, (struct sockaddr *)&client_addr,
		                       &ca_size);
		if (client_fd == -1) {
			perror("accept");
			continue;
		}

		char addr_str[INET6_ADDRSTRLEN + 10] = {0};
		inet_addr_str((struct sockaddr *)&client_addr, ca_size,
		              addr_str, sizeof(addr_str));
		printf("server: got connection from %s\n", addr_str);

		char pbuf[MAXDSIZE] = {0};
		snprintf(pbuf, sizeof(pbuf), "%d", port);

		socklen_t addrlen = 0;
		int req_sfd = inet_listen(pbuf, BACKLOG, &addrlen);
		if (req_sfd < 0) {
			fprintf(stderr, "could not create req_sfd\n");
			break;
		} else {
			if (send(client_fd, pbuf, sizeof(pbuf), 0) == -1) {
				perror("send");
			}

			port++;
			if (port > SRV_PORT_MAX)
				port = SRV_PORT_MIN;
		}

		switch(fork()) {
		case -1:
			perror("fork");
			break;
		case 0:
			close(srv_sfd);
			close(client_fd);
			process_request(req_sfd);
			_exit(0);
			break;
		default:
			break;
		}

		close(req_sfd);
		close(client_fd);
	}
	close(srv_sfd);

	exit(EXIT_SUCCESS);
}

void process_request(int sfd)
{
	pid_t pid = getpid();
	printf("server: process %d waiting for connection\n", pid);

	struct sockaddr_storage client_addr;
	while(1) {
		socklen_t ca_size = sizeof(client_addr);
		int client_fd = accept(sfd, (struct sockaddr *)&client_addr,
		                       &ca_size);
		if (client_fd == -1) {
			perror("accept");
			break;
		}

		char addr_str[INET6_ADDRSTRLEN + 10] = {0};
		inet_addr_str((struct sockaddr *)&client_addr, ca_size,
		              addr_str, sizeof(addr_str));
		printf("server: process %d got connection from %s\n",
		       pid, addr_str);

		char buf[MAXDSIZE] = {0};
		int numbytes = recv(client_fd, buf, MAXDSIZE - 1, 0);
		if (numbytes == -1) {
			perror("recv");
			break;
		}
		buf[numbytes] = '\0';
		printf("server: process %d got message from %s: %s\n",
		       pid, addr_str, buf);

		if (send(client_fd, MSG_SRV, sizeof(MSG_SRV), 0) == -1)
			perror("send");

		break;
	}
	close(sfd);
}

void sigchld_handler(int unused)
{
	(void)unused;

	// waitpid() might overwrite errno
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

