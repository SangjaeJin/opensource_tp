#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define oops(msg) { perror(msg); exit(1);}
#define BUF_SIZE 1000
#define MYPORT 15000      //기본 port (server와 협의 필요)

int main(int argc, char *argv[]) {

    int port;
    
    char message[BUF_SIZE + 1];
    char kb_msg[BUF_SIZE + 10];
    int messlen = 0;
    
    int sock_id; // the file descriptor & the soket id
    
    char hostname[BUF_SIZE];
    struct hostent *hp;
    struct sockaddr_in servadd; // the number to call
    
    int isQuestionReceived = 0; // 이미 설문을 서버로 붙어 전달 받은 상태인가?
    
    // 포트 옵션 주면 따로 포트 설정가능 안주면 MYPORT로 설정.
    if(argc==2 || argc==4){
        if(!strcmp("-p",argv[1])){
            if(argc==2) {
                oops("Invalid parameters.\nUsage: ./chat [-p PORT] HOSTNAME\n");
            }
            else if (argc==4)
            {
                sscanf(argv[2],"%i",&port);
                strcpy(hostname,argv[3]);
            }
        }else{ // argc==3
            port=MYPORT;
            strcpy(hostname,argv[1]);
        }
    }
    // 통신 규약
    printf("\n*** Poll Client starting (enter \"quit\" to stop): \n");
    
    /* 클라이언트 소캣 생성 */
    sock_id = socket(PF_INET, SOCK_STREAM, 0);
    if (sock_id == -1)
        oops("socket");
    printf("### socket is created ###\n");
    
    bzero ((void*)&servadd, sizeof(servadd));
    hp = gethostbyname(hostname);
    if (hp == NULL) {
        printf("### hp == NULL ###\n");
        oops(hostname);
    }
    bcopy(hp->h_addr, (struct sockaddr*)&servadd.sin_addr, hp->h_length);
    servadd.sin_port = htons(port);
    servadd.sin_family = AF_INET;
    printf("### assign server_addr value ###\n");
 //   servadd.sin_addr.s_addr=inet_addr(211.51.176.149);

    
    /* 서버에 연결 */
    if(connect(sock_id, (struct sockaddr *)&servadd, sizeof(servadd)) != 0)
        oops("connect");

    printf("### connect is done ###\n");
    /*서버로 부터 데이터를 읽어 들여 설문을 클라이언트가 받아서 출력시킴*/
    //설문지 출력
    messlen = (int)read(sock_id, message, BUF_SIZE+1);
    if (messlen == -1)
        oops("read");
    
    printf("설문 조사를 시작합니다.");
    if (write(1, message, messlen) != messlen)    // 설문조사지 뿌려줌
        oops("write");
    isQuestionReceived = 1;

    fflush(stdout);
    
    printf("------해당되는 번호를 적어주세요-------\n");  // 설문에 대한 응답 날림
    messlen = (int)read(0, kb_msg, BUFSIZ);
    if (messlen == -1)
        oops("read");
    
    if (write(sock_id, kb_msg, messlen) != messlen)
        oops("write");
    
    if (messlen == 0) // client가 다른 아무것도 안치고 엔터만 쳤다면, 응답 안했으니 결과 안받고 종료
    {
        close(sock_id);
        return 1;
    }
        
    
    while (1) { // 최종 결과 기다림
        if ((messlen = (int)read(sock_id, message, BUFSIZ))>0) // 서버로 부터 결과 받음
        {
            write(1, message, messlen);
            break;
        }
        sleep(2);
    }
    close(sock_id);
    return 0;
}

