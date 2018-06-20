//
//  server.c
//  Practice
//
//  Created by 진상재 on 2018. 6. 2..
//  Copyright © 2018년 진상재. All rights reserved.
//

#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <strings.h>
#include <netdb.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <arpa/inet.h>

#define HOSTLEN 256
#define oops(msg) { perror(msg); exit(1);}
#define MAX_CLNT 3
#define BUF_SIZE 1000

typedef struct{
    int optionNum,votes;
}Vote;

int otherOption,num_otherOption=0;
char otherOptString[BUF_SIZE][BUF_SIZE];
char buf[BUF_SIZE];
Vote result[10];
int options,responded;

int messlen=0;
int clnt_cnt=0;                                                //접속한 클라이언트수 카운트
int clnt_socks[MAX_CLNT];

pthread_mutex_t mutx;

void sort(Vote arr[]){
    for(int i=0;i<options-1;i++){
        for(int j=0; j< options-i-1;j++){
            if(result[j].votes<result[j+1].votes){
                Vote temp =result[j];
                result[j]= result[j+1];
                result[j+1]=temp;
            }
        }
    }
}

void* receive_ans(void* arg);

int main(int argc ,char *argv[]){
    int server_sock_id;
    int clnt_sock_id;          // 클라이언트의 소켓 정보 저장 용도
    struct sockaddr_in serveraddr,clntaddr;
    char question[1000]={0};
    pthread_t t_id;
    
    pthread_mutex_init(&mutx, NULL);
    
    if(argc!=2){
        printf("usage: %s <port>\n",argv[0]);
        exit(-1);
    }
    /*
     * step1 :소켓 획득
     */
    server_sock_id = socket(PF_INET, SOCK_STREAM, 0);
    if(server_sock_id == -1)
        oops("socket");
    
    /*
     *  step2 프로토콜 설정, 초기화작업들
     */
    
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_port= htons(atoi(argv[1]));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr  = htonl(INADDR_ANY);                               // 자동으로 ip가 할당된다....
    
    if(bind(server_sock_id, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) !=0 )
        oops("bind");
    
    /*
     *   step3 대기상태. listen의 2번째 인자만큼의 클라이언트가 들어올 수 있음,
     */
    if( listen(server_sock_id, MAX_CLNT) !=0)
        oops("listen");
    
    
    // 설문조사할 질문을 입력받음 , 마지막 선택지는 주관식으로 입력받음.
    /*
     예를 들어, 1,2,3,4 번의 선택지가 있을 시 , 클라이언트가 4개의 선택지모두 마음에 들지 않을 시,
     5번을 입력하고 , 다른 의견을 입력할 수 있음.
     */
    printf("설문조사를 입력하세요. \n");
    fgets(question, 1000, stdin);
    question[sizeof(question)-1]=0;
    printf("제대로 입력받았습니다. 마지막 선택지의 번호가 몇번인가요?\n");
    scanf("%d",&options);
    otherOption = options;
    printf("제대로 입력받았습니다. 설문조사를 전송합니다\n");
    
    /*
     한 사람씩 연결해 설문조사를 던지는 부분.
     */
    for(int i=0;i<MAX_CLNT;i++){
        
        // accept: 클라이언트와의 연결을 기다림. 연결되면 그 클라이언트의 id가 반환됨
        clnt_sock_id = accept(server_sock_id, (struct sockaddr*)NULL, NULL);
        if(clnt_sock_id==-1)
            oops("accept");
        
        sprintf(buf,"%d",options);                      //option의 개수를 client에 넘겨줌
        write(clnt_sock_id,buf,BUF_SIZE);
        
        /*
         여기서 뮤텍스를 쓰는 이유는 밑에서 쓰레드로 함수실행해서 전역변수인 clnt_cnt에 접근하기 때문
         */
        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++]= clnt_sock_id;             // 한명씩 클라이언트의 수 증가시켜줌. 클라이언트가 다수기 때문에, 클라이언트가 응답을 하지 않을시 삭제해야 되기때문에 배열 선언.
        pthread_mutex_unlock(&mutx);
        
        write(clnt_sock_id, question, sizeof(question));
        
        
        /*
         쓰레드를 생성하는 이유는
         질문을 한사람씩 던지기 때문에 만약 질문을 받은 사람이 대답을 하지 않으면  block 상태가 되기 떄문.
         따라서 쓰레드로 독립적인 함수실행이 필요하다.
         */
        pthread_create(&t_id, NULL, receive_ans, (void*)&clnt_sock_id);
        pthread_detach(t_id);                                                // 메인쓰레드에서 분리
        printf("연결된 클라이언트: No.%d from <%s>\n", clnt_cnt, inet_ntoa(clntaddr.sin_addr));    //새로 연결된 클라이언트 정보
    }
    
    while(1)                           // 모든 클라이언트가 응답을 다 할때까지 blocked상태로 유지
    {
        if(responded==MAX_CLNT)
            break;
        sleep(2);
    }
    
    /*
     result[0].votes :1번을 선택한 사람의 수, result[1].votes:2번을 선택한 사람의 수  ....
     result[0].optionNum :1 , result[1].optionNum :2....
     votes의 내림차순으로 정렬
     */
    
    for(int i=0;i<options;i++)
        result[i].optionNum= i+1;
    sort(result);
    
    /*
     응답한 클라이언트에게만 설문조사 결과를 전송.
     */
    int k;
    
    for(int i=0;i<clnt_cnt;i++){
        
        write(clnt_socks[i],"결과는 ...\n",sizeof("결과는 ...\n"));
        
        for(int j=0;j<options-1;j++)
        {
            if(j!=0 &&result[j].votes ==result[j-1].votes ){
                
                k=j;
                for ( ; k<options-1 && result[k].votes ==result[k-1].votes  ; k++);
                
                for(int a =j;a<k;a++){
                    sprintf(buf,"%d위: %d번 [ %d표]\n",j,result[a].optionNum,result[a].votes);
                    write(clnt_socks[i],buf,sizeof(buf));
                }
                j=k-1;
                
            }
        
            
            else{
                sprintf(buf,"%d위: %d번 [ %d표]\n",j+1,result[j].optionNum,result[j].votes);
                write(clnt_socks[i],buf,sizeof(buf));
            }
        }
        if(num_otherOption!=0)                              //기타 의견이 있을시 함께 출력
        {
            write(clnt_socks[i],"기타 의견은...\n",sizeof("기타 의견은...\n"));
            for(int k=0; k<num_otherOption; k++)
                write(clnt_socks[i],otherOptString[k],sizeof(otherOptString[k]));
            write(clnt_socks[i],"가(이) 있습니다.\n",sizeof("가(이) 있습니다.\n"));
        }
        close(clnt_socks[i]);
    }
    
    close(server_sock_id);
    printf("설문조사를 종료합니다.");
    return 0;
}

