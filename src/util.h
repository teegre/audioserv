#ifndef __UTIL_H__
#define __UTIL_H__

#include <linux/limits.h>
#include <stddef.h>
#include <stdatomic.h>
#include <time.h>

#define MUSIC_DIR "%s/.audioserv/music"

extern atomic_int g_running;
extern char g_musicdir[PATH_MAX];

extern char OS_VERSION[256];

extern char g_uuid[64];
extern char g_local_ip[64];
extern char g_location_url[256];

// void generate_uuid(char*, size_t);
// void generate_uuid2(char*, size_t);
int get_primary_ipv4(char*, size_t);
void replace_whitespace(char*, char);
void init_os_version(void);
void init_music_dir(void);
void get_lastmodified(char*, timer_t);
void lowercase(char*);
const char *get_mime_type(const char*);
int from_hex(char);
void url_encode(const char*, char*, size_t);
void url_decode(char*);
void html_escape(const char*, char*, size_t);

#endif /* ifndef __UTIL_H__ */
