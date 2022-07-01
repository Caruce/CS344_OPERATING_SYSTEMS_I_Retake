/***********************************************************
* File name   : dec_client.c
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
#include <sys/stat.h>
#endif

#ifndef MOD_SOCKET_
#define MOD_SOCKET_  "server"
#endif

#define IN
#define OUT
#define IO

#ifndef _WIN32
typedef int sock_t;
#else
#include <winsock2.h>
typedef SOCKET sock_t;
#endif

#ifdef _WIN32
#define MSG_NOSIGNAL  0
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

/***********************************************************
* Function    : SOCKET_Connect
* Description : 
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_Connect
(
    IN sock_t       nClientHandle, 
    IN unsigned int nPort, 
    IN char        *pServerIp
)
{
    int nres = 0;			
	struct sockaddr_in serAddr;
    
	memset(&serAddr, 0x0, sizeof(struct sockaddr_in));
	serAddr.sin_family          =   AF_INET;
	serAddr.sin_port            =   htons(nPort);
    serAddr.sin_addr.s_addr     =   inet_addr(pServerIp);

	for (;;)
	{
		nres = connect(nClientHandle, 
		               (struct sockaddr*)&serAddr, 
		               sizeof(serAddr));
#ifdef _WIN32				
		if(SOCKET_ERROR == nres)                        //Handling connection errors
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK  == nErrCode ||
				WSAEINVAL       == nErrCode)            //The connection is not complete
			{
				continue;
			}
            else if (WSAEISCONN == nErrCode)            //The connection has been completed
			{
				break;
			}
            else                                        //For other reasons, the connection failed
			{
				return -1;
			}
		}	
#endif
		if (nres == 0)                                  //The connection is successful
		{
			break;	
		}
	}

	return 1;
}


/***********************************************************
* Function    : SOCKET_ConnectTime
* Description : 
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_ConnectTime
(
    IN sock_t       nClientHandle, 
    IN unsigned int nPort, 
    IN char        *pServerIp,
	IN int          nSecond
)
{
    int nres = 0;			
	struct sockaddr_in serAddr;
    
	memset(&serAddr, 0x0, sizeof(struct sockaddr_in));
	serAddr.sin_family          =   AF_INET;
	serAddr.sin_port            =   htons(nPort);
    serAddr.sin_addr.s_addr     =   inet_addr(pServerIp);

	for (;;)
	{
		nres = connect(nClientHandle, 
		               (struct sockaddr*)&serAddr, 
		               sizeof(serAddr));
#ifdef _WIN32				
		if(SOCKET_ERROR == nres)                        //Handling connection errors
		{
			int nErrCode = WSAGetLastError();
			if (WSAEWOULDBLOCK  == nErrCode ||
				WSAEINVAL       == nErrCode)            //The connection is not complete
			{
				continue;
			}
            else if (WSAEISCONN == nErrCode)            //The connection has been completed
			{
				break;
			}
            else                                        //For other reasons, the connection failed
			{
				return -1;
			}
		}	
#endif
		if (nres == 0)                                  //The connection is successful
		{
			break;	
		}
	}

	return 1;
}


/***********************************************************
* Function    : SOCKET_NewClient
* Description : 
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
sock_t SOCKET_NewClient(IN unsigned int nPort, IN char *pServerIp)
{
    int			nres = 0;	
    
#ifdef _WIN32
	WSADATA		wsData;
#endif

    sock_t          nsocket_handle      = 0;
    
#ifdef _WIN32      
	nres = WSAStartup(MAKEWORD(2,2),&wsData);	
#endif

	nsocket_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#ifdef _WIN32
	if (INVALID_SOCKET == nsocket_handle)
	{
		return -1;
	}
#else
    if (-1 == nsocket_handle)
    {
        return -1;
    }
#endif

    nres = SOCKET_Connect(nsocket_handle, nPort, pServerIp);
    if (-1 == nres)
    {
        return -1;
    }
    
	return nsocket_handle;
}


/***********************************************************
* Function    : SOCKET_Send
* Description : To send data
* Input       : In order, handle, data content, and data length
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_Send(IN sock_t nClientHandle, IN char *pBuf, IN int nLen)
{
    int             nsend_len   = 0;
    int    ntotal_len           = 0;
    struct timeval  tovertime;
  
    for (;ntotal_len < nLen;)
    {
        nsend_len = send(nClientHandle, pBuf + ntotal_len, 
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
    IN      sock_t      nClientHandle, 
    OUT     char        *pBuf, 
    IN      int         nLen
)
{
    int             nrecv_len  = 0;
    int             ntotal_len = 0;
    struct timeval  tovertime;
    fd_set fds;
    struct timeval tv, *tvptr;
    int nlen = 0;

    if (nClientHandle < 0 || NULL == pBuf)
    {
        //printf("[%s] %d input error \n", MOD_SOCKET_, __LINE__);
        return -1;
    }

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(nClientHandle, &fds);
    tv.tv_sec = 1;
    tvptr = &tv; 

    if (select(nClientHandle + 1, &fds, NULL, NULL, tvptr) > 0) 
    {
        for (;ntotal_len < nLen;)
        {
            nrecv_len = recv(nClientHandle, pBuf + ntotal_len, 
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
int SOCKET_RecvNoLen(IN sock_t nClientHandle, OUT char *pBuf, IN int nLen)
{
    fd_set fds;
    struct timeval tv, *tvptr;
    int nlen = 0;

    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(nClientHandle, &fds);
    tv.tv_sec = 1;
    tvptr = &tv; 

    if (select(nClientHandle + 1, &fds, NULL, NULL, tvptr) > 0) 
    {
        do 
        {
            nlen = recv(nClientHandle, pBuf, nLen, 0);
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
* Function    : SOCKET_IsSocketClosed
* Description : Determine if the link is broken
* Input       : 
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int SOCKET_IsSocketClosed(IN sock_t nClientSocket)  
{  
    int nret    = 0;  
    int ndwret  = 0;
    
#ifdef WIN32
    HANDLE nclose_event = WSACreateEvent();  
    WSAEventSelect(nClientSocket, nclose_event, FD_CLOSE);  

    ndwret = WaitForSingleObject(nclose_event, 0);  

    if(ndwret == WSA_WAIT_EVENT_0)  
    {
        //printf("socket disconnected \n");
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
    if (nClientSocket <= 0)
    {
        nret = 1;
    }
    memset(&info,0,sizeof(info));
    getsockopt(nClientSocket, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&nlen);
    if (info.tcpi_state == 1) 
    {
        nret = 0;
    } 
    else
    {
        //printf("socket disconnected = %d\n", nServerSocket);
        nret = 1;
    }
#endif

    return nret;  
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
int SOCKET_Close(IN sock_t nClientHandle)
{
#ifdef _WIN32
    closesocket(nClientHandle);
#else
    close(nClientHandle);
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


/***********************************************************
* Function    : FILE_ReadData
* Description : Fetching file data
* Input       : In order, the file name, size, received data, offset
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int FILE_ReadData
(
    char    *pFilePathAndName, 
    int     nIsByBin,
    int     nSize, 
    int     nCount, 
    char    *pBuf, 
    int     nOffSet
)
{
    FILE *pfile = NULL;
    int nres = 0;

    if (1 == nIsByBin)
    {
        pfile = fopen(pFilePathAndName, "rb");
    }
    else
    {
        pfile = fopen(pFilePathAndName, "r");
    }
    
    if (NULL == pfile)
    {
        return -1;
    }
    
    if (0 != nOffSet)
    {
        fseek(pfile, nOffSet, 0);
    }
    nres = fread(pBuf, nSize, nCount, pfile);
    
    fclose(pfile);
    
    return nres;
}

/***********************************************************
* Function    : FILE_GetFileSize
* Description : Get file size
* Input       : file name
* Output      : no
* Return      : no
* Date        : 2021.05.28
* Others      : none
***********************************************************/
int FILE_GetFileSize(char *pFilePathAndName)
{
    struct stat buf;
	
	if (NULL == pFilePathAndName)
	{
		//printf("00001\n");
		return -1;
	}

	if(stat(pFilePathAndName, &buf) != 0)
	{
		//printf("00002\n");
		return -1;
	}

	return (int)buf.st_size;
}


