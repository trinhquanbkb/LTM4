#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5))
    {
        perror("listen() failed");
        return 1;
    }

    struct pollfd fds[64];
    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    char buf[256];
    int users[64];
    int num_users = 0;

    while (1)
    {
        int ret = poll(fds, nfds, -1);
        if (ret < 0)
        {
            perror("poll() failed");
            break;
        }

        if (fds[0].revents & POLLIN)
        {
            int client = accept(listener, NULL, NULL);
            if (nfds == 64)
            {
                // Tu choi ket noi
                close(client);
            }
            else
            {
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;
                nfds++;

                printf("New client connected: %d\n", client);
                char *msg = "Nhap tai khoan vs mat khau (tai_khoan mat_khau):\n";
                send(client, msg, strlen(msg), 0);
            }
        }

        for (int i = 1; i < nfds; i++)
            if (fds[i].revents & POLLIN)
            {
                ret = recv(fds[i].fd, buf, sizeof(buf), 0);
                if (ret <= 0)
                {
                    close(fds[i].fd);
                    // Xoa khoi mang
                    if (i < nfds - 1)
                        fds[i] = fds[nfds - 1];
                    nfds--;
                    i--;
                }
                else
                {
                    buf[ret] = 0;
                    printf("Received from %d: %s\n", fds[i].fd, buf);

                    int client = fds[i].fd;

                    // Kiem tra trang thai dang nhap
                    int j = 0;
                    for (; j < num_users; j++)
                        if (users[j] == client)
                            break;

                    if (j == num_users) // Chua dang nhap
                    {
                        FILE *f = fopen("user.txt", "rb");
                        char line[100];
                        int k = 0;
                        while (fgets(line, 100, f) != NULL)
                        {
                            if (strncmp(line, buf, strlen(buf) - 1) == 0)
                            {
                                k = 1;
                                break;
                            }
                        }
                        if (k)
                        {
                            char *msg = "Dang nhap hoan thanh. Nhap lenh can xu ly:\n";
                            send(client, msg, strlen(msg), 0);

                            users[num_users] = client;
                            num_users++;
                        }
                        else
                        {
                            char *msg = "Nhap sai. Nhap lai.\n";
                            send(client, msg, strlen(msg), 0);
                        }
                        fclose(f);
                    }
                    else // Da dang nhap
                    {
                        ret = system(buf);
                        if (ret == 0)
                        {
                            char *msg = "Hoan Thanh!\n";
                            send(client, msg, strlen(msg), 0);
                        }
                        else
                        {
                            char *msg = "Lỗi khi thực thi lệnh";
                            send(client, msg, strlen(msg), 0);
                        }
                    }
                }
            }
    }

    close(listener);

    return 0;
}