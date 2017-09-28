#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "common.h"

#define MAX_FILENAME   4096
#define MAX_REQUEST    4096
#define MAX_RESPONSE   4096

/**
 * The server command line arguments.
 */
typedef struct ServerArgs {
    unsigned short listenPort;
    const char    *htdocRoot;
} ServerArgs;

static ServerArgs svrArgs;


/**
 **************************************************************************
 *
 * \brief Create a TCP listen socket on the given port.
 *
 **************************************************************************
 */
static int
CreatePassiveTCP(unsigned port)
{
    int sock;
    int optval;
    struct sockaddr_in svrAddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to allocate the listen socket");
        exit(EXIT_FAILURE);
    }

    optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof (optval)) < 0) {
        perror("Failed to set socket option SO_REUSEADDR");
        exit(EXIT_FAILURE);
    }

    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_port = htons(port);
    svrAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&svrAddr, sizeof svrAddr) < 0) {
        perror("Failed to bind IP address and port to the listen socket");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 5) < 0) {
        perror("Failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    return sock;
}


/**
 **************************************************************************
 *
 * \brief Reads a request line from the client.
 *
 * The request line received from the client is saved as a null-terminated
 * string in the "buf" argument. The characters '\r' and '\n' are removed.
 *
 * Returns the number of bytes saved in the "buf" argument, or -1 if there
 * is an error or EOF.
 *
 **************************************************************************
 */
static int
httpd_readline(int sock, char *buf, int maxlen)
{
    int   n = 0;
    char *p = buf;

    while (n < maxlen - 1) {
        char c;
        int rc = read(sock, &c, 1);
        if (rc == 1) {
            /* Stop at \n and also strip away \r and \n. */
            if (c == '\n') {
                *p = 0;  /* null-terminated */
                return n;
            } else if (c != '\r') {
                *p++ = c;
                n++;
            }
        } else if (rc == 0) {
            return -1;   /* EOF */
        } else if (errno == EINTR) {
            continue;    /* retry */
        } else {
            return -1;   /* error */
        }
    }
    return -1;  /* buffer too small */
}


/**
 **************************************************************************
 *
 * \brief Get the size of the requested file.
 *
 **************************************************************************
 */
static int
httpd_get_file_size(FILE *fp)
{
    int fsize;
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    return fsize;
}


/**
 **************************************************************************
 *
 * \brief Get the MIME type of the requested file.
 *
 **************************************************************************
 */
static const char *
httpd_get_mime(const char *fname)
{
    const char *extf = strrchr(fname, '.');
    if (extf == NULL) {
        return "application/octet-stream";
    } else if (strcmp(extf, ".html") == 0) {
        return "text/html";
    } else if (strcmp(extf, ".jpg") == 0) {
        return "image/jpeg";
    } else if (strcmp(extf, ".gif") == 0) {
        return "image/gif";
    } else if (strcmp(extf, ".text") == 0 ||
               strcmp(extf, ".txt") == 0) {
        return "text/plain";
    } else {
        return "application/octet-stream";
    }
}


/**
 **************************************************************************
 *
 * \brief Send a response header to the client.
 *
 **************************************************************************
 */
static int
httpd_write_header(int sock, int status, int content_len, const char *mime)
{
    char header[MAX_RESPONSE];

    Log("thread-%u: RESPONSE: status=%d mime=%s content_length=%d\n",
        pthread_self(), status, mime, content_len);
    snprintf(header, sizeof header,
             "%s\r\n"
             "Server: 207httpd/0.0.1\r\n"
             "Connection: close\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "\r\n",
             status == 200 ? "HTTP/1.1 200 OK" : "HTTP/1.1 404 Not Found",
             mime, content_len);
    return write(sock, header, strlen(header));
}


/**
 **************************************************************************
 *
 * \brief Send a not-found (404) response to the client.
 *
 **************************************************************************
 */
static void
httpd_write_notfound(int sock, const char *fname)
{
    char response[MAX_RESPONSE];

    snprintf(response, sizeof response,
             "<html>\n<body>\n<h1>404 Not Found</h1>\n"
             "%s is not found\n"
             "</body></html>\n", fname);
    httpd_write_header(sock, 404, strlen(response), "text/html");
    write(sock, response, strlen(response));
}


/**
 **************************************************************************
 *
 * \brief Send a response to the client.
 *
 * If the requested file is found, a normal repsonse (200) is sent to the
 * client with the requested file.
 *
 * Otherwise, a not-found response (404) is sent to the client.
 *
 **************************************************************************
 */
static void
httpd_write_response(int sock, const char *fname)
{
    char data[MAX_RESPONSE];
    char fullpath[MAX_FILENAME];
    FILE *fp;
    int sz;

    snprintf(fullpath, sizeof fullpath, "%s/%s", svrArgs.htdocRoot, fname);
    fp = fopen(fullpath, "rb");
    if (fp == NULL) {
        httpd_write_notfound(sock, fname);
        return;
    }
    httpd_write_header(sock, 200,
                       httpd_get_file_size(fp),
                       httpd_get_mime(fname));
    while ((sz = fread(data, 1, sizeof data, fp)) > 0) {
        write(sock, data, sz);
    }
    fclose(fp);
}


/**
 **************************************************************************
 *
 * \brief Maps a URL pathname to a filename relative to htdocRoot.
 *
 * If the URL ends with "/", "index.html" is appended.
 * If the URL starts with "/images", it is then mapped to "/img".
 * Otherwise, the URL is mapped as-is to the filename.
 *
 **************************************************************************
 */
static int
httpd_map_url(const char *url, char *fname, int fname_len)
{
    const char *images = "/images/";
    const char *prefix = "";
    const char *suffix = "";


    if (strlen(url) == 0) {
        Error("thread-%u: Invalid empty URL\n", pthread_self());
        return -1;
    }
    if (url[strlen(url) - 1] == '/') {
        suffix = "index.html";
    }
    if (strlen(url) >= strlen(images) &&
        strncmp(url, images, strlen(images)) == 0) {
        url = url + strlen(images);
        prefix = "/img/";
    }
    snprintf(fname, fname_len, "%s%s%s", prefix, url, suffix);
    Log("thread-%u: GET: url=%s file=%s\n", pthread_self(), url, fname);
    return 0;
}


/**
 **************************************************************************
 *
 * \brief Parses a given line of a GET request.
 *
 * Returns 1 if it is the GET line,
 *         0 if it is any other GET header, or
 *        -1 if there is an error.
 *
 * The filename that the URL is mapped to is saved in the "fname" argument.
 * The string in the "req" argument may also be modified.
 *
 **************************************************************************
 */
static int
httpd_parse_get_request(char *req, char *fname, int fname_len)
{
    if (strlen(req) > 4 && memcmp(req, "GET ", 4) == 0) {
        char *httpStr;
        char *url = req + 4;
        httpStr = strchr(url, ' ');
        if (memcmp(httpStr, " HTTP", 5) != 0) {
            Error("thread-%u: Invalid GET: %s\n", pthread_self(), req);
            return -1;
        }
        *httpStr = '\0';
        if (httpd_map_url(url, fname, fname_len) < 0) {
            return -1;
        }
        return 1;
    }
    /* Ignore other headers. */
    return 0;
}


/**
 **************************************************************************
 *
 * \brief The thread worker function to handle each GET request.
 *
 **************************************************************************
 */
static void *
httpd_process_request(void *arg)
{
    char req[MAX_REQUEST];
    char fname[MAX_REQUEST];
    int sock    = (int)arg;
    bool gotURL = false;

    Log("thread-%u: Starting (ssock=%u)\n", pthread_self(), sock);

    while (1) {
        int rc;
        if (httpd_readline(sock, req, sizeof req) <= 0) {
            break;
        }
        Log("thread-%u: HEADER: %s\n", pthread_self(), req);
        rc = httpd_parse_get_request(req, fname, sizeof fname);
        if (rc == 1) {
            gotURL = true;
        } else if (rc < 0) {
            break;
        }
    }
    if (gotURL) {
        httpd_write_response(sock, fname);
    }

    Log("thread-%u: Exiting (ssock=%u)\n", pthread_self(), sock);
    close(sock);
    return NULL;
}


/**
 **************************************************************************
 *
 * \brief The server loop to accept new client connections.
 *
 **************************************************************************
 */
static void
ServerListenerLoop(int msock)
{
    pthread_t th;
    pthread_attr_t ta;
    bool listenerRunning = true;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    while (listenerRunning) {
        int ssock;
        struct sockaddr_in cliAddr;
        struct sockaddr_in localAddr;
        socklen_t cliAddrLen;
        socklen_t localAddrLen;
        char cliName[INET_ADDRSTRLEN + PORT_STRLEN];
        char svrName[INET_ADDRSTRLEN + PORT_STRLEN];

        cliAddrLen = sizeof cliAddr;
        ssock = accept(msock, (struct sockaddr *)&cliAddr, &cliAddrLen);
        if (ssock < 0) {
            if (listenerRunning && ssock != EINTR) {
                perror("Failed to accept a connection");
                listenerRunning = false;
            }
            continue;
        }

        localAddrLen = sizeof localAddr;
        if (getsockname(ssock,
                        (struct sockaddr *)&localAddr,
                        &localAddrLen) < 0) {
            perror("Failed to get server address info for new connection");
            close(ssock);
            continue;
        }
        SocketAddrToString(&localAddr, svrName, sizeof svrName);
        SocketAddrToString(&cliAddr, cliName, sizeof cliName);
        Log("Accepted client %s at server %s (ssock=%u)\n",
            cliName, svrName, ssock);

        if (pthread_create(&th, &ta, httpd_process_request,
                           (void *)ssock) < 0) {
            perror("Failed to create a worker thread");
            listenerRunning = false;
            close(ssock);
            continue;
        }
    }
    pthread_attr_destroy(&ta);
}


/**
 **************************************************************************
 *
 * \brief Show the usage message and exit the program.
 *
 **************************************************************************
 */
static void
Usage(const char *prog) // IN
{
    Log("Usage:\n");
    Log("    %s port /path/to/htdoc\n", prog);
    exit(EXIT_FAILURE);
}


/**
 **************************************************************************
 *
 * \brief Parse command line arguments.
 *
 **************************************************************************
 */
void
ParseArgs(int argc,            // IN
          char *argv[],        // IN
          ServerArgs *svrArgs) // OUT
{
    if (argc != 3) {
        Usage(argv[0]);
    }
    svrArgs->listenPort = atoi(argv[1]);
    svrArgs->htdocRoot  = argv[2];
    if (svrArgs->listenPort == 0) {
        Usage(argv[0]);
    }
}

/**
 **************************************************************************
 *
 * \brief Main entry point.
 *
 **************************************************************************
 */
int
main(int argc, char *argv[])
{
    int msock;

    ParseArgs(argc, argv, &svrArgs);

    msock = CreatePassiveTCP(svrArgs.listenPort);

    Log("\nhttpd started at port %u, htdoc=%s\n",
        svrArgs.listenPort, svrArgs.htdocRoot);

    ServerListenerLoop(msock);

    close(msock);

    Log("\nhttpd stopped\n");

    return 0;
}
