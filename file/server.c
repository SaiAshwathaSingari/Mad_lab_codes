// server.c
// Simple TCP file server that performs many file ops on r.txt (and directory)
// small names, minimal comments

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8081
#define FNAME "r.txt"
#define BACKLOG 5

// send exactly n bytes
int send_all(int s, const void *buf, int n){
    const char *p = buf;
    int sent = 0;
    while(sent < n){
        int r = send(s, p+sent, n-sent, 0);
        if(r <= 0) return -1;
        sent += r;
    }
    return sent;
}
// recv exactly n bytes
int recv_all(int s, void *buf, int n){
    char *p = buf;
    int got = 0;
    while(got < n){
        int r = recv(s, p+got, n-got, 0);
        if(r <= 0) return -1;
        got += r;
    }
    return got;
}
// send length-prefixed message
int send_msg(int s, const char *msg){
    uint32_t ln = (uint32_t)strlen(msg);
    uint32_t nl = htonl(ln);
    if(send_all(s, &nl, 4) < 0) return -1;
    if(ln==0) return 0;
    return send_all(s, msg, ln);
}
// recv length-pref msg, allocates heap string (caller free)
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

// helper to read whole file into heap buffer (caller free)
char *read_file_all(const char *fn){
    FILE *f = fopen(fn,"r");
    if(!f) return NULL;
    fseek(f,0,SEEK_END);
    long sz = ftell(f);
    fseek(f,0,SEEK_SET);
    char *b = malloc(sz+1);
    if(!b){ fclose(f); return NULL; }
    size_t r = fread(b,1,sz,f);
    b[r]=0;
    fclose(f);
    return b;
}

// op: list files in current dir
char *op_list_files(){
    DIR *d = opendir(".");
    if(!d) return strdup("opendir failed\n");
    struct dirent *e;
    size_t cap = 512;
    char *out = malloc(cap); out[0]=0;
    while((e = readdir(d))){
        if(strlen(out)+strlen(e->d_name)+3 > cap){
            cap *= 2; out = realloc(out,cap);
        }
        strcat(out, e->d_name);
        strcat(out, "\n");
    }
    closedir(d);
    return out;
}

// op: read whole file
char *op_read_all(){
    char *t = read_file_all(FNAME);
    if(!t) return strdup("file open error or file not present\n");
    return t;
}

// op: read specific line (1-based)
char *op_read_line(int ln){
    FILE *f = fopen(FNAME,"r");
    if(!f) return strdup("file open error\n");
    char buf[1024];
    int i=1;
    while(fgets(buf, sizeof(buf), f)){
        if(i==ln){
            fclose(f);
            char *r = malloc(strlen(buf)+32);
            snprintf(r, strlen(buf)+32, "line %d: %s", ln, buf);
            return r;
        }
        i++;
    }
    fclose(f);
    return strdup("line not found\n");
}

// op: find pattern (case-sensitive) -> return count + lines
char *op_find(const char *pat){
    FILE *f = fopen(FNAME,"r");
    if(!f) return strdup("file open error\n");
    char buf[2048];
    int lineno=0;
    int tot=0;
    size_t cap = 1024;
    char *out = malloc(cap); out[0]=0;
    while(fgets(buf, sizeof(buf), f)){
        lineno++;
        char *p = buf;
        while((p = strstr(p, pat))){ tot++; p = p + 1; }
        if(strstr(buf,pat)){
            size_t need = strlen(out) + 64 + strlen(buf);
            if(need > cap){ cap = need*2; out = realloc(out, cap); }
            char tmp[128];
            snprintf(tmp, sizeof(tmp), "line %d: ", lineno);
            strcat(out,tmp);
            strcat(out, buf);
        }
    }
    fclose(f);
    char hdr[64];
    snprintf(hdr, sizeof(hdr), "found %d occurrences\n", tot);
    char *res = malloc(strlen(hdr) + strlen(out) + 4);
    strcpy(res, hdr);
    strcat(res, out);
    free(out);
    return res;
}

// op: replace all occurrences of old with neu (write back), return summary
char *op_replace(const char *oldw, const char *neww){
    char *txt = read_file_all(FNAME);
    if(!txt) return strdup("file open error\n");
    size_t oldl = strlen(oldw), newl = strlen(neww);
    // naive replace
    size_t cap = strlen(txt) + 1;
    char *out = malloc(cap+1); out[0]=0;
    int repl = 0;
    char *p = txt;
    while(*p){
        char *q = strstr(p, oldw);
        if(!q){
            // append rest
            size_t need = strlen(out) + strlen(p) + 1;
            out = realloc(out, need+1);
            strcat(out, p);
            break;
        }
        // append part before q
        size_t before = q - p;
        size_t need = strlen(out) + before + newl + 8;
        out = realloc(out, need+1);
        strncat(out, p, before);
        strcat(out, neww);
        repl++;
        p = q + oldl;
    }
    free(txt);
    if(repl==0){
        free(out);
        return strdup("no matches, file unchanged\n");
    }
    // write back
    FILE *f = fopen(FNAME,"w");
    if(!f){ free(out); return strdup("write error\n"); }
    fwrite(out,1,strlen(out),f);
    fclose(f);
    char hdr[80];
    snprintf(hdr, sizeof(hdr), "replaced %d occurrences\n", repl);
    free(out);
    return strdup(hdr);
}

