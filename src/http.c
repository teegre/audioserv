/*
Minimal HTTP file server
A: Stéphane MEYER (Teegre)
C: 2025-09-25
M: 2025-10-09
*/

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include "http.h"
#include "pool.h"
#include "util.h"

void send_file(int conn, const char *filepath, const char *filename, const char* range_header) {
    struct stat st;
    if (stat(filepath, &st) < 0 || !S_ISREG(st.st_mode)) {
        const char *resp =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "File not found\n";
        send(conn, resp, strlen(resp), 0);
        return;
    }

    char timebuf[128];
    struct tm tm;
    gmtime_r(&st.st_mtime, &tm);
    strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", &tm);
    
    FILE *f = fopen(filepath, "rb");
    if (!f) return;

    const char *mime = get_mime_type(filename);
    
    off_t start = 0;
    off_t end = st.st_size - 1;
    int partial = 0;

    if (range_header && strncmp(range_header, "bytes=", 6) == 0) {
        char *dash = strchr(range_header + 6, '-');
        if (dash) {
            *dash = '\0';
            start = atoll(range_header + 6);
            if (*(dash + 1) != '\0') {
                end = atoll(dash + 1);
            }
            if (start < 0) start = 0;
            if (end >= st.st_size) end = st.st_size - 1;
            if (start <= end) {
                partial = 1;
            }
        }
    }

    off_t length = end - start + 1;

    char header[512];
    if (partial) {
        snprintf(header, sizeof(header), 
            "HTTP/1.1 206 Partial Content\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %jd\r\n"
            "Content-Range: bytes %jd-%jd/%jd\r\n"
            "Accept-Ranges: bytes\r\n"
            "Last-Modified: %s\r\n\r\n",
            mime, (intmax_t)length,
            (intmax_t)start, (intmax_t)end, (intmax_t)st.st_size, timebuf);
    }
    else {
        snprintf(header, sizeof(header), 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %jd\r\n"
            "Accept-Ranges: bytes\r\n"
            "Last-Modified: %s\r\n\r\n",
            mime, (intmax_t)st.st_size, timebuf);
    }
    send(conn, header, strlen(header), 0);

    fseeko(f, start, SEEK_SET);

    char buffer[4096];
    off_t remaining = length;
    while (remaining > 0) {
        if (!g_running) {
            const char *resp =
                "HTTP/1.1 503 Service Unavailable\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n"
                "Content-Length: 19\r\n\r\n"
                "Server shutting down\n";
            send(conn, resp, strlen(resp), MSG_NOSIGNAL);
            printf("x 503 service unavailable\n");
            fclose(f);
            return;
        }

        size_t to_read = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        size_t n = fread(buffer, 1, to_read, f);
        if (n == 0) break;
        if (send(conn, buffer, n, MSG_NOSIGNAL) <= 0) break;
        remaining -= n;
    }

    fclose(f);
}

int cmpstringp(const void *p1, const void *p2) {
    return strcmp(*(const char **)p1, *(const char **)p2);
}

void send_directory_listing(int conn, const char *dirpath, const char *urlpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        const char *resp =
          "HTTP/1.1 403 Forbidden\r\n"
          "Content-Type: text/plain\r\n\r\n"
          "Cannot open directory\n";
        send(conn, resp, strlen(resp), 0);
        return;
    }

    char *entries[1024];
    size_t count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) && g_running) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        if (strcmp(entry->d_name, "..") == 0) continue;
        char *copy = strdup(entry->d_name);
        if (count < 1024) {
            entries[count++] = copy;
        }
        else {
            free(copy);
            break;
        }
    }
    closedir(dir);

    qsort(entries, count, sizeof(char *), cmpstringp);

    const char *header =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/html; charset=utf-8\r\n\r\n";
    send(conn, header, strlen(header), 0);

    const char *html_head = "<html><body><h1>Index of ";
    send(conn, html_head, strlen(html_head), 0);
    send(conn, urlpath, strlen(urlpath), 0);
    const char *html_mid = "</h1><ul>";
    send(conn, html_mid, strlen(html_mid), 0);

    for (size_t i = 0; i < count; i++) {
        char filepath[PATH_MAX];
        struct stat st;

        snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entries[i]);

        if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
            const char *mime = get_mime_type(filepath);
            if (strcmp(mime, "application/octet-stream") == 0) {
                free(entries[i]);
                continue;
            }
        }

        char line[1024] = {0};
        char escaped[1024] = {0};
        char encoded[1024] = {0};

        url_encode(entries[i], encoded, sizeof(encoded));  // href
        html_escape(entries[i], escaped, sizeof(escaped)); // display

        if (strcmp(urlpath, "/") == 0) {
          snprintf(line, sizeof(line),
              "<li><a href=\"%s\">%s</a>",
              encoded, escaped);
        } 
        else {
            snprintf(line, sizeof(line),
                "<li><a href=\"%s/%s\">%s</a></li>",
                urlpath,
                encoded,
                escaped);
        }
        send(conn, line, strlen(line), 0);
        free(entries[i]);
    }

    const char *html_end = "</ul></body></html>";
    send(conn, html_end, strlen(html_end), 0);
}

void handle_propfind_request(int conn, char *dirpath, const char *urlpath, int depth) {
    struct stat st;
    char timebuf[128];
    struct tm tm;

    if (stat(dirpath, &st) == 0 && S_ISREG(st.st_mode)) {
        gmtime_r(&st.st_mtime, &tm);
        strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", &tm);

        const char *mime = get_mime_type(dirpath);

        char href[1024];
        url_encode(urlpath, href, sizeof(href));

        char buf[2048];
        snprintf(buf, sizeof(buf),
            DRESPONSE_HEADER DRESPONSE,
            urlpath, basename(dirpath),
            (long long)st.st_size, mime, timebuf);
        send(conn, buf, strlen(buf), 0);
        return;
    }

    DIR *dir = opendir(dirpath);
    if (!dir) {
        const char *resp =
          "HTTP/1.1 404 Not Found\r\n"
          "Content-Type: text/plain\r\n\r\n"
          "PROPFIND Not found\n";
        send(conn, resp, strlen(resp), 0);
        return;
    }

    char buffer[8192];
    send(conn, DRESPONSE_HEADER, strlen(DRESPONSE_HEADER), 0);

    char href[1024];
    url_encode(urlpath, href, sizeof(href));

    if ( depth == 0 ) {
        snprintf(buffer, sizeof(buffer), 
            "<D:response>"
              "<D:href>%s</D:href>"
              "<D:propstat>"
                "<D:prop>"
                  "<D:displayname>%s</D:displayname>"
                  "<D:resourcetype><D:collection/></D:resourcetype>"
                "</D:prop>"
                "<D:status>HTTP/1.1 200 OK</D:status>"
              "</D:propstat>"
            "</D:response>", href, urlpath);
        send(conn, buffer, strlen(buffer), 0);
    }

    if (depth > 0) {
        char timebuf[128];
        struct tm tm;
        struct stat st_self;
        // SELF...
        if (stat(dirpath, &st_self) == 0) {
            gmtime_r(&st_self.st_mtime, &tm);
            strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", &tm);

            char self_href[1024];
            snprintf(self_href, sizeof(self_href), "%s%s", urlpath,
                urlpath[strlen(urlpath) - 1] == '/' ? "" : "/");
            char buf[2048];
            snprintf(buf, sizeof(buf),
                DCOLLECTION,
                self_href,
                basename(dirpath),
                timebuf);
            send(conn, buf, strlen(buf), 0);
        }

        struct dirent *entry;
        while ((entry = readdir(dir))) {
            if (!g_running) {
                const char *resp =
                    "HTTP/1.1 503 Service Unavailable\r\n"
                    "Content-Length: 0\r\n\r\n";
                send(conn, resp, strlen(resp), 0);
                closedir(dir);
                return;
            }
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char childpath[1024];
            snprintf(childpath, sizeof(childpath), "%s/%s", dirpath, entry->d_name);

            if (stat(childpath, &st) != 0) continue;

            char entry_href[1024];
            char entry_encoded[512];
            url_encode(entry->d_name, entry_encoded, sizeof(entry_encoded));
            gmtime_r(&st.st_mtime, &tm);
            strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT", &tm);

            if (S_ISDIR(st.st_mode)) {
                snprintf(entry_href, sizeof(entry_href), "%s%s/", urlpath, entry_encoded);
            } else {
                snprintf(entry_href, sizeof(entry_href), "%s%s", urlpath, entry_encoded);
            }

            char prop_buffer[2048];
            if (S_ISDIR(st.st_mode)) {
                snprintf(prop_buffer, sizeof(prop_buffer),
                    DCOLLECTION, entry_href, entry->d_name, timebuf);
            }
            else {
                const char *mime = get_mime_type(entry->d_name);
                // Only show audio and cover art images
                if (strcmp(mime, "application/octet-stream") == 0)  continue;
                snprintf(prop_buffer, sizeof(prop_buffer),
                    DRESPONSE,
                    entry_href, entry->d_name,
                    (long long)st.st_size, mime, timebuf);
            }
            send(conn, prop_buffer, strlen(prop_buffer), 0);
        }
    }

    send(conn, DRESPONSE_FOOTER, strlen(DRESPONSE_FOOTER), 0);

    closedir(dir);
}

