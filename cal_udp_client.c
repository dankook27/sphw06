#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 3800
#define IP "10.0.10.30"

struct cal_data
{
    int left_num;
    int right_num;
    char op;
    int result;
    short int error;
};

int main(int argc, char **argv)
{
    struct sockaddr_in server_addr;
    int s;
    int len;
    int sbyte, rbyte;
    struct cal_data sdata;
    int max_val, min_val;
    socklen_t addr_len;
    
    if (argc != 1)
    {
         printf("Usage : %s\n", argv[0]);
         return 1;
    }

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s == -1)
    {
         return 1;
    }
   
    // 클라이언트 소켓에 동적 포트 할당
    struct sockaddr_in client_addr;
    memset((void *)&client_addr, 0x00, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    client_addr.sin_port = htons(0);  // 0으로 설정하여 시스템이 자동으로 포트 할당
    
    if (bind(s, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
    {
         perror("bind error");
         close(s);
         return 1;
    }
    
    // 클라이언트가 사용하는 실제 포트 번호 출력
    socklen_t addr_len = sizeof(client_addr);
    getsockname(s, (struct sockaddr *)&client_addr, &addr_len);
    printf("Client started on port %d\n", ntohs(client_addr.sin_port));
   
    memset((void *)&server_addr, 0x00, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);  // 서버의 포트 번호
    server_addr.sin_addr.s_addr = inet_addr(IP);

    // 연산자가 '$'일 때까지 계속 통신
    while(1)
    {
        memset((void *)&sdata, 0x00, sizeof(sdata));
        
        // 사용자 입력 받기
        printf("Enter operation (num1 num2 op, or '$' to quit): ");
        char input[100];
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            break;
        }
        
        // 키보드 입력을 16진법으로 출력
        printf("FROM KEYBOARD : ");
        for (int i = 0; input[i] != '\0' && input[i] != '\n'; i++)
        {
            printf("%02x ", (unsigned char)input[i]);
        }
        printf("\n");
        
        if (sscanf(input, "%d %d %c", &sdata.left_num, &sdata.right_num, &sdata.op) != 3)
        {
            if (input[0] == '$')
            {
                sdata.left_num = 0;
                sdata.right_num = 0;
                sdata.op = '$';
            }
            else
            {
                printf("Invalid input format\n");
                continue;
            }
        }

        len = sizeof(sdata);
        sdata.left_num = htonl(sdata.left_num);
        sdata.right_num = htonl(sdata.right_num);
        
        // UDP로 데이터 전송
        sbyte = sendto(s, (char *)&sdata, len, 0, 
                      (struct sockaddr *)&server_addr, sizeof(server_addr));
        if(sbyte != len)
        {
             close(s);
             return 1;
        }

        // UDP로 데이터 수신
        addr_len = sizeof(server_addr);
        rbyte = recvfrom(s, (char *)&sdata, len, 0, 
                        (struct sockaddr *)&server_addr, &addr_len);
        if(rbyte != len)
        {
             close(s);
             return 1;
        }
        
        if (ntohs(sdata.error) != 0)
        {
             printf("CALC Error %d\n", ntohs(sdata.error));
        }
        else
        {
             printf("%d %c %d = %d\n", ntohl(sdata.left_num), sdata.op, ntohl(sdata.right_num), ntohl(sdata.result)); 
        }
        
        // 연산자가 '$'이면 종료
        if (sdata.op == '$')
        {
            break;
        }
    }

    // 서버로부터 최대값/최소값 수신
    addr_len = sizeof(server_addr);
    rbyte = recvfrom(s, (char *)&max_val, sizeof(max_val), 0, 
                    (struct sockaddr *)&server_addr, &addr_len);
    if(rbyte == sizeof(max_val))
    {
        max_val = ntohl(max_val);
    }
    
    addr_len = sizeof(server_addr);
    rbyte = recvfrom(s, (char *)&min_val, sizeof(min_val), 0, 
                    (struct sockaddr *)&server_addr, &addr_len);
    if(rbyte == sizeof(min_val))
    {
        min_val = ntohl(min_val);
    }
    
    printf("Disconnected : max=%d min=%d\n", max_val, min_val);

    close(s);
    return 0;
}