int main(int argc, char **argv)
{
    sock_t nhandle = 0;
    int nres = 0;
    char abuf[1024] = {0};
	int nindex = 0;
    char resolved_path[80] = {0};
    char afile_path[128] = {0};
    char akey_path[128] = {0};
    char atmp_buf[1024] = {0};
    int  nfile_len = 0;
    char *psend_data = NULL;
    struct SHeader *phead = NULL;
    int ncurr_index = 0;
	int port = 0;
	char *phead_data = NULL;
	char *pRecvData = NULL;
    
    if (argc < 3)
    {
        //printf("argc must be: <appname> <port> <ipaddr>\n");
    }

    realpath("./",resolved_path);
	strcpy(afile_path, resolved_path);
	strcpy(afile_path+strlen(afile_path), "/");
	strcpy(afile_path+strlen(afile_path), argv[1]);
	//printf("curr path = %s\n", afile_path);
	
	strcpy(akey_path, resolved_path);
	strcpy(akey_path+strlen(akey_path), "/");
	strcpy(akey_path+strlen(akey_path), argv[2]);
	
	port = atoi(argv[3]);
	//printf("begin connect\n");
    nhandle = SOCKET_NewClient(port, "127.0.0.1");
	//printf("end connect\n");
    if (-1 == nhandle)
    {
		fprintf(stderr, "Error: could not contact enc_server on port %d\n", port);
        return 2;
    }

    nfile_len = FILE_GetFileSize(afile_path);
	
	//printf("file size = %d\n", nfile_len);
    psend_data = (char*)malloc(sizeof(struct SHeader));
    memset(psend_data, 0x0, sizeof(struct SHeader));
    phead = (struct SHeader*)psend_data;
	
	phead->m_nIsOk = 200;
	phead->n_ntype = 3;
	phead->m_nBodyLen = nfile_len;
	phead->m_nCurrIndex = 1;
	phead->m_nItemSize = 1000;
	phead->m_nItemCount =  nfile_len/1000 + 
								((nfile_len%1000 > 0)?1:0);
								
	//printf("before file info len = %d, index = %d, count = %d", nfile_len, ncurr_index, phead->m_nItemCount);
	
	memset(atmp_buf, 0x0, sizeof(atmp_buf));
	nres = SOCKET_Send(nhandle, psend_data, sizeof(struct SHeader));
	//printf("end file info len = %d, index = %d, count = %d", nfile_len, ncurr_index, phead->m_nItemCount);
	free(psend_data);
	
	ncurr_index++;
    for (nindex = 0; nindex < phead->m_nItemCount; nindex++)
    {

        ncurr_index++;
        memset(atmp_buf, 0x0, sizeof(atmp_buf));
        FILE_ReadData(afile_path, 2, 1000, 1, atmp_buf, 1000*(phead->m_nCurrIndex - 1));
        nres = SOCKET_Send(nhandle, atmp_buf, strlen(atmp_buf));
		phead->m_nCurrIndex++;
    }
	
	phead_data = (char*)malloc(sizeof(struct SHeader));
	memset(phead_data, 0x0, sizeof(struct SHeader));
	//printf("begin read\n");
	nres = recv(nhandle, phead_data, sizeof(struct SHeader), 0);
	//printf("main recv len = %d\n", nres);
	phead = (struct SHeader*)phead_data;
	if (phead->m_nIsOk == 300)
	{
		fprintf(stderr, "Error: could not contact enc_server on port %d\n", port);
		close(nhandle);
		return 2;
	}
	// end 明文
				
	//printf("key begin \n");
    nfile_len = FILE_GetFileSize(akey_path);
    psend_data = (char*)malloc(nfile_len+sizeof(struct SHeader));
    memset(psend_data, 0x0, nfile_len+sizeof(struct SHeader));
    phead = (struct SHeader*)psend_data;
	//printf("key file len = %d \n", nfile_len);
	
	phead->m_nIsOk = 200;
	phead->n_ntype = 4;
	phead->m_nBodyLen = nfile_len;
	phead->m_nCurrIndex = 1;
	phead->m_nItemSize = 1000;
	phead->m_nItemCount =  nfile_len/1000 + 
									((nfile_len%1000 > 0)?1:0);
	//printf("key send begin file len = %d\n", nfile_len);
	nres = SOCKET_Send(nhandle, psend_data, sizeof(struct SHeader));
	//printf("key key end send len = %d \n", nres);
	
    for (nindex = 0; nindex < phead->m_nItemCount; nindex++)
    {    
        memset(atmp_buf, 0x0, sizeof(atmp_buf));
        FILE_ReadData(akey_path, 2, 1000, 1, atmp_buf, 1000*(phead->m_nCurrIndex - 1));
		//printf("key key read data = %s , path = %s, index = %d\n", atmp_buf, akey_path, phead->m_nCurrIndex);
        nres = SOCKET_Send(nhandle, atmp_buf, strlen(atmp_buf));
		//printf("key key send data len = %d \n", nres);
		phead->m_nCurrIndex++;
    }
    free(psend_data);
	
	phead_data = (char*)malloc(sizeof(struct SHeader));
	memset(phead_data, 0x0, sizeof(struct SHeader));
	//printf("begin read\n");
	nres = recv(nhandle, phead_data, sizeof(struct SHeader), 0);
	//printf("main recv len = %d\n", nres);
	phead = (struct SHeader*)phead_data;
	if (phead->m_nIsOk == 300)
	{
		fprintf(stderr, "Error: could not contact enc_server on port %d\n", port);
		close(nhandle);
		return 2;
	}
    
    for (;;)
    {
		phead_data = (char*)malloc(1024);
		memset(phead_data, 0x0, 1024);
		nres = recv(nhandle, phead_data, sizeof(struct SHeader), 0);
		//printf("main recv len = %d\n", nres);
		phead = (struct SHeader*)phead_data;
		//printf("type = %d\n", phead->m_nIsOk);
		if (phead->m_nIsOk == 300)
		{
			fprintf(stderr, "Error: could not contact enc_server on port %d\n", port);
			close(nhandle);
			return 2;
		}

		//printf("recv miwen %d %d %d %d %d\n", phead->n_ntype, phead->m_nItemSize, phead->m_nItemCount, phead->m_nBodyLen, phead->m_nCurrIndex);
		
		pRecvData = (char*)malloc(phead->m_nBodyLen);
		memset(pRecvData, 0x0, phead->m_nBodyLen);
		for (nindex = 0; nindex < phead->m_nItemCount; nindex++)
		{
			//printf("begin recv miwen data\n");
			nres = recv(nhandle, pRecvData+nindex*1000, 1000, 0);
			//printf(" recv miwen len = %d, src = %s\n", nres, pRecvData);
		}
		printf("%s\n", pRecvData);
		close(nhandle);
		return 0;
    }
	
    return 0;
}



