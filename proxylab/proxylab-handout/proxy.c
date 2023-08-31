#include <stdio.h>
#include "csapp.h"
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define NTHREADS 4
#define SBUFSIZE 16

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

// 客户端与服务端相关
struct Uri
{
    char host[MAXLINE]; // hostname
    char port[MAXLINE]; // 端口
    char path[MAXLINE]; // 路径
};
void doit(int fd);
void parse_uri(const char *uri, struct Uri *uri_data);
void build_header(char *header, const char *host, const char *path);
void send_request(int serverfd, char *header);
void receive_response(int connfd, int serverfd);
void sigpipe_handler(int signum);

//多线程相关
typedef struct
{
    int * buf ;  // buffer array
    int n ;     // maximum number of slots
    int front ; // buf[(front + 1) % n] is fitst item
    int rear ;  // buf[rear % n] is last item
    sem_t mutex ;  // protects accesses to buf
    sem_t slots ;  // count s available slots
    sem_t items ; // counts available items
}sbuf_t;
sbuf_t sbuf ;
void sbuf_init (sbuf_t* sp , int n) ;
void sbuf_insert (sbuf_t * sp , int item) ;
int sbuf_remove (sbuf_t * sp) ;
void * thread (void * vargp) ;

//cache 以及 读写者模型
int readcnt ;
sem_t mutex , w ;
void reader() ;
void writer() ;


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

void sbuf_init(sbuf_t * sp , int n)
{
    sp -> buf = Calloc(n , sizeof(int)) ; // 分配内存
    sp -> n = n ; // buffer holds max of n items
    sp -> front = sp ->rear = 0 ; // empty buffer iff front == rear
    Sem_init(&sp -> mutex , 0 , 1) ; // binary semaphore for locking
    Sem_init(&sp -> slots , 0 , n) ; // initially buf has n empty slots
    Sem_init(&sp -> items , 0 , 0) ; // initially buf has zero data items
}

void sbuf_insert(sbuf_t * sp , int item)
{
    P(&sp -> slots) ; // wait for available slot
    P(&sp -> mutex) ; // lock the buffer
    sp -> buf[(++sp -> rear) % (sp -> n)] = item ; //sp->rear 是下一个可插入位置的索引，sp->n 表示缓冲区的大小。通过对 sp->rear 进行自增并取模运算，可以循环利用缓冲区。
    V(&sp -> mutex) ; // unlock the buffer
    V(&sp -> items) ; // announce available item
}

int sbuf_remove(sbuf_t * sp)
{
    int item;
    P(&sp -> items);                          /* Wait for available item */
    P(&sp -> mutex);                          /* Lock the buffer */
    item = sp -> buf[(++sp -> front)%(sp -> n)];  /* Remove the item */
    V(&sp -> mutex);                          /* Unlock the buffer */
    V(&sp -> slots);                          /* Announce available slot */
    return item;
}

void * thread (void * vargp) // 线程执行函数
{
    Pthread_detach(pthread_self());

    while(1)
    {
        //从缓冲区中读出描述符
        int connfd = sbuf_remove(&sbuf);
        //转发
        doit(connfd);
        //关闭客户端的连接描述符
        Close(connfd) ;
    }
}

void reader()
{
    while (1)
    {
        P(&mutex) ;
        readcnt++ ;

        if(readcnt == 1)
            P(&w) ;
        V(&mutex) ;

        P(&mutex) ;
        readcnt-- ;

        if(readcnt == 0)
            V(&w) ;
        V(&mutex) ;
    }
}

void writer()
{
    while(1)
    {
        P(&w) ;

        V(&w) ;
    }
}

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid ;

    if (argc != 2)
        unix_error("proxy usage: ./proxy <port number>");

    Signal(SIGPIPE, sigpipe_handler);
    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf , SBUFSIZE) ; // 初始化线程数据
    for (int i = 0 ; i < NTHREADS ; i++) // create worker threads
        Pthread_create(&tid , NULL , thread , NULL) ;

    while (1)
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        sbuf_insert(&sbuf , connfd) ; // 向缓冲区写入描述符
        Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        printf("Accepted connection from (%s %s).\n", hostname, port);
    }

    Close(listenfd); // 关闭监听描述符

    return 0;
}
