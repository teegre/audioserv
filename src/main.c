/*
AudioServ
A: Stéphane MEYER (Teegre)
C: 2025-09-25
M: 2025-10-09
*/

#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "http.h"
#include "util.h"

#define AUDIOSERV_VERSION "0.1"

char g_local_ip[64];
char g_location_url[256];
const char *lockfile = "/tmp/audioserv.lock";
int lockfd;

static void handle_signal(int sig) {
    (void)sig;
    printf("\n");
    printf("x received interruption signal.\n");
    g_running = 0;
}

int lock_instance(void) {
    int fd = open(lockfile, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("x lock");
        return -1;
    }
    if (lockf(fd, F_TLOCK, 0) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            fprintf(stderr, "x an instance of audioserv is already running.\n");
            close(fd);
            return -1;
        }
        perror("x lock");
        close(fd);
        return -1;
    }
    return fd;
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGPIPE, SIG_IGN);

    init_os_version();
    lowercase(OS_VERSION);

    printf("o audioserv version %s %s\n", AUDIOSERV_VERSION, OS_VERSION);

    int lockfd = lock_instance();
    if (lockfd == -1) return 1;

    init_music_dir();

    if (get_primary_ipv4(g_local_ip, sizeof(g_local_ip)) != 0) {
        snprintf(g_local_ip, sizeof(g_local_ip), "127.0.0.1");
    }
    snprintf(g_location_url, sizeof(g_location_url), "http://%s:%d", g_local_ip, HTTP_PORT);

    printf("o server address: %s\n", g_location_url);

    pthread_t http_tid;

    pthread_create(&http_tid, NULL, http_worker, NULL);
    while (g_running) {
        sleep(1);
    }

    pthread_join(http_tid, NULL);

    unlink(lockfile);
    close(lockfd);

    printf("o bye!\n");
    return 0;
}
