#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <poll.h>

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

    int users[64];      // Mang socket client da dang nhap
    char *user_ids[64]; // Mang id client da dang nhap
    int num_users = 0;  // So luong client da dang nhap

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
                    {
                        fds[i] = fds[nfds - 1];
                    }
                    nfds--;

                    // int j = 0;
                    // for (; j < num_users; j++)
                    //     if (users[j] == fds[i].fd)
                    //         break;
                    // if (j < num_users - 1)
                    // {
                    //     users[j] = users[num_users - 1];
                    //     strcpy(user_ids[j], user_ids[num_users - 1]);
                    // }
                    // printf("%d %d %d\n", i, j, num_users);
                    // num_users--;
                    // printf("%d %d %d\n", i, j, num_users);
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
                        // Xu ly cu phap lenh dang nhap
                        char cmd[32], id[32], tmp[32];
                        ret = sscanf(buf, "%s%s%s", cmd, id, tmp);
                        if (ret == 2)
                        {
                            if (strcmp(cmd, "client_id:") == 0)
                            {
                                // Kiểm tra xem id có bị trùng với các client đang có không
                                int k = 1;
                                for (int j = 0; j < num_users; j++)
                                {
                                    if (strncmp(user_ids[j], id, strlen(user_ids[j])) == 0)
                                    {
                                        k = 0;
                                        break;
                                    }
                                }

                                if (k)
                                {
                                    char *msg = "Dung cu phap. Hay nhap tin nhan de chuyen tiep.\n";
                                    send(client, msg, strlen(msg), 0);

                                    // Luu vao mang user
                                    users[num_users] = client;
                                    user_ids[num_users] = malloc(strlen(id) + 1);
                                    strcpy(user_ids[num_users], id);
                                    num_users++;
                                }
                                else
                                {
                                    char *msg = "ID da ton tai. Vui long nhap lai :) \n";
                                    send(client, msg, strlen(msg), 0);
                                }
                            }
                            else
                            {
                                char *msg = "Sai cu phap. Hay nhap lai.\n";
                                send(client, msg, strlen(msg), 0);
                            }
                        }
                        else
                        {
                            char *msg = "Sai tham so. Hay nhap lai.\n";
                            send(client, msg, strlen(msg), 0);
                        }
                    }
                    else // Da dang nhap
                    {

                        char sendbuf[256];

                        strcpy(sendbuf, user_ids[j]);
                        strcat(sendbuf, ": ");
                        strcat(sendbuf, buf);

                        // Forward du lieu cho cac user
                        for (int k = 0; k < num_users; k++)
                            if (users[k] != client)
                                send(users[k], sendbuf, strlen(sendbuf), 0);
                    }
                }
            }
    }

    close(listener);

    return 0;
}