#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX 100

int main() {

    int sfd;
    struct sockaddr_in serv;
    socklen_t len = sizeof(serv);

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
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("UDP Client ready.\n");

    // ================= STRING =================
    printf("Enter String: ");
    fgets(str, MAX, stdin);

    if (sendto(sfd, str, strlen(str) + 1, 0,
               (struct sockaddr*)&serv, len) < 0) {
        perror("String send failed");
        exit(1);
    }

    if (recvfrom(sfd, str, MAX, 0,
                 (struct sockaddr*)&serv, &len) < 0) {
        perror("String receive failed");
        exit(1);
    }

    printf("Updated String: %s\n", str);

    // ================= INTEGER =================
    printf("Enter Integer: ");
    scanf("%d", &num);

    if (sendto(sfd, &num, sizeof(num), 0,
               (struct sockaddr*)&serv, len) < 0) {
        perror("Integer send failed");
        exit(1);
    }

    if (recvfrom(sfd, &num, sizeof(num), 0,
                 (struct sockaddr*)&serv, &len) < 0) {
        perror("Integer receive failed");
        exit(1);
    }

    printf("Updated Integer: %d\n", num);

    // ================= ARRAY =================
    printf("Enter array size: ");
    scanf("%d", &n);

    if (n > MAX || n <= 0) {
        printf("Invalid array size\n");
        exit(1);
    }

    printf("Enter array elements:\n");
    for(int i = 0; i < n; i++)
        scanf("%d", &arr[i]);

    if (sendto(sfd, &n, sizeof(n), 0,
               (struct sockaddr*)&serv, len) < 0) {
        perror("Array size send failed");
        exit(1);
    }

    if (sendto(sfd, arr, sizeof(int)*n, 0,
               (struct sockaddr*)&serv, len) < 0) {
        perror("Array send failed");
        exit(1);
    }

    if (recvfrom(sfd, arr, sizeof(int)*n, 0,
                 (struct sockaddr*)&serv, &len) < 0) {
        perror("Array receive failed");
        exit(1);
    }

    printf("Updated Array: ");
    for(int i = 0; i < n; i++)
        printf("%d ", arr[i]);
    printf("\n");

    close(sfd);
    return 0;
}
