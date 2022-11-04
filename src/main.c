#include "rest.h"

#include "array.h"
#include "http.h"
#include "util.h"
#include "router.h"
#include "path.h"
#include "context.h"

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <stdarg.h>

int main() {
	run();
	return EXIT_SUCCESS;
}

void *handler (void *arg) {
	route_context_t *context = arg;

	printf("MATCH! METHOD: %s, PATH: %s\n", context->method, context->path);
	return NULL;
}

int run() {
	router_t *router = router_init(NULL, NULL);
	router_register(router, collect_methods("GET", NULL), PATH_ROOT, handler);

	int server_fd;
	int new_socket;

	struct sockaddr_in address;
	int addrlen = sizeof(address);

	// Create socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );

	memset(address.sin_zero, NULL_TERM, sizeof address.sin_zero);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(server_fd, 10) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	printf("Listening on port %i...\n", PORT);

  while(1) {
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		char recv_buffer[1024];

		recvfrom(
			new_socket,
			(char *)recv_buffer,
			sizeof(recv_buffer), 0,
			(struct sockaddr *)&address,
			&addrlen
		);

		printf("BUFFER: %s\n", recv_buffer);

		struct request r = build_request(recv_buffer);

		router_run(router, r.method, r.url);

		buffer_t *response = build_response(
			OK,
			"text/plain",
			"Hello world!",
			"X-Powered-By: rest-c",
			NULL
		);

		printf(response->state);

		write(new_socket, response->state, response->len);
		buffer_free(response);

		close(new_socket);
	}

	return EXIT_SUCCESS;
}