// op: count vowels
char *op_count_vowels(){
    FILE *f = fopen(FNAME,"r");
    if(!f) return strdup("file open error\n");
    long c, cnt=0;
    while((c = fgetc(f)) != EOF){
        char ch = (char)c;
        if(strchr("aeiouAEIOU", ch)) cnt++;
    }
    fclose(f);
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "vowel count = %ld\n", cnt);
    return strdup(tmp);
}

// op: sum digits
char *op_sum_digits(){
    FILE *f = fopen(FNAME,"r");
    if(!f) return strdup("file open error\n");
    long c, sum=0;
    while((c = fgetc(f)) != EOF){
        if(c>='0' && c<='9') sum += c - '0';
    }
    fclose(f);
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "sum of digits = %ld\n", sum);
    return strdup(tmp);
}

int handle_cmd(int cs, const char *cmd){
    // cmd is like: "LIST", "READ", "READLINE:3", "FIND:pat", "REPLACE:old:neu", "VOWELS", "SUMDIG", "EXIT"
    if(strcmp(cmd, "LIST")==0){
        char *r = op_list_files();
        int ok = send_msg(cs, r);
        free(r);
        return ok==0?0:1;
    } else if(strcmp(cmd, "READ")==0){
        char *r = op_read_all();
        int ok = send_msg(cs, r);
        free(r);
        return ok==0?0:1;
    } else if(strncmp(cmd, "READLINE:",9)==0){
        int ln = atoi(cmd+9);
        char *r = op_read_line(ln);
        int ok = send_msg(cs, r);
        free(r);
        return ok==0?0:1;
    } else if(strncmp(cmd, "FIND:",5)==0){
        char *pat = (char*)(cmd+5);
        char *r = op_find(pat);
        int ok = send_msg(cs, r);
        free(r);
        return ok==0?0:1;
    } else if(strncmp(cmd, "REPLACE:",8)==0){
        // format REPLACE:old:neu
        const char *p1 = cmd+8;
        const char *p2 = strchr(p1, ':');
        if(!p2) return send_msg(cs, "bad replace format, use REPLACE:old:neu\n"), 1;
        size_t l1 = p2 - p1;
        char *oldw = malloc(l1+1);
        strncpy(oldw, p1, l1); oldw[l1]=0;
        const char *neu = p2+1;
        char *r = op_replace(oldw, neu);
        int ok = send_msg(cs, r);
        free(r); free(oldw);
        return ok==0?0:1;
    } else if(strcmp(cmd, "VOWELS")==0){
        char *r = op_count_vowels();
        int ok = send_msg(cs, r);
        free(r);
        return ok==0?0:1;
    } else if(strcmp(cmd, "SUMDIG")==0){
        char *r = op_sum_digits();
        int ok = send_msg(cs, r);
        free(r);
        return ok==0?0:1;
    } else if(strcmp(cmd, "EXIT")==0){
        send_msg(cs, "bye\n");
        return 0;
    } else {
        send_msg(cs, "unknown command\n");
        return 1;
    }
}

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){ perror("socket"); return 1; }
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in a;
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = INADDR_ANY;
    a.sin_port = htons(PORT);
    if(bind(s, (struct sockaddr*)&a, sizeof(a)) < 0){ perror("bind"); return 1; }
    if(listen(s, BACKLOG) < 0){ perror("listen"); return 1; }
    printf("server listening on %d\n", PORT);
    while(1){
        struct sockaddr_in caddr;
        socklen_t cl = sizeof(caddr);
        int cs = accept(s, (struct sockaddr*)&caddr, &cl);
        if(cs < 0){ perror("accept"); continue; }
        printf("client connected %s\n", inet_ntoa(caddr.sin_addr));
        while(1){
            char *req = recv_msg(cs);
            if(!req) { printf("client disconnected\n"); close(cs); break; }
            // handle and if client sent EXIT break
            int rc = handle_cmd(cs, req);
            free(req);
            if(rc==0) { /* if command was EXIT or send failed */ break; }
            // otherwise continue
        }
    }
    close(s);
    return 0;
}
