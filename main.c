// ldd: -lcrypto
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define OUTPUT_FILE "found."
#define DIFFICULTY 6
#define READ_BUFFER 10 // Int Max

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
    pid_t pid = getpid();

    while (1) {
        str = malloc((intlen(first) + 1) * sizeof(char));
        if (!str){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        sprintf(str, "%d", first);
        if (zeros(md5hash((unsigned char *)str), zero)){
            nonce = strdup(str);
            free(str);
            break;
        }
        free(str);
        first += step;
    }
    str = malloc((strlen(OUTPUT_FILE) + intlen(pid) + 1) * sizeof(char));
    sprintf(str, "%s%d", OUTPUT_FILE, pid);
    int fd = open(str, O_WRONLY | O_CREAT, 0600);
    if (fd == -1){
        perror("open");
        exit(EXIT_FAILURE);
    }
    write(fd, nonce, strlen(nonce));
    if (close(fd) == -1){
        perror("close");
        exit(EXIT_FAILURE);
    }
    free(nonce);
    exit(EXIT_SUCCESS);
}


int main(void) {
    pid_t ppid = getpid();
    pid_t grp = -1;
    char *result_file;
    char buffer[READ_BUFFER];
    int fd, status, size;
    pid_t result_pid;

    for (int i = 0; i < 10; i++) {
        pid_t child = fork();
        if (child < 0){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (child == 0){
            bruteforce(i, 10, DIFFICULTY);
        } else {
            if (grp == -1){
                grp = child;
            }
            if (setpgid(child, grp) == -1) {
                perror("setpgid");
            }
        }
    }
    while (1) {
        result_pid = wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            break;
        }
    }
    kill(-grp, SIGKILL);
    result_file = malloc((strlen(OUTPUT_FILE) + intlen(result_pid) + 1) * sizeof(char));
    sprintf(result_file, "%s%d", OUTPUT_FILE, result_pid);
    fd = open(result_file, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    if ((size = read(fd, buffer, READ_BUFFER)) == -1) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
    buffer[size] = '\0';
    printf("%s\n", buffer);
    if (unlink(result_file) == -1) {
        perror("unlink");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}
