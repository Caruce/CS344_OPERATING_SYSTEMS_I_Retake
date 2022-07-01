/***********************************************************
* File name   : enc_server.c
* Version     : 0.1
* Date        : 2021.05.27
* Description : 1. 
*               2.
* Others      : none
* History     : none
***********************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#ifdef _WIN32
#include <process.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h> 
#include <unistd.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include <sys/wait.h>  
#endif

#ifndef MOD_SOCKET_
#define MOD_SOCKET_  "server"
#endif

#ifndef _WIN32
typedef int sock_t;
typedef int handle;
#else
#include <winsock2.h>
typedef HANDLE handle;
typedef SOCKET sock_t;
typedef HANDLE pthread_t;
#endif

#define IN
#define OUT
#define IO

#ifdef _WIN32
#define MSG_NOSIGNAL  0
#endif

#ifndef errno
extern int errno;
#endif

struct SHeader
{
  int               m_nIsOk;
  int               n_ntype;
  int               m_nItemSize;
  int               m_nItemCount;
  int               m_nBodyLen;
  int               m_nCurrIndex;
};

typedef struct {
        char let;
        int seq;
} Letter;

//to assign each letter a numerical value, e.g., "A" is 0, "B" is 1, and so on
Letter letters[] = {
        {'A', 0},
        {'B', 1},
        {'C', 2},
        {'D', 3},
        {'E', 4},
        {'F', 5},
        {'G', 6},
        {'H', 7},
        {'I', 8},
        {'J', 9},
        {'K', 10},
        {'L', 11},
        {'M', 12},
        {'N', 13},
        {'O', 14},
        {'P', 15},
        {'Q', 16},
        {'R', 17},
        {'S', 18},
        {'T', 19},
        {'U', 20},
        {'V', 21},
        {'W', 22},
        {'X', 23},
        {'Y', 24},
        {'Z', 25},
        {' ', 26}
};


/***********************************************************
* Function    : SOCKET_Send
* Description : To send data
* Input       : In order, handle, data content, and data length
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_Send(IN sock_t nServerHandle, IN char *pBuf, IN int nLen)
{
    int             nsend_len   = 0;
    int    ntotal_len           = 0;

    if (NULL == pBuf)
    {
        printf("[%s] %d input error \n", MOD_SOCKET_, __LINE__);
        return -1;
    }
    
    for (;ntotal_len < nLen;)
    {
        nsend_len = send(nServerHandle, pBuf + ntotal_len, 
                         nLen - ntotal_len, MSG_NOSIGNAL);
        if (0 == nsend_len)
        {
            return nsend_len;
        }
        if (-1 == nsend_len)
        {
            if ((errno == EAGAIN || errno == EINTR))
            {
                continue ;
            }
            else
            {
                return nsend_len;
            }
        }
        ntotal_len += nsend_len;        
    }
     
    return ntotal_len;
}


/***********************************************************
* Function    : SOCKET_RecvByLen
* Description : To send data
* Input       : In order, handle, data content, and data length
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_RecvByLen
(
    IN      sock_t      nServerHandle, 
    OUT     char       *pBuf, 
    IN      int         nLen
)
{
    int             nrecv_len  = 0;
    int             ntotal_len = 0;
    fd_set fds;
    struct timeval tv, *tvptr;
    //int nlen = 0;

    if (nServerHandle < 0 || NULL == pBuf)
    {
        printf("[%s] %d input error \n", MOD_SOCKET_, __LINE__);
        return -1;
    }

    tv.tv_sec   = 0;
    tv.tv_usec  = 0;

    FD_ZERO(&fds);
    FD_SET(nServerHandle, &fds);
    tv.tv_sec   = 1;
    tvptr       = &tv; 

    //if (select(nServerHandle + 1, &fds, NULL, NULL, tvptr) > 0) 
    {
        for (;ntotal_len < nLen;)
        {
			printf("begin recv nLen = %d\n", nLen);
            nrecv_len = recv(nServerHandle, pBuf + ntotal_len, 
                             nLen - ntotal_len, 0);
            if (0 == nrecv_len)
            {
                return nrecv_len;
            }

            if (-1 == nrecv_len)
            {
                if ((errno == EAGAIN || errno == EINTR))
                {
                    continue ;
                }
                else
                {
                    return nrecv_len;
                }
            }
            ntotal_len += nrecv_len;
        }
    }
    printf("recv len = %d\n", nrecv_len);
    return nrecv_len;
}


/***********************************************************
* Function    : SOCKET_RecvNoLen
* Description : To recive data
* Input       : In order, handle, data content, and data length
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_RecvNoLen
(
    IN      sock_t  nServerHandle, 
    OUT     char   *pBuf, 
    IN      int     nLen
)
{
    fd_set fds;
    struct timeval tv, *tvptr;
    int nlen = 0;

    if (NULL == pBuf)
    {
        printf("[%s] %d input error \n", MOD_SOCKET_, __LINE__);
        return -1;
    }

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(nServerHandle, &fds);
    tv.tv_sec = 1;
    tvptr = &tv; 

    if (select(nServerHandle + 1, &fds, NULL, NULL, tvptr) > 0) 
    {
        do 
        {
            nlen = recv(nServerHandle, pBuf, nLen, 0);
        } 
        while (nlen == -1 && (errno == EAGAIN || errno == EINTR));  

        if (nlen > 0) 
        {
            return nlen;
        } 
        else
        {
            return -1;
        }
    }
   
    return -1;
}


/***********************************************************
* Function    : SOCKET_Send
* Description : 
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int get_seq(char letter)
{
        int i;
        for(i = 0; i < 27; i++)
        {
                if(letter == letters[i].let)
                        return letters[i].seq;
        }
        return -1;
}

/***********************************************************
* Function    : SOCKET_Send
* Description : 
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
char get_letter(int seq)
{
        if(seq >= 0 && seq <= 26)
        {
                return letters[seq].let;
        }
        return '\0';
}


/***********************************************************
* Function    : SOCKET_Send
* Description : 
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_IsSocketClosed(IN sock_t nServerSocket)  
{  
    int nret    = 0;  
    int ndwret  = 0;
    
#ifdef WIN32
    handle nclose_event = WSACreateEvent();  
    WSAEventSelect(nServerSocket, nclose_event, FD_CLOSE);  

    ndwret = WaitForSingleObject(nclose_event, 0);  

    if(ndwret == WSA_WAIT_EVENT_0)  
    {
        printf("socket disconnected = %d\n", nServerSocket);
        nret = 1;  
    }
    else if(ndwret == WSA_WAIT_TIMEOUT)  
    {
        nret = 0;  
    }
    WSACloseEvent(nclose_event);  
#else
    struct tcp_info info;
     int nlen = sizeof(info);
    if (nServerSocket <= 0)
    {
        printf("socket disconnected = %d\n", nServerSocket);
        nret = 1;
    }
    memset(&info,0,sizeof(info));
    getsockopt(nServerSocket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&nlen);
    if (info.tcpi_state == 1) 
    {
        printf("socket connected = %d\n", nServerSocket);
        nret = 0;
    } 
    else
    {
        printf("socket disconnected = %d\n", nServerSocket);
        nret = 1;
    }
#endif

    return nret;  
} 


/***********************************************************
* Function    : SOCKET_Send
* Description : 
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_PrintfServerVersion(void)
{
    printf("%s\n", "** socket server version is [1.0.0]");
    
    return 0;
}


/***********************************************************
* Function    : SOCKET_Send
* Description : 
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_Error(void)
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}


/***********************************************************
* Function    : SOCKET_Close
* Description : Close the socket
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_Close(IN sock_t nServerHandle)
{
#ifdef _WIN32
    closesocket(nServerHandle);
#else
    close(nServerHandle);
#endif
    return 0;
}


/***********************************************************
* Function    : SOCKET_ShutDown
* Description : Close the socket
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_ShutDown(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

void handler(int num) {   
    int status;   
    int pid = waitpid(-1, &status, WNOHANG);   
    if (WIFEXITED(status)) {   
        //printf("The child %d exit with code %d\n", pid, WEXITSTATUS(status));   
    }   
}  

int HandleBc(sock_t nSocket, void *pArgv)
{
    printf("BcFun %d\n", nSocket);
    return 0;
}


int ENC_SocketListen(int nPort)
{
     struct sockaddr_in serAddr;
     int        nres                    =   0;
     //pthread_t  pthread                 =   0;
#ifdef _WIN32
    WSADATA		        wsData;
#endif
    //struct SLinkInfo    *plink          = NULL;
    sock_t              nsocket_handle  = 0;

#ifdef _WIN32        
	nres = WSAStartup(MAKEWORD(2,2),&wsData);	
#endif

	nsocket_handle = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
	if (INVALID_SOCKET == nsocket_handle)
	{
		return -1;
	}
#else
    if (nsocket_handle < 0)
    {
        printf("[%s] %d build socket fail \n", MOD_SOCKET_, __LINE__);
        return -1;
    }
#endif

    memset(&serAddr, 0x0, sizeof(struct sockaddr_in));
	serAddr.sin_family          =   AF_INET;
	serAddr.sin_port            =   htons(nPort);
	serAddr.sin_addr.s_addr     =   htonl(INADDR_ANY);
	
	int opt =1;
    setsockopt(nsocket_handle, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    
	nres = bind(nsocket_handle, 
                (struct sockaddr*)&serAddr, 
                sizeof(serAddr));
#ifdef _WIN32
	if (SOCKET_ERROR == nres)
	{
        printf("bind error\n");
		return -1;	
	}
#else
    if (nres < 0)
    {
        printf("bind error\n");
        return -1;
    }
#endif
        	
	nres = listen(nsocket_handle, 5);
#ifdef _WIN32
	if (SOCKET_ERROR == nres)
	{
      printf("listen error\n");
		return -1;
	}
#else
    if (nres < 0)
    {
        printf("listen error\n");
        return -1;
    }
#endif
    return nsocket_handle;
}


int main(int argc, char **argv)
{
    //sock_t              naccept     = 0;
    sock_t              nserverfd   = 0;
    sock_t              nclientfd   = 0;
    int                 child_pid   = 0;
    int                 nport       = 0;
	struct sockaddr_in client_addr;
    int sin_size;
    char *phead_data = NULL;
    struct SHeader *phead = NULL;
    //char arecv_buf[1024*1024] = {0};
    int nindex = 0;
	int nres = 0;
	char *pRecvData = NULL;
	//int nbreak = 0;
	char *psend_data = NULL;
	char *prcv_file_data = NULL;
	char *prcv_key_data = NULL;
	int keyLen = 0;
	int plainLen = 0;
	int i = 0;
	int plainSeq = 0;
	int 	keySeq = 0;
	int cipherSeq = 0;
	char ciphertextBuf[1024*1024] = {0};
	char atmp_buf[1024] = {0};
	int ntotal_type = 0;
	
    if (argc < 2)
    {
        printf("argc < 2\n");
        return -1;
    }
    nport = atoi(argv[1]);
    nserverfd = ENC_SocketListen(nport);
	signal(SIGCHLD, handler);

    for(;;)
    {
        sin_size = sizeof(client_addr);
        memset(&client_addr,0,sizeof(client_addr));
        
        if((nclientfd = accept(nserverfd, (struct sockaddr*)&client_addr, &sin_size))<0)
        {
            if(errno == EINTR || errno == ECONNABORTED)
                continue;
            else
            {
                return -4;
            }
        }

        if((child_pid = fork())==0)
        {
		    for (;;)
			{
				phead_data = (char*)malloc(1024);
				memset(phead_data, 0x0, 1024);
				//nres = SOCKET_RecvByLen(nclientfd, phead_data, sizeof(struct SHeader));
				//printf("begin read\n");
				nres = recv(nclientfd, phead_data, sizeof(struct SHeader), 0);
				//printf("main recv len = %d\n", nres);
				phead = (struct SHeader*)phead_data;

				//printf("recv %d %d %d %d %d\n", phead->n_ntype, phead->m_nItemSize, phead->m_nItemCount, phead->m_nBodyLen, phead->m_nCurrIndex);
				
				pRecvData = (char*)malloc(phead->m_nBodyLen);
				memset(pRecvData, 0x0, phead->m_nBodyLen);
				for (nindex = 0; nindex < phead->m_nItemCount; nindex++)
				{
					nres = recv(nclientfd, pRecvData+nindex*1000, 1000, 0);
					//printf("data recv len = %d\n", nres);
				}
				
				if (phead->n_ntype == 1)
				{
					prcv_file_data = pRecvData;
				}
				if (phead->n_ntype == 2)
				{
					prcv_key_data = pRecvData;
				}
				if (phead->n_ntype > 2)
				{
					ntotal_type = 1;
				}
				//printf("recv file = %s\n", prcv_file_data);
				//printf("recv key = %s\n", prcv_key_data);
				
				if (ntotal_type == 1)
				{
					//printf("duan kou bu  dui \n");
					psend_data = (char*)malloc(sizeof(struct SHeader));
					memset(psend_data, 0x0, sizeof(struct SHeader));
					phead = (struct SHeader*)psend_data;
					phead->m_nIsOk = 300;
					phead->n_ntype = 8;
					phead->m_nBodyLen = 0;
					phead->m_nCurrIndex = 1;
					phead->m_nItemSize = 1000;
					phead->m_nItemCount =  0;
					nres = SOCKET_Send(nclientfd, psend_data, sizeof(struct SHeader));	
					//printf("ok = %d, type = %d, size = %d\n", phead->m_nIsOk, phead->n_ntype, phead->m_nItemSize);
					close(nclientfd);
					return 1;
				}
				else
				{
					psend_data = (char*)malloc(sizeof(struct SHeader));
					memset(psend_data, 0x0, sizeof(struct SHeader));
					phead = (struct SHeader*)psend_data;
					phead->n_ntype = 9;
					nres = SOCKET_Send(nclientfd, psend_data, sizeof(struct SHeader));
				}
				
				if (NULL != prcv_file_data && NULL != prcv_key_data)
				{
					plainLen = strlen(prcv_file_data) - 1;
					keyLen = strlen(prcv_key_data);
					for(i = 0; i < keyLen; i++)
					{
						if(i >= plainLen)
						{
								break;
						}
						//from letter to sequence
						plainSeq = get_seq(prcv_file_data[i]);
						keySeq = get_seq(prcv_key_data[i]);
						//calculate the cipher sequence
						cipherSeq = (plainSeq + keySeq) % 27;
						//from sequence to letter
						ciphertextBuf[i] = get_letter(cipherSeq);
					}
					ciphertextBuf[i] = '\0';
					//printf("key .. src = %s\n", ciphertextBuf);
					
					memset(psend_data, 0x0, sizeof(struct SHeader));
					phead = (struct SHeader*)psend_data;
					phead->m_nIsOk = 200;
					phead->n_ntype = 3;
					phead->m_nBodyLen = strlen(ciphertextBuf)+1;
					phead->m_nCurrIndex = 1;
					phead->m_nItemSize = 1000;
					phead->m_nItemCount =  phead->m_nBodyLen/1000 + 
												((phead->m_nBodyLen%1000 > 0)?1:0);
					nres = SOCKET_Send(nclientfd, psend_data, sizeof(struct SHeader));		
					for (nindex = 0; nindex < phead->m_nItemCount; nindex++)
					{
						memset(atmp_buf, 0x0, sizeof(atmp_buf));
						strncpy(atmp_buf, ciphertextBuf, 1000);
						
						//printf("begin send miwen data = %s\n", atmp_buf);
						nres = SOCKET_Send(nclientfd, atmp_buf, strlen(atmp_buf));
						phead->m_nCurrIndex++;
					}
					
					prcv_file_data = NULL;
					prcv_key_data = NULL;
					close(nclientfd);
					return 1;
				}
				//sleep(1);
				
			}
        }
        else if (child_pid > 0)
        {
            close(nclientfd);
        }
    }
    return 0;
}



