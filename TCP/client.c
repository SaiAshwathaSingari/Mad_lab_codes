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

    int sfd;
    struct sockaddr_in serv;

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
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect
    if (connect(sfd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("Connection failed");
        close(sfd);
        exit(1);
    }

    printf("Connected to server.\n");

    // ================= STRING =================
    printf("Enter String: ");
    fgets(str, MAX, stdin);

    if (send(sfd, str, strlen(str) + 1, 0) <= 0) {
        perror("String send failed");
        exit(1);
    }

    if (recv(sfd, str, MAX, 0) <= 0) {
        perror("String receive failed");
        exit(1);
    }

    printf("Updated String: %s\n", str);

    // ================= INTEGER =================
    printf("Enter Integer: ");
    scanf("%d", &num);

    if (send(sfd, &num, sizeof(num), 0) <= 0) {
        perror("Integer send failed");
        exit(1);
    }

    recv_all(sfd, &num, sizeof(num));

    printf("Updated Integer: %d\n", num);

    // ================= ARRAY =================
    printf("Enter array size: ");
    scanf("%d", &n);

    if (n > MAX || n <= 0) {
        printf("Invalid array size\n");
        close(sfd);
        exit(1);
    }

    printf("Enter array elements:\n");
    for(int i = 0; i < n; i++)
        scanf("%d", &arr[i]);

    if (send(sfd, &n, sizeof(n), 0) <= 0) {
        perror("Array size send failed");
        exit(1);
    }

    if (send(sfd, arr, sizeof(int) * n, 0) <= 0) {
        perror("Array send failed");
        exit(1);
    }

    recv_all(sfd, arr, sizeof(int) * n);

    printf("Updated Array: ");
    for(int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");

    close(sfd);

    return 0;
}
