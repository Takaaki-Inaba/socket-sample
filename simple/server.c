#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>

struct command_line_option {
#define PORT_NUMBER_MAX_LEN 5
	char port[PORT_NUMBER_MAX_LEN + 1];
};

static int parse_option(int argc, char *argv[], struct command_line_option *option)
{
	int opt, ret = -1;

	opterr = 0;
	while ((opt = getopt(argc, argv, "p:")) != -1) {
		switch (opt) {
		case 'p':
			snprintf(option->port, sizeof(option->port), "%s", optarg);
			break;
		default:
			goto end;
			break;
		}
	}

	if (optind < argc) {
		goto end;
	}

	if (option->port[0] == '\0') {
		goto end;
	}

	ret = 0;
end:
	return ret;
}

int open_server_socket(const char *port)
{
	int sock = -1, optval, err = -1;
	struct addrinfo hints, *result = NULL, *rp = NULL;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;

	if ((err = getaddrinfo(NULL, port, &hints, &result)) != 0) {
		fprintf(stderr, "getnameinfo %s", gai_strerror(err));
		goto end;
	}

	optval = 1;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1) {
			continue;
		}

		// if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		// 	perror("setsockopt");
		// 	goto end;
		// }

		if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
	}

	if (rp == NULL) {
		fprintf(stderr, "Could not bind socket to any address");
		goto end;
	}

	if (listen(sock, SOMAXCONN) == -1) {
		perror("listen");
		goto end;
	}

	err = 0;
end:
	if (result != NULL) {
		freeaddrinfo(result);
	}

	if (err != 0 && sock != -1) {
		close(sock);
	}

	return sock;
}

int main(int argc, char *argv[])
{
	struct command_line_option option = {{ 0 }};
	int server_socket = -1;
	int connected_socket = -1;
	char buf[64] = { 0 };

	if (parse_option(argc, argv, &option)) {
		fprintf(stderr, "server -p port\n");
		goto end;
	}

	if ((server_socket = open_server_socket(option.port)) == -1) {
		goto end;
	}

	if ((connected_socket = accept(server_socket, NULL, NULL)) == -1) {
		perror("accept");
		goto end;
	}

	if (recv(connected_socket, buf, sizeof(buf), MSG_WAITALL) == -1) {
		perror("recv");
		goto end;
	}
	printf("%s\n", buf);

end:
	if (server_socket != -1) {
		close(server_socket);
	}
	if (connected_socket != -1) {
		close(connected_socket);
	}

	sleep(10);
	return 0;
}

