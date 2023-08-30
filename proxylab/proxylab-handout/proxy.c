#include <stdio.h>
#include "csapp.h"
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct Uri
{
    char host[MAXLINE]; // hostname
    char port[MAXLINE]; // 端口
    char path[MAXLINE]; // 路径
};

struct sbuf_t
{
    
};

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void doit(int fd);
void parse_uri(const char *uri, struct Uri *uri_data);
void build_header(char *header, const char *host, const char *path);
void send_request(int serverfd, char *header);
void receive_response(int connfd, int serverfd);
void sigpipe_handler(int signum);

void doit(int connfd)
{
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char server[MAXLINE];

    rio_t rio, server_rio;

    /* Read request line and headers */
    Rio_readinitb(&rio, connfd); // 将描述符与读缓冲区和重置缓冲区关联
    Rio_readlineb(&rio, buf, MAXLINE); // 健壮地读取文本行(缓冲)
    printf("Request headers:\n");
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET"))
    {
        printf("proxy does not implement this method");
        return;
    }

    struct Uri uri_data;
    parse_uri(uri, &uri_data);

    build_header(buf, uri_data.host, uri_data.path);
    int serverfd = open_clientfd(uri_data.host, uri_data.port);
    if (serverfd < 0)
    {
        printf("connection failed\n");
        return;
    }

    send_request(serverfd, buf);
    receive_response(connfd, serverfd);

    Close(serverfd);
}

void parse_uri(const char *uri, struct Uri *uri_data)
{
    char tmp_uri[MAXLINE];
    strncpy(tmp_uri, uri, MAXLINE);

    char *host_pos = strstr(tmp_uri, "//");
    if (host_pos == NULL)
    {
        strncpy(uri_data->path, tmp_uri, MAXLINE);
        strncpy(uri_data->port, "80", MAXLINE);
        return;
    }
    else
    {
        char *port_pos = strstr(host_pos + 2, ":");
        if (port_pos != NULL)
        {
            *port_pos = '\0';
            strncpy(uri_data->port, port_pos + 1, MAXLINE);
            sscanf(uri_data->port, "%[^/]", uri_data->port);
            strncpy(uri_data->path, strstr(port_pos + 1, "/"), MAXLINE);
        }
        else
        {
            strncpy(uri_data->path, strstr(host_pos + 2, "/"), MAXLINE);
            strncpy(uri_data->port, "80", MAXLINE);
        }

        *host_pos = '\0';
        strncpy(uri_data->host, host_pos + 2, MAXLINE);
    }
}

void sigpipe_handler(int signum)
{
    printf("Received SIGPIPE signal\n");
}

void build_header(char *header, const char *host, const char *path)
{
    snprintf(header, MAXLINE,
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "User-Agent: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host, user_agent_hdr);
}

void send_request(int serverfd, char *header)
{
    Rio_writen(serverfd, header, strlen(header));
}

void receive_response(int connfd, int serverfd)
{
    char buf[MAXLINE];
    ssize_t n;
    rio_t client_rio, server_rio;

    Rio_readinitb(&server_rio, serverfd);
    Rio_readinitb(&client_rio, connfd);

    while ((n = Rio_readlineb(&server_rio, buf, MAXLINE)) != 0)
    {
        printf("proxy received %d bytes, then send\n", (int)n);
        Rio_writen(connfd, buf, n);
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2)
        unix_error("proxy usage: ./proxy <port number>");

    Signal(SIGPIPE, sigpipe_handler);
    listenfd = Open_listenfd(argv[1]);

    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s %s).\n", hostname, port);

        doit(connfd);
        Close(connfd);
    }

    Close(listenfd); // 关闭监听描述符

    return 0;
}
