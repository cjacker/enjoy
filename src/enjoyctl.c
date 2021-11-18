#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
void usage() {
    printf("enjoyctl - suspend or resume enjoy daemon.\n");
    printf("usage: enjoyctl <device> [suspend|resume|toggle]\n");
}

int main(int argc, char **argv) {
    if(argc != 3) {
        usage();
        exit(EXIT_FAILURE);
    }

    char *device = argv[1];
    char *action = argv[2];
    if(!strstr("suspend resume toggle", action)) {
        usage();
        exit(EXIT_FAILURE);
    }

    int ret;
    char buff[8192];

    int fd;
    struct sockaddr_un addr;
    if ((fd = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    char sock_path[256];
    sprintf(sock_path, "/tmp/enjoy_%s.socket", device);

    if(access(sock_path, W_OK) != 0) {
        fprintf(stderr, "wrong device name: %s\n", device);
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, sock_path);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        exit(1);
    }

    if (send(fd, action, strlen(action)+1, 0) == -1) {
        perror("send");
        exit(1);
    }

    close(fd);
    return EXIT_SUCCESS;
}
