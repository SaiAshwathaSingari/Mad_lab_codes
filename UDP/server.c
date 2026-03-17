#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 100

int main() {

    int sfd;
    struct sockaddr_in serv, cli;
    socklen_t len = sizeof(cli);

    char str[MAX];
    int num;
    int arr[MAX];
    int n;

    // Create socket
    sfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORT);
    serv.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(sfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("Bind failed");
        close(sfd);
        exit(1);
    }

    printf("UDP Server is listening on port %d...\n", PORT);

    // ================= STRING =================
    if (recvfrom(sfd, str, MAX, 0, (struct sockaddr*)&cli, &len) < 0) {
        perror("String receive failed");
        exit(1);
    }

    for(int i = 0; str[i]; i++)
        if(str[i] >= 'a' && str[i] <= 'z')
            str[i] -= 32;

    if (sendto(sfd, str, strlen(str) + 1, 0,
               (struct sockaddr*)&cli, len) < 0) {
        perror("String send failed");
        exit(1);
    }

    // ================= INTEGER =================
    if (recvfrom(sfd, &num, sizeof(num), 0,
                 (struct sockaddr*)&cli, &len) < 0) {
        perror("Integer receive failed");
        exit(1);
    }

    num = num * num;

    if (sendto(sfd, &num, sizeof(num), 0,
               (struct sockaddr*)&cli, len) < 0) {
        perror("Integer send failed");
        exit(1);
    }

    // ================= ARRAY =================
    if (recvfrom(sfd, &n, sizeof(n), 0,
                 (struct sockaddr*)&cli, &len) < 0) {
        perror("Array size receive failed");
        exit(1);
    }

    if (n > MAX || n <= 0) {
        printf("Invalid array size\n");
        exit(1);
    }

    if (recvfrom(sfd, arr, sizeof(int)*n, 0,
                 (struct sockaddr*)&cli, &len) < 0) {
        perror("Array receive failed");
        exit(1);
    }

    for(int i = 0; i < n; i++)
        arr[i] *= 2;

    if (sendto(sfd, arr, sizeof(int)*n, 0,
               (struct sockaddr*)&cli, len) < 0) {
        perror("Array send failed");
        exit(1);
    }

    close(sfd);
    return 0;
}
