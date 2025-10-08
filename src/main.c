/*
AudioServ
A: Stéphane MEYER (Teegre)
C: 2025-09-25
M: 2025-10-08
*/

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

static void handle_signal(int sig) {
    (void)sig;
    printf("\n");
    printf("x received interruption signal.\n");
    g_running = 0;
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGPIPE, SIG_IGN);

    init_os_version();
    lowercase(OS_VERSION);

    printf("o audioserv version %s %s\n", AUDIOSERV_VERSION, OS_VERSION);

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

    printf("o bye!\n");
    return 0;
}
