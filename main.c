#include <stdio.h>
#include <string.h>         // mem***, bzero(void *, size_t)...
#include <sys/socket.h>     // socket(int, int, int)...
#include <netinet/in.h>     // struct sockaddr_in
#include <sys/errno.h>      // global variable: errno
#include <unistd.h>         // function: close(int)
#include <arpa/inet.h>      // function: inet_addr(const char *)

#include <zlib.h>

#include "dechunk.h"

int main (int argc, const char * argv[]) 
{
    const char              *server_ipstr   = "10.0.0.173";   // the host address of "www.mtime.com"
                                                                // you can also use function gethostbyname
                                                                // to get the address
    const unsigned short    server_port     = 8080;               // default port of http protocol
    
    int sock_http_get = -1;
    
    do 
    {
        sock_http_get = socket(PF_INET, SOCK_STREAM, 0);
        
        if (-1 == sock_http_get)
        {
            printf("socket() error %d.\n", errno);
            break;
        }

        struct sockaddr_in addr_server;
        bzero(&addr_server, sizeof(addr_server));
        
        addr_server.sin_family          = AF_INET;
        addr_server.sin_addr.s_addr     = inet_addr(server_ipstr);
        addr_server.sin_port            = htons(server_port);
    
        if (-1 == connect(sock_http_get, (struct sockaddr *)&addr_server, sizeof(addr_server)))
        {
            printf("connect() error %d.\n", errno);
            break;
        }
        
        printf("connected...\n");
        
        char *request = 
        "GET / HTTP/1.1\r\n"
        "Host: www.mtime.com\r\n"
        "Connection: keep-alive\r\n"
        "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_6_8) AppleWebKit/535.1 (KHTML, like Gecko) Chrome/13.0.782.220 Safari/535.1\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
        "Accept-Encoding: gzip,deflate,sdch\r\n"
        "Accept-Language: zh-CN,zh;q=0.8\r\n"
        "Accept-Charset: GBK,utf-8;q=0.7,*;q=0.3\r\n"
        "Cookie: DefaultCity-CookieKey=561; DefaultDistrict-CookieKey=0; _userCode_=2011725182104293; _userIdentity_=2011725182109188; _movies_=105799.87871.91873.68867; __utma=196937584.1484842614.1299113024.1314613017.1315205344.4; __utmz=196937584.1299113024.1.1.utmcsr=(direct)|utmccn=(direct)|utmcmd=(none); _urm=0%7C0%7C0%7C0%7C0; _urmt=Mon%2C%2005%20Sep%202011%2006%3A49%3A04%20GMT\r\n"
        "\r\n";

        if (-1 == send(sock_http_get, request, strlen(request), 0))
        {
            printf("send() error %d.\n", errno);
            break;
        }
        
#define MY_BUF_SIZE     1024
        char buff[MY_BUF_SIZE] = {0};
        int recv_bytes = 0;
        int chunkret = DCE_OK;
        
        if (DCE_OK == dechunk_init())
        {
            while (-1 != (recv_bytes = recv(sock_http_get, buff, MY_BUF_SIZE, 0)) 
                   && 0 != recv_bytes)
            {
                printf("%s", buff);
                if (DCE_OK != (chunkret = dechunk(buff, recv_bytes)))
                {
                    printf("\nchunkret = %d\n", chunkret);
                    break;
                }
                
                if (NULL != memstr(buff, recv_bytes, "\r\n0\r\n"))
                {
                    break;
                }
                
                bzero(buff, MY_BUF_SIZE);
            }
            
            printf("\n*********************************\n");
            printf("receive finished.\n");
            printf("*********************************\n");
            
            void *zipbuf = NULL;
            size_t zipsize = 0;
            dechunk_getbuff(&zipbuf, &zipsize);
            
            printf("\n%s\n", (char *)zipbuf);
            
            z_stream strm;
            bzero(&strm, sizeof(strm));

            printf("BEGIN:decompress...\n");
            printf("*********************************\n");
            
            if (Z_OK == inflateInit2(&strm, 31))    // 31:decompress gzip
            {
                strm.next_in    = zipbuf;
                strm.avail_in   = zipsize;
                
                char zbuff[MY_BUF_SIZE] = {0};
                
                do 
                {
                    bzero(zbuff, MY_BUF_SIZE);
                    strm.next_out = (Bytef *)zbuff;
                    strm.avail_out = MY_BUF_SIZE;
                    
                    int zlibret = inflate(&strm, Z_NO_FLUSH);
                    
                    if (zlibret != Z_OK && zlibret != Z_STREAM_END)
                    {
                        printf("\ninflate ret = %d\n", zlibret);
                        break;
                    }
                    
                    printf("%s", zbuff);
                    
                } while (strm.avail_out == 0);
            }
            
            printf("\n");
            printf("*********************************\n");
            
            dechunk_free();
        }
#undef MY_BUF_SIZE
        
    } while (0);
    
    close(sock_http_get);
    
    return 0;
}
