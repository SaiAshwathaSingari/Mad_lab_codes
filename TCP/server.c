#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 100

void recv_all(int sock, void *buffer, int size) {
    int received = 0;
    while (received < size) {
        int r = recv(sock, (char*)buffer + received, size - received, 0);
        if (r <= 0) {
            perror("Receive failed");
            exit(1);
        }
        received += r;
    }
}

int main() {

    int sfd, cfd;
    struct sockaddr_in serv, cli;
    socklen_t len = sizeof(cli);

    char str[MAX];
    int num;
    int arr[MAX];
    int n;

    // Create socket
    sfd = socket(AF_INET, SOCK_STREAM, 0);
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

    // Listen
    if (listen(sfd, 5) < 0) {
        perror("Listen failed");
        close(sfd);
        exit(1);
    }

    printf("Server is listening on port %d...\n", PORT);

    // Accept
    cfd = accept(sfd, (struct sockaddr*)&cli, &len);
    if (cfd < 0) {
        perror("Accept failed");
        close(sfd);
        exit(1);
    }

    printf("Client connected.\n");

    // ================= STRING =================
    if (recv(cfd, str, MAX, 0) <= 0) {
        perror("String receive failed");
        exit(1);
    }

    char alpha[20];
    int j = 0;
    int sum = 0;
    for(int i = 0; str[i]!='\0'; i++){
        if(str[i]>='A' && str[i]<='Z'){
            alpha[j] = str[i];
            j++;
        }
        if(str[i]>='0' && str[i]<='9'){
            sum = sum + (str[i] - '0');
        }
    }
    alpha[j] = '\0';
    char temp[200];
    sprintf(temp, "%s and sum is %d", alpha, sum);

    if (send(cfd, temp, strlen(temp) + 1, 0) <= 0) {
        perror("String send failed");
        exit(1);
    }

    // ================= INTEGER =================
    recv_all(cfd, &num, sizeof(num));

    num = num * num;

    if (send(cfd, &num, sizeof(num), 0) <= 0) {
        perror("Integer send failed");
        exit(1);
    }

    // ================= ARRAY =================
    recv_all(cfd, &n, sizeof(n));

    if (n > MAX || n <= 0) {
        printf("Invalid array size\n");
        close(cfd);
        close(sfd);
        exit(1);
    }

    recv_all(cfd, arr, sizeof(int) * n);

    for(int i = 0; i < n; i++)
        arr[i] *= 2;

    if (send(cfd, arr, sizeof(int) * n, 0) <= 0) {
        perror("Array send failed");
        exit(1);
    }

    close(cfd);
    close(sfd);

    return 0;
}
