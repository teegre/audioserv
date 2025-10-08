/*
Utility functions.
A: Stéphane MEYER (Teegre)
C: 2025-09-25
M: 2025-09-27
*/

#include <bits/types/timer_t.h>
#include <ctype.h>
#include <ifaddrs.h>
#include <linux/limits.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/utsname.h>

#include "util.h"

atomic_int g_running = 1;
char g_musicdir[PATH_MAX];

char OS_VERSION[256];

int get_primary_ipv4(char *out, size_t n) {
    // Get IP of the machine on the local networ
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    struct sockaddr_in remote;
    remote.sin_family = AF_INET;
    remote.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);
    if (connect(sock, (struct sockaddr*)&remote, sizeof(remote)) < 0) {
        close(sock);
        return -1;
    }
    struct sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(sock, (struct sockaddr*)&local, &len) < 0) {
        close(sock);
        return -1;
    }
    close(sock);
    inet_ntop(AF_INET, &local.sin_addr, out, n);
    return 0;
}

void replace_whitespace(char *str, char c) {
    while (*str) {
        if (isspace((unsigned char)*str)) {
            *str = c;
        }
        str++;
    }
}

void lowercase(char *str) {
  int c = 0;
  while (str[c]) {
      str[c] = tolower(str[c]);
      c++;
  }
}

void init_music_dir(void) {
    struct passwd *pw = getpwuid(getuid());
    snprintf(g_musicdir, sizeof(g_musicdir), MUSIC_DIR, pw->pw_dir);
}

void init_os_version(void) {
    struct utsname info;
    if (uname(&info) == 0) {
        snprintf(OS_VERSION, sizeof(OS_VERSION), "%s/%s", info.sysname, info.release);
        replace_whitespace(OS_VERSION, '-');
    } else {
        strcpy(OS_VERSION, "Unknown");
    }
}

const char *get_mime_type(const char *filename) {
  const char *ext = strrchr(filename, '.');
  if (!ext) return "application/octet-stream";
  ext++;
  if (strcasecmp(ext, "flac") == 0) return "audio/flac";
  if (strcasecmp(ext, "m3u")  == 0) return "audio/x-mpegurl";
  if (strcasecmp(ext, "m4a")  == 0) return "audio/mp4";
  if (strcasecmp(ext, "mp3")  == 0) return "audio/mpeg";
  if (strcasecmp(ext, "ogg")  == 0) return "audio/ogg";
  if (strcasecmp(ext, "opus") == 0) return "audio/opus";
  if (strcasecmp(ext, "wav")  == 0) return "audio/wav";
  if (strcasecmp(ext, "jpg")  == 0) return "image/jpeg";
  if (strcasecmp(ext, "png")  == 0) return "image/png";
  return "application/octet-stream";
}

int from_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

void url_encode(const char *src, char *dst, size_t size) {
    static const char *hex = "0123456789ABCDEF";
    while (*src && size > 1) {
        unsigned char c = (unsigned char)*src;
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            *dst++ = c;
            size--;
        }
        else {
            if (size < 4) break;
            *dst++ = '%';
            *dst++ = hex[c >> 4];
            *dst++ = hex[c & 0xF];
            size -= 3;
        }
        src++;
    }
    *dst = '\0';
}

void url_decode(char *str) {
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            int hi = from_hex(src[1]);
            int lo = from_hex(src[2]);
            if (hi >= 0 && lo >= 0) {
                *dst++ = (char)((hi <<  4) |  lo);
                src += 3;
                continue;
            }
        }
        else if (*src == '+') {
            *dst++ = ' ';
            src++;
            continue;
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

void html_escape(const char *src, char *dst, size_t size) {
    while (*src && size > 1) {
        if (*src == '<') {
            strncat(dst, "&lt;", size - 1);
            size -= 4;
        }
        else if (*src == '>') {
            strncat(dst, "&gt;", size - 1);
            size -= 4;
        }
        else if (*src == '&') {
            strncat(dst, "&amp;", size - 1);
            size -= 5;
        }
        else if (*src == ' ') {
            strncat(dst, "&nbsp;", size - 1);
            size -= 6;
        }
        else {
            strncat(dst, (char[]){*src, 0}, size-1);
            size--;
        }
        src++;
    }
}
