#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <limits.h>

struct command_line_option {
#define PORT_NUMBER_MAX_LEN 5
	char port[PORT_NUMBER_MAX_LEN + 1];
	char hostname[HOST_NAME_MAX + 1];
};

static int parse_option(int argc, char *argv[], struct command_line_option *option)
{
	int opt, ret = -1;

	opterr = 0;
	while ((opt = getopt(argc, argv, "h:p:")) != -1) {
		switch (opt) {
		case 'h':
			snprintf(option->hostname,
				sizeof(option->hostname), "%s", optarg);
			break;
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
		return -1;
	}
	if (option->hostname[0] == '\0') {
		return -1;
	}

	ret = 0;
end:
	return ret;
}

int open_client_socket(const char *hostname, const char *port_no)
{
	struct addrinfo hints, *res = NULL;
	int gai_err, err_state = -1;
	int sock = -1;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((gai_err = getaddrinfo(hostname, port_no, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error %s", gai_strerror(gai_err));
		goto end;
	}

	if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
		perror("socket");
		goto end;
	}

	if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
		perror("connect");
		goto end;
	}

	err_state = 0;
end:
	if (res != NULL) {
		freeaddrinfo(res);
	}

	if (err_state == -1) {
		if (sock != -1) {
			close(sock);
			sock = -1;
		}
	}

	return sock;
}

int main(int argc, char *argv[])
{
	struct command_line_option option = {{ 0 }};
	int sock = -1;
	int ret_code = 1;
	char buf[64] = { 0 };

	strcpy(buf, "hello");

	if (parse_option(argc, argv, &option)) {
		fprintf(stderr, "client -h hostname -p port\n");
		goto end;
	}

	signal(SIGPIPE, SIG_IGN);

	if ((sock = open_client_socket(option.hostname, option.port)) == -1) {
		goto end;
	}

	if (send(sock, buf, sizeof(buf), 0) == -1) {
		perror("send");
		goto end;
	}

	ret_code = 0;
end:
	if (sock != -1) {
		close(sock);
	}

	sleep(10);
	return ret_code;
}
