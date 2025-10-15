#ifndef __HTTP_H__
#define __HTTP_H__

#include <linux/limits.h>
#include <stddef.h>

#define HTTP_PORT 8200

#define HTTP_HEADER \
    "<!doctype html>" \
    "<html xmlns=\"http://www.w3.org/1999/xhtml\" lang=\"en\" xml:lang=\"en\">" \
      "<head>" \
        "<meta charset=\"utf-8\"/>" \
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=yes\"/>" \
        "<title>AudioServ</title>" \
        "<style>" \
          "html {" \
            "background: #0D0D0D;" \
            "color: #292A2D;" \
          "}" \
          "a {" \
            "color: #8ab4f8;" \
          "}" \
        "</style>" \
      "</head>" \
      "<h1>Index of "

#define HTTP_FOOTER "</ul></body></html>"

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
