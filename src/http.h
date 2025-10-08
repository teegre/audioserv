#ifndef __HTTP_H__
#define __HTTP_H__

#include <linux/limits.h>
#include <stddef.h>

#define HTTP_PORT 8200

#define DRESPONSE_HEADER \
    "HTTP/1.1 207 Multi-Status\r\n" \
    "Content-Type: application/xml; charset\"utf-8\"\r\n\r\n" \
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
    "<D:multistatus xmlns:D=\"DAV:\">"

#define DRESPONSE_FOOTER "</D:multistatus>"

#define DRESPONSE \
    "<D:response>" \
      "<D:href>%s</D:href>" \
      "<D:propstat>" \
        "<D:prop>" \
          "<D:displayname>%s</D:displayname>" \
          "<D:resourcetype/>" \
          "<D:getcontentlength>%lld</D:getcontentlength>" \
          "<D:getcontenttype>%s</D:getcontenttype>" \
          "<D:getlastmodified>%s</D:getlastmodified>" \
        "</D:prop>" \
        "<D:status>HTTP/1.1 200 OK</D:status>" \
      "</D:propstat>" \
    "</D:response>"

#define DCOLLECTION \
    "<D:response>" \
      "<D:href>%s</D:href>" \
      "<D:propstat>" \
        "<D:prop>" \
          "<D:displayname>%s</D:displayname>" \
          "<D:resourcetype><D:collection/></D:resourcetype>" \
          "<D:getlastmodified>%s</D:getlastmodified>" \
        "</D:prop>" \
        "<D:status>HTTP/1.1 200 OK</D:status>" \
      "</D:propstat>" \
    "</D:response>"

extern char g_musicdir[PATH_MAX];

// HTTP server thread
void *http_worker(void*);

#endif /* ifndef __HTTP_H__ */
