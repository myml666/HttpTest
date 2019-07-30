#include <jni.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include "http.h"

#define BUFFER_SIZE 1024
#define HTTP_POST "POST /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\n"\
    "Content-Type:application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s"
#define HTTP_GET "GET /%s HTTP/1.1\r\nHOST: %s:%d\r\nAccept: */*\r\n\r\n"
static int http_tcpclient_create(const char* host, int port) {
    struct hostent* he;
    struct sockaddr_in server_addr;
    int socket_fd;

    if ((he = gethostbyname(host)) == NULL) {
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = *((struct in_addr*)he->h_addr);

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    if (connect(socket_fd, (struct sockaddr*) & server_addr, sizeof(struct sockaddr)) == -1) {
        return -1;
    }

    return socket_fd;
}
/*
 * 关闭连接
 * */
static void http_tcpclient_close(int socket) {
    close(socket);
}
/*
 * 解析URL
 * */
static int http_parse_url(const char* url, char* host, char* file, int* port)
{
    char* ptr1, * ptr2;
    int len = 0;
    if (!url || !host || !file || !port) {
        return -1;
    }

    ptr1 = (char*)url;

    if (!strncmp(ptr1, "http://", strlen("http://"))) {
        ptr1 += strlen("http://");
    }
    else {
        return -1;
    }

    ptr2 = strchr(ptr1, '/');
    if (ptr2) {
        len = strlen(ptr1) - strlen(ptr2);
        memcpy(host, ptr1, len);
        host[len] = '\0';
        if (*(ptr2 + 1)) {
            memcpy(file, ptr2 + 1, strlen(ptr2) - 1);
            file[strlen(ptr2) - 1] = '\0';
        }
    }
    else {
        memcpy(host, ptr1, strlen(ptr1));
        host[strlen(ptr1)] = '\0';
    }
    //get host and ip
    ptr1 = strchr(host, ':');
    if (ptr1) {
        *ptr1++ = '\0';
        *port = atoi(ptr1);
    }
    else {
        *port = MY_HTTP_DEFAULT_PORT;
    }

    return 0;
}


static int http_tcpclient_recv(int socket, char* lpbuff) {
    int recvnum = 0;

    recvnum = recv(socket, lpbuff, BUFFER_SIZE * 4, 0);

    return recvnum;
}

static int http_tcpclient_send(int socket, char* buff, int size) {
    int sent = 0, tmpres = 0;

    while (sent < size) {
        tmpres = send(socket, buff + sent, size - sent, 0);
        if (tmpres == -1) {
            return -1;
        }
        sent += tmpres;
    }
    return sent;
}

static char* http_parse_result(const char* lpbuf)
{
    char* ptmp = NULL;
    char* response = NULL;
    ptmp = (char*)strstr(lpbuf, "HTTP/1.1");
    if (!ptmp) {
        printf("http/1.1 not faind\n");
        return NULL;
    }
    if (atoi(ptmp + 9) != 200) {
        printf("result:\n%s\n", lpbuf);
        return NULL;
    }

    ptmp = (char*)strstr(lpbuf, "\r\n\r\n");
    if (!ptmp) {
        printf("ptmp is NULL\n");
        return NULL;
    }
    response = (char*)malloc(strlen(ptmp) + 1);
    if (!response) {
        printf("malloc failed \n");
        return NULL;
    }
    strcpy(response, ptmp + 4);
    return response;
}
/*
 * Post请求
 * */
char* http_post(const char* url, const char* post_str) {

    char post[BUFFER_SIZE] = { '\0' };
    int socket_fd = -1;
    char lpbuf[BUFFER_SIZE * 4] = { '\0' };
    char* ptmp;
    char host_addr[BUFFER_SIZE] = { '\0' };
    char file[BUFFER_SIZE] = { '\0' };
    int port = 0;
    int len = 0;
    char* response = NULL;

    if (!url || !post_str) {
        printf("      failed!\n");
        return NULL;
    }

    if (http_parse_url(url, host_addr, file, &port)) {
        printf("http_parse_url failed!\n");
        return NULL;
    }
    //printf("host_addr : %s\tfile:%s\t,%d\n",host_addr,file,port);

    socket_fd = http_tcpclient_create(host_addr, port);
    if (socket_fd < 0) {
        printf("http_tcpclient_create failed\n");
        return NULL;
    }

    sprintf(lpbuf, HTTP_POST, file, host_addr, port, strlen(post_str), post_str);

    if (http_tcpclient_send(socket_fd, lpbuf, strlen(lpbuf)) < 0) {
        printf("http_tcpclient_send failed..\n");
        return NULL;
    }
    //printf("发送请求:\n%s\n",lpbuf);

    /*it's time to recv from server*/
    if (http_tcpclient_recv(socket_fd, lpbuf) <= 0) {
        printf("http_tcpclient_recv failed\n");
        return NULL;
    }

    http_tcpclient_close(socket_fd);

    return http_parse_result(lpbuf);
}
/*
 * Get请求
 * */
char* http_get(const char* url)
{

    char post[BUFFER_SIZE] = { '\0' };
    int socket_fd = -1;
    char lpbuf[BUFFER_SIZE * 4] = { '\0' };
    char* ptmp;
    char host_addr[BUFFER_SIZE] = { '\0' };
    char file[BUFFER_SIZE] = { '\0' };
    int port = 0;
    int len = 0;

    if (!url) {
        printf("      failed!\n");
        return NULL;
    }

    if (http_parse_url(url, host_addr, file, &port)) {
        printf("http_parse_url failed!\n");
        return NULL;
    }
    //printf("host_addr : %s\tfile:%s\t,%d\n",host_addr,file,port);

    socket_fd = http_tcpclient_create(host_addr, port);
    if (socket_fd < 0) {
        printf("http_tcpclient_create failed\n");
        return NULL;
    }

    sprintf(lpbuf, HTTP_GET, file, host_addr, port);

    if (http_tcpclient_send(socket_fd, lpbuf, strlen(lpbuf)) < 0) {
        printf("http_tcpclient_send failed..\n");
        return NULL;
    }
    //	printf("发送请求:\n%s\n",lpbuf);

    if (http_tcpclient_recv(socket_fd, lpbuf) <= 0) {
        printf("http_tcpclient_recv failed\n");
        return NULL;
    }
    http_tcpclient_close(socket_fd);

    return http_parse_result(lpbuf);
}
/*
 * 字符串转换
 * */
char*   Jstring2CStr(JNIEnv*   env,   jstring   jstr)
{
    char*   rtn   =   NULL;
    jclass   clsstring   =   (*env)->FindClass(env,"java/lang/String");
    jstring   strencode   =  (*env)->NewStringUTF(env,"GB2312");
    jmethodID   mid   =   (*env)->GetMethodID(env,clsstring,   "getBytes",   "(Ljava/lang/String;)[B");
    jbyteArray   barr=   (jbyteArray)(*env)->CallObjectMethod(env,jstr,mid,strencode);
    jsize   alen   =  (*env)->GetArrayLength(env,barr);
    jbyte*   ba   =   (*env)->GetByteArrayElements(env,barr,JNI_FALSE);
    if(alen   >   0)
    {
        rtn   =   (char*)malloc(alen+1);         //new   char[alen+1];
        memcpy(rtn,ba,alen);
        rtn[alen]=0;
    }
    (*env)->ReleaseByteArrayElements(env,barr,ba,0);

    return rtn;
}
JNIEXPORT jstring JNICALL
Java_com_itfitness_httptest_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject jobj,jstring postUrl,jstring params) {
    char * pu = Jstring2CStr(env,postUrl);
    char * pm = Jstring2CStr(env,params);
    char * result = http_post(pu,pm);
    return (*env)->NewStringUTF(env,result);
}