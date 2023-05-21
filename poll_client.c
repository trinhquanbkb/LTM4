#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <poll.h>

int main(int argc, char *argv[])
{
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int portNum = atoi(argv[1]);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(portNum);

    if (connect(client, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("connect() failed");
        return 1;
    }

    struct pollfd fds[2];

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = client;
    fds[1].events = POLLIN;

    char buf[256];

    while (1)
    {
        int ret = poll(fds, 2, -1);
        if (ret < 0)
        {
            perror("poll() failed");
            break;
        }

        if (ret == 0)
        {
            printf("Timed out.\n");
            continue;
        }

        printf("ret = %d\n", ret);

        if (fds[0].revents & POLLIN)
        {
            fgets(buf, sizeof(buf), stdin);
            send(client, buf, strlen(buf), 0);
        }

        if (fds[1].revents & POLLIN)
        {
            ret = recv(client, buf, sizeof(buf), 0);
            if (ret <= 0)
            {
                break;
            }
            buf[ret] = 0;
            printf("Received: %s\n", buf);
        }
    }

    // Ket thuc, dong socket
    close(client);

    return 0;
}