#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define OUTPUT_FILE "found."
#define DIFFICULTY 6

// = {0} => all the array is reset to zero (only works for zero!)
char hash[1 + 2*MD5_DIGEST_LENGTH] = {0};

char *md5hash (const unsigned char *str){
    unsigned char md5[MD5_DIGEST_LENGTH] = {0};
    MD5(str, strlen((char *)str), md5);
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++){
        sprintf(hash + 2*i, "%02x", md5[i]);
    }
    return hash;
}

int zeros (char *s, int n){
    while (n > 0){
        if (*s != '0')
            return 0;
        n--;
        s++;
    }
    return 1;
}

int intlen (int n){
    int i = 0;
    while (n > 0){
        i++;
        n /= 10;
    }
    return i;
}

void bruteforce (int first, int step, int zero){
    char *str;
    char *nonce;

    pid_t ppid = getpid();
    pid_t pid = fork();
    if (setpgid(pid, ppid) == -1 || pid < 0){
        perror(pid < 0 ? "fork" : "setpgid");
        exit(EXIT_FAILURE);
    }
    while (1) {
        str = malloc((intlen(first) + 1) * sizeof(char));
        if (!str){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        sprintf(str, "%d", first);
        if (zeros(md5hash((unsigned char *)str), zero)){
            free(str);
            break;
        }
        free(str);
        first += step;
    }
    if (ppid == getpid()){
        nonce = malloc((intlen(first) + 1) * sizeof(char));
        sprintf(nonce, "%d", first);
        str = malloc((strlen(OUTPUT_FILE) + intlen(pid) + 1) * sizeof(char));
        sprintf(str, "%s%d", OUTPUT_FILE, pid);
        int fd = open(str, O_WRONLY | O_CREAT, 470);
        if (fd == -1){
            perror("open");
            exit(EXIT_FAILURE);
        }
        write(fd, nonce, strlen(nonce));
        if (close(fd) == -1){
            perror("close");
            exit(EXIT_FAILURE);
        }
        kill(ppid, SIGKILL);
    }
    exit(EXIT_SUCCESS);
}

int main (void){
    for (int i = 0; i < 10; i++){
        bruteforce(i, 10, DIFFICULTY);
    }
}
