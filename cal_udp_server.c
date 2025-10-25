#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define PORT 3800

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
        struct sockaddr_in client_addr, sock_addr;
        int server_sockfd;
        socklen_t addr_len;
        struct cal_data rdata;
        int left_num, right_num, cal_result;
        short int cal_error;

        if( (server_sockfd  = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        {
                perror("Error ");
                return 1;
        }

        memset((void *)&sock_addr, 0x00, sizeof(sock_addr));
        sock_addr.sin_family = AF_INET;
        sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        sock_addr.sin_port = htons(PORT);

        if( bind(server_sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) == -1)
        {
                perror("Error ");
                return 1;
        }

        // 서버가 사용하는 실제 포트 번호 출력
        addr_len = sizeof(sock_addr);
        getsockname(server_sockfd, (struct sockaddr *)&sock_addr, &addr_len);
        printf("UDP Server started on port %d\n", ntohs(sock_addr.sin_port));

        for(;;)
        {
                addr_len = sizeof(client_addr);
                recvfrom(server_sockfd, (void *)&rdata, sizeof(rdata), 0,
                        (struct sockaddr *)&client_addr, &addr_len);
                
                printf("New Client Connect : %s (%d)\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                int max_result = -2147483648; // INT_MIN
                int min_result = 2147483647;  // INT_MAX
                int result_count = 0;

                // 연산자가 '$'일 때까지 계속 통신
                while(1)
                {
                        cal_result = 0;
                        cal_error = 0;

                        left_num = ntohl(rdata.left_num);
                        right_num = ntohl(rdata.right_num);

                        // 16진법으로 수신 데이터 출력
                        printf("FROM CLIENT : ");
                        unsigned char *data_ptr = (unsigned char *)&rdata;
                        for (int i = 0; i < sizeof(rdata); i++)
                        {
                            printf("%02x ", data_ptr[i]);
                        }
                        printf("\n");

                        switch(rdata.op)
                        {
                                case '+':
                                        cal_result = left_num + right_num;
                                        break;
                                case '-':
                                        cal_result = left_num  - right_num;
                                        break;
                                case 'x':
                                        cal_result = left_num * right_num;
                                        break;
                                case '/':
                                        if(right_num == 0)
                                        {
                                                cal_error = 2;
                                                break;
                                        }
                                        cal_result = left_num / right_num;
                                        break;
                                case '$':
                                        // 종료 신호
                                        cal_error = 0;
                                        cal_result = 0;
                                        break;
                                default:
                                        cal_error = 1;
                        }
                        
                        rdata.result = htonl(cal_result);
                        rdata.error = htons(cal_error);
                        
                        // 16진법으로 송신 데이터 출력
                        printf("TO CLIENT : ");
                        unsigned char *send_ptr = (unsigned char *)&rdata;
                        for (int i = 0; i < sizeof(rdata); i++)
                        {
                            printf("%02x ", send_ptr[i]);
                        }
                        printf("\n");
                        
                        if (rdata.op != '$' && cal_error == 0)
                        {
                                printf("%d %c %d = %d\n", left_num, rdata.op, right_num, cal_result);
                                
                                // 최대값/최소값 업데이트
                                if (cal_result > max_result)
                                        max_result = cal_result;
                                if (cal_result < min_result)
                                        min_result = cal_result;
                                result_count++;
                        }
                        
                        // UDP로 응답 전송
                        sendto(server_sockfd, (void *)&rdata, sizeof(rdata), 0,
                               (struct sockaddr *)&client_addr, addr_len);
                        
                        // 연산자가 '$'이면 종료
                        if (rdata.op == '$')
                        {
                                break;
                        }

                        // 다음 요청 수신
                        addr_len = sizeof(client_addr);
                        recvfrom(server_sockfd, (void *)&rdata, sizeof(rdata), 0,
                                (struct sockaddr *)&client_addr, &addr_len);
                }

                // 클라이언트 종료 시 최대값/최소값 계산 및 출력
                if (result_count > 0)
                {
                        printf("Disconnected : max=%d min=%d\n", max_result, min_result);
                        
                        // 클라이언트에게 최대값/최소값 전송
                        int max_val_net = htonl(max_result);
                        int min_val_net = htonl(min_result);
                        sendto(server_sockfd, (void *)&max_val_net, sizeof(max_val_net), 0,
                               (struct sockaddr *)&client_addr, addr_len);
                        sendto(server_sockfd, (void *)&min_val_net, sizeof(min_val_net), 0,
                               (struct sockaddr *)&client_addr, addr_len);
                }
                else
                {
                        printf("Client disconnected. No valid calculations performed.\n");
                }
        }

        close(server_sockfd);
        return 0;
}