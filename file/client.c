// client.c
// Simple TCP client to talk with server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8081
#define HOST "127.0.0.1"

// send exactly n bytes
int send_all(int s, const void *buf, int n){
    const char *p = buf; int sent=0;
    while(sent < n){
        int r = send(s, p+sent, n-sent, 0);
        if(r <= 0) return -1;
        sent += r;
    }
    return sent;
}
int recv_all(int s, void *buf, int n){
    char *p = buf; int got=0;
    while(got < n){
        int r = recv(s, p+got, n-got, 0);
        if(r <= 0) return -1;
        got += r;
    }
    return got;
}
int send_msg(int s, const char *msg){
    uint32_t ln = (uint32_t)strlen(msg);
    uint32_t nl = htonl(ln);
    if(send_all(s, &nl, 4) < 0) return -1;
    if(ln==0) return 0;
    return send_all(s, msg, ln);
}
char *recv_msg(int s){
    uint32_t nl;
    if(recv_all(s, &nl, 4) <= 0) return NULL;
    uint32_t ln = ntohl(nl);
    char *b = malloc(ln+1);
    if(ln>0){
        if(recv_all(s, b, ln) <= 0){ free(b); return NULL; }
    }
    b[ln]=0;
    return b;
}

void show_menu(){
    puts("==== menu ====");
    puts("1) list files in folder");
    puts("2) read whole file (r.txt)");
    puts("3) read line (enter line no)");
    puts("4) find pattern (enter pattern)");
    puts("5) replace (enter old then new)");
    puts("6) count vowels");
    puts("7) sum digits");
    puts("8) exit");
    puts("==============");
}

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){ perror("socket"); return 1; }
    struct sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_port = htons(PORT);
    inet_pton(AF_INET, HOST, &a.sin_addr);
    if(connect(s, (struct sockaddr*)&a, sizeof(a)) < 0){ perror("connect"); return 1; }
    printf("connected to %s:%d\n", HOST, PORT);
    while(1){
        show_menu();
        int ch;
        printf("choice: ");
        if(scanf("%d", &ch)!=1){ while(getchar()!= '\n'); continue; }
        while(getchar()!='\n'); // eat rest
        char sendb[2048];
        sendb[0]=0;
        if(ch==1) strcpy(sendb, "LIST");
        else if(ch==2) strcpy(sendb, "READ");
        else if(ch==3){
            int ln;
            printf("line no: ");
            if(scanf("%d",&ln)!=1){ while(getchar()!='\n'); printf("bad input\n"); continue; }
            while(getchar()!='\n');
            snprintf(sendb, sizeof(sendb), "READLINE:%d", ln);
        } else if(ch==4){
            char pat[512];
            printf("pattern: ");
            if(!fgets(pat, sizeof(pat), stdin)) continue;
            pat[strcspn(pat, "\n")] = 0;
            snprintf(sendb, sizeof(sendb), "FIND:%s", pat);
        } else if(ch==5){
            char aold[256], anew[256];
            printf("old: ");
            if(!fgets(aold,sizeof(aold),stdin)) continue;
            aold[strcspn(aold,"\n")] = 0;
            printf("new: ");
            if(!fgets(anew,sizeof(anew),stdin)) continue;
            anew[strcspn(anew,"\n")] = 0;
            snprintf(sendb, sizeof(sendb), "REPLACE:%s:%s", aold, anew);
        } else if(ch==6) strcpy(sendb, "VOWELS");
        else if(ch==7) strcpy(sendb, "SUMDIG");
        else if(ch==8) { strcpy(sendb, "EXIT"); send_msg(s, sendb); char *r = recv_msg(s); if(r){ printf("%s\n", r); free(r);} break; }
        else { printf("bad choice\n"); continue; }

        if(send_msg(s, sendb) < 0){ printf("send error\n"); break; }
        char *resp = recv_msg(s);
        if(!resp){ printf("server disconnected\n"); break; }
        printf("---- server response ----\n%s\n-------------------------\n", resp);
        free(resp);
    }
    close(s);
    return 0;
}
