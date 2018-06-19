#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>

#define oops(msg) { perror(msg); exit(1);}
#define BUF_SIZE 1000
#define OPTION 3

int option_num;
char otherOption[BUF_SIZE];

int main(int argc, char *argv[]) {
    
    char message[BUF_SIZE];
    char kb_msg[BUF_SIZE]="";
    int messlen = 0;
    
    int sock_id; // the file descriptor & the soket id
    
    struct sockaddr_in servadd; // the number to call
    
    
    if(argc!=4) {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }
    
    // 통신 규약
    printf("\n*** Poll Client starting (enter \"quit\" to stop): \n");
    
    /* 클라이언트 소캣 생성 */
    sock_id = socket(PF_INET, SOCK_STREAM, 0);   // ~~~~~~~~
    if (sock_id == -1)
        oops("socket");
    
    memset(&servadd,0,sizeof(servadd));
    servadd.sin_family=AF_INET;
    servadd.sin_addr.s_addr=inet_addr(argv[1]);
    servadd.sin_port=htons(atoi(argv[2]));
    
    /* 서버에 연결 */
    if(connect(sock_id, (struct sockaddr *)&servadd, sizeof(servadd)) != 0)
        oops("connect");
    
    read(sock_id,message,BUF_SIZE);         //option의 개수를 server로부터 받음
    option_num=atoi(message);
    
    /*서버로 부터 데이터를 읽어 들여 설문을 클라이언트가 받아서 출력시킴*/
    //설문지 출력
    messlen = (int)read(sock_id, message, BUF_SIZE+1);
    message[sizeof(message)-1]='\0';
    
    if (messlen == -1)
        oops("read");
    
    printf("설문 조사를 시작합니다.");
    printf("%s\n",message);                 //설문 조사지 뿌려줌
    
    
    write(sock_id , argv[3], sizeof(argv[3]));
    
    printf("------해당되는 번호를 적어주세요-------\n");  // 설문에 대한 응답 날림
    messlen = (int)read(0, kb_msg, BUFSIZ);
    kb_msg[sizeof(kb_msg)-1]='\0';
    if (messlen == -1)
        oops("read");
    
    if(atoi(kb_msg) == option_num+1)
    {
        printf("기타 의견을 적어주세요\n");
        fgets(otherOption,BUF_SIZE,stdin);              //기타 의견일시 미리 버퍼에 담아둠
    }
    if (kb_msg[0]=='\n') // client가 다른 아무것도 안치고 엔터만 쳤다면, 응답 안했으니 결과 안받고 종료
    {
        close(sock_id);
        
        return 1;
    }
    
    if (write(sock_id, kb_msg, messlen) == messlen)
    {
        if(atoi(kb_msg) == option_num+1)
        {
            write(sock_id,otherOption,BUF_SIZE);    //기타의견 서버에 전송
        }
    }
    else
        oops("write");
    
    
    while (1){ // 최종 결과 기다림
        if ((messlen = (int)read(sock_id, message, BUF_SIZE))>0) // 서버로 부터 결과 받음
        {
            write(1, message, messlen);
            
            while((messlen =(int)read(sock_id,message,BUF_SIZE))!=0)
            {
                write(1,message,messlen);
            }
            break;
        }
        sleep(2);
    }
    close(sock_id);
    return 0;
}