void* receive_ans(void* arg){
    int clnt_sock_id = *((int*)arg);
    int str_len =0,i;
    char msg[BUF_SIZE];
    char name[BUF_SIZE];
    
    messlen = (int)read(clnt_sock_id,name,BUF_SIZE);              //클라이언트의 이름을 입력 받음
    name[messlen-1]='\0';
    
    /*
     클라이언트에게서 입력받은 결과로 votes를 증가.
     만약 read가 0을 반환할시 아무 입력도 하지 않은 걸로 간주하고,
     if문 탈출.
     */
    if((str_len=(int)read(clnt_sock_id, msg, sizeof(msg)))!=0){
        
        printf("%s님이 설문조사에 응했습니다\n",name);
        pthread_mutex_lock(&mutx);
        responded++;
        
        if(atoi(msg)==otherOption)                                               //기타 의견일시
        {
            char other[BUF_SIZE];
            read(clnt_sock_id,other,BUF_SIZE);              //기타 의견 받음
            strcpy(otherOptString[num_otherOption],other);  //의견을 배열 otherOptString에 저장
            num_otherOption++;
            
            pthread_mutex_unlock(&mutx);                          //종료 전 mutex를 unlock
            
            return NULL;
        }
        result[atoi(msg)-1].votes++;
        pthread_mutex_unlock(&mutx);
        return NULL;
    }
    
    /*
     아무 입력안했기 때문에 클라이언트 목록에서 제거.
     전역변수 clnt_cnt에 접근하기 때문에 뮤텍스 필요
     */
    
    printf("%s님이 설문조사에 응하지 않았습니다\n",name);
    pthread_mutex_lock(&mutx);                                //클라이언트 제거동작중 메모리접근 보호
    for(i=0; i<clnt_cnt; i++)   {                            //remove disconnected client
        if(clnt_sock_id==clnt_socks[i])    {                    //종료한 클라이언트소켓 필터링
            while(i++<clnt_cnt)                     //클라이언트 배열에서 종료한 클라이언트 뒤에있는 클라이언트들 하나씩 앞으로
            {
                clnt_socks[i]=clnt_socks[i+1];
            }
            break;
        }
    }
    responded++;
    clnt_cnt--;                                                //종료한 클라이언트 있기때문에 전체 클라이언트수 -1
    pthread_mutex_unlock(&mutx);                            //클라이언트 제거중 메모리접근 보호 해제
    close(clnt_sock_id);
    return NULL;
}