void handle_http_request(int conn) {
    char buf[1024];
    ssize_t r = recv(conn, buf, sizeof(buf) - 1, 0);

    if (r <= 0) return;

    buf[r] = 0;

    char method[16], path[512];
    if (sscanf(buf, "%15s %511s", method, path) != 2) return;

    char buf_copy[8192];
    strncpy(buf_copy, buf, sizeof(buf_copy));
    buf_copy[sizeof(buf_copy) - 1] = '\0';

    char *range_header = NULL;
    char *line = strtok(buf_copy, "\r\n");

    while (line) {
        if (strncasecmp(line, "Range:", 6) == 0) {
            range_header =  line + 6;
            while (*range_header == ' ') range_header++;
        }
        line = strtok(NULL, "\r\n");
    }

    url_decode(path);

    char filepath[PATH_MAX*2];
    snprintf(filepath, sizeof(filepath), "%s%s", g_musicdir, path);

    char resolved[PATH_MAX];
    if (!realpath(filepath, resolved)) {
        const char *resp =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n\r\n"
            "Not Found\n";
        send(conn, resp, strlen(resp), 0);
        return;
    }

    char root_resolved[PATH_MAX];
    if (!realpath(g_musicdir, root_resolved)) {
        perror("realpath(g_musicdir) failed");
        return;
    }

    if (strncmp(resolved, root_resolved, strlen(root_resolved)) != 0) {
        const char *resp =
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 14\r\n\r\n"
            "Access denied\n";
        send(conn, resp, strlen(resp), 0);
        return;
    }

    struct stat st;
    if (stat(filepath, &st) != 0) {
        const char *resp =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 10\r\n\r\n"
            "Not Found\n";
        send(conn, resp, strlen(resp), 0);
        return;
    }

    if (strcmp(method, "GET") == 0) {
        if (S_ISDIR(st.st_mode)) {
            printf("→ %s\n", path);
            send_directory_listing(conn, filepath, path);
        }
        else if (S_ISREG(st.st_mode)) {
            printf("+ get file %s\n", path);
            send_file(conn, filepath, path, range_header);
        }
        else {
            const char *resp =
                "HTTP/1.1 403 Forbidden\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: 20\r\n\r\n"
                "Unsupported file type\n";
            send(conn, resp, strlen(resp), 0);
        }
    }
    else if (strcmp(method, "HEAD") == 0) {
        if (S_ISREG(st.st_mode)) {
            const char *mime = get_mime_type(path);
            char header[256];
            snprintf(header, sizeof(header), 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %ld\r\n"
                "DAV: 1,2\r\n"
                "Allow: OPTIONS, GET, HEAD, PROPFIND\r\n\r\n",
                mime, st.st_size);

            printf("h file %s\n", path);
            send(conn, header, strlen(header), 0);
        }
        else if (S_ISDIR(st.st_mode)) {
            char header[1024];
            snprintf(header, sizeof(header),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: httpd/unix-directory\r\n"
                "Content-Length: 0\r\n"
                "Content-Location: %s\r\n"
                "DAV: 1,2\r\n"
                "Allow: OPTIONS, GET, HEAD, PROPFIND\r\n\r\n", path);

            printf("→ %s\n", path);
            send(conn, header, strlen(header), 0);
        }
        else {
            const char *resp =
                "HTTP/1.1 403 Forbidden\r\n"
                "Content-Type: text/plain\r\n\r\n"
                "Unsupported file type\n";
            send(conn, resp, strlen(resp), 0);
        }
    }
    else if (strcmp(method, "PROPFIND") == 0) {
        int depth = 0;
        char *depth_hdr = strcasestr(buf, "Depth:");
        if (depth_hdr) {
            depth = atoi(depth_hdr + 6);
            if (depth < 0) depth = 0;
            if (depth > 1) depth = 1;
        }
        printf("p %s\n", path);
        handle_propfind_request(conn, filepath, path, depth);
    }
    else if (strcmp(method, "OPTIONS") == 0) {
        const char *resp =
            "HTTP/1.1 200 OK\r\n"
            "DAV: 1,2\r\n"
            "Allow: OPTIONS, GET, HEAD, PROPFIND\r\n"
            "Content-Length: 0\r\n\r\n";
        send(conn, resp, strlen(resp), 0);
    }
    else {
        const char *resp =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Allow: OPTIONS, GET, HEAD, PROPFIND\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 18\r\n\r\n"
            "Method not allowed\n";
        send(conn, resp, strlen(resp), 0);
    }
    close(conn);
    return;
}

