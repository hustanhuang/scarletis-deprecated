#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <arpa/inet.h>

#include "def.h"
#include "log.h"
#include "session.h"

int Register(int kq, int fd) {
    struct kevent changes[1];
	EV_SET(&changes[0], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);

	int ret = kevent(kq, changes, 1, NULL, 0, NULL);
	if (ret == -1)
		return 0;

	return 1;
}

void Accept(int kq, int connSize, int listen_fd) {
	for (int i = 0; i < connSize; i++) {
		int client;
        if ( (client = accept(listen_fd, NULL, NULL)) < 0)
            s_err("accept");
		if (Register(kq, client) == 0)
            s_err("register");
        s_log("Connection Established");
	}
}

int main(int argc, char* argv[]) {
    int listen_fd;
    if ( (listen_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        s_err("retrieve socket");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SCAR_PORT);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        s_err("bind address");

    if (listen(listen_fd, 16) < 0)
        s_err("listening");

    int kq = kqueue();
    if (Register(kq, listen_fd) == 0)
        s_err("register");

    fprintf(stdout, welcome, SCAR_PORT);

    struct kevent events[MAX_EVENT_COUNT];
    for ( ; ; ) {
        int nevents = 0;
        if ( (nevents = kevent(kq, NULL, 0, events, MAX_EVENT_COUNT, NULL)) < 0)
            s_err("kevent");

        for (int i = 0; i != nevents; ++i) {
            int cli_fd =    events[i].ident,
                data =      events[i].data;

            if (cli_fd == listen_fd)
                Accept(kq, data, listen_fd);
            else
                Session(cli_fd, data);
        }
    }

    exit(EXIT_SUCCESS);
}