void *worker_thread(void *arg) {
    while (q_running) {
        int conn = queue_pop();
        if (conn < 0) break;
        handle_http_request(conn);
        close(conn);
    }
    return NULL;
}

void *http_worker(void *arg) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd < 0) {
        perror("http socket");
        return NULL;
    }

    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("http setsockopt");
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HTTP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("http bind");
        close(listen_fd);
        return NULL;
    }

    if (listen(listen_fd, 10) < 0) {
      perror("http listen");
      close(listen_fd);
      return NULL;
    }

    printf("* listening on port %d\n", HTTP_PORT);

    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_create(&workers[i], NULL, worker_thread, NULL);
    }

    while (g_running) {
      fd_set fds;
      struct timeval tv;

      FD_ZERO(&fds);
      FD_SET(listen_fd, &fds);

      tv.tv_sec = 1;
      tv.tv_usec = 0;

      int ret = select(listen_fd + 1, &fds, NULL, NULL, &tv);
      if (ret > 0 && FD_ISSET(listen_fd, &fds)) {
          if (!g_running) break;
          struct sockaddr_in peer = {0};
          socklen_t peer_len = sizeof(peer);
          int conn = accept(listen_fd, (struct sockaddr*)&peer, &peer_len);
          // int conn = accept(listen_fd, NULL, NULL);
          if (conn >= 0) {
              char ipstr[INET_ADDRSTRLEN];
              inet_ntop(AF_INET, &peer.sin_addr, ipstr, INET_ADDRSTRLEN);
              printf("<%s> ", ipstr);
              queue_push(conn);
          }
          else if (errno != EINTR) {
              perror("http accept");
              break;
          }
      }
      else if (ret < 0 && errno != EINTR) {
          perror("http select");
          break;
      }
    }

    close(listen_fd);

    printf("* shutting down...\n");

    queue_cleanup();

    printf("* bye!\n");
    return NULL;
}
