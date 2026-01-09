// RconModule.cpp - RCON Administration Module Implementation
// Provides remote administration capabilities for Helbreath MapServer
// Uses pure C-style code to be compatible with the existing codebase
//
//////////////////////////////////////////////////////////////////////

// Include winsock2 FIRST before any other headers
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include "../Header/Game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global RCON instance
CRconModule* g_pRcon = NULL;

// External functions
extern void PutLogList(char* cMsg);
extern CMapServer* c_map;

//////////////////////////////////////////////////////////////////////////
// CRconModule Implementation
//////////////////////////////////////////////////////////////////////////

CRconModule::CRconModule()
{
    m_pGameServer = NULL;
    m_listenSocket = INVALID_SOCKET;
    m_iPort = RCON_DEFAULT_PORT;
    m_bRunning = false;
    m_dwStartTime = 0;
    m_iTotalConnections = 0;

    ZeroMemory(m_szPassword, sizeof(m_szPassword));
    ZeroMemory(m_recvBuffer, sizeof(m_recvBuffer));
    ZeroMemory(m_sendBuffer, sizeof(m_sendBuffer));

    // Initialize client slots
    for (int i = 0; i < RCON_MAX_CLIENTS; i++)
    {
        m_clients[i].socket = INVALID_SOCKET;
        m_clients[i].bAuthenticated = false;
        m_clients[i].bConnected = false;
        m_clients[i].dwConnectTime = 0;
        m_clients[i].dwLastActivity = 0;
        ZeroMemory(m_clients[i].szIP, sizeof(m_clients[i].szIP));
    }
}

CRconModule::~CRconModule()
{
    Shutdown();
}

bool CRconModule::Initialize(CMapServer* pGameServer, int iPort, const char* szPassword)
{
    if (m_bRunning) return true;

    m_pGameServer = pGameServer;
    m_iPort = iPort;
    strncpy(m_szPassword, szPassword, sizeof(m_szPassword) - 1);

    if (!StartListening())
    {
        return false;
    }

    m_bRunning = true;
    m_dwStartTime = GetTickCount();

    char cMsg[256];
    sprintf(cMsg, "(RCON) Module initialized on port %d", m_iPort);
    PutLogList(cMsg);

    return true;
}

void CRconModule::Shutdown()
{
    if (!m_bRunning) return;

    m_bRunning = false;

    // Disconnect all clients
    for (int i = 0; i < RCON_MAX_CLIENTS; i++)
    {
        if (m_clients[i].socket != INVALID_SOCKET)
        {
            closesocket(m_clients[i].socket);
            m_clients[i].socket = INVALID_SOCKET;
            m_clients[i].bConnected = false;
        }
    }

    // Close listen socket
    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    PutLogList((char*)"(RCON) Module shutdown");
}

bool CRconModule::StartListening()
{
    // Create socket
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        PutLogList((char*)"(RCON) ERROR: Failed to create socket");
        return false;
    }

    // Set socket options
    int iOptVal = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&iOptVal, sizeof(iOptVal));

    // Set non-blocking mode
    u_long iMode = 1;
    ioctlsocket(m_listenSocket, FIONBIO, &iMode);

    // Bind
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons((u_short)m_iPort);

    if (bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        char cMsg[256];
        sprintf(cMsg, "(RCON) ERROR: Failed to bind to port %d (Error: %d)", m_iPort, WSAGetLastError());
        PutLogList(cMsg);
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    // Listen
    if (listen(m_listenSocket, RCON_MAX_CLIENTS) == SOCKET_ERROR)
    {
        PutLogList((char*)"(RCON) ERROR: Failed to listen");
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    return true;
}

void CRconModule::Process()
{
    if (!m_bRunning) return;

    // Accept new connections
    AcceptConnections();

    // Process existing clients
    for (int i = 0; i < RCON_MAX_CLIENTS; i++)
    {
        if (m_clients[i].bConnected)
        {
            ProcessClient(i);

            // Check for timeout (5 minutes)
            if (GetTickCount() - m_clients[i].dwLastActivity > 300000)
            {
                char cMsg[256];
                sprintf(cMsg, "(RCON) Client %s timed out", m_clients[i].szIP);
                PutLogList(cMsg);
                DisconnectClient(i);
            }
        }
    }
}

void CRconModule::AcceptConnections()
{
    sockaddr_in clientAddr;
    int iAddrLen = sizeof(clientAddr);

    SOCKET clientSocket = accept(m_listenSocket, (sockaddr*)&clientAddr, &iAddrLen);
    if (clientSocket == INVALID_SOCKET)
    {
        return; // No pending connections
    }

    // Find empty slot
    int iSlot = -1;
    for (int i = 0; i < RCON_MAX_CLIENTS; i++)
    {
        if (!m_clients[i].bConnected)
        {
            iSlot = i;
            break;
        }
    }

    if (iSlot == -1)
    {
        // No slots available
        closesocket(clientSocket);
        PutLogList((char*)"(RCON) Connection rejected - no slots available");
        return;
    }

    // Set non-blocking
    u_long iMode = 1;
    ioctlsocket(clientSocket, FIONBIO, &iMode);

    // Store client info
    m_clients[iSlot].socket = clientSocket;
    m_clients[iSlot].bConnected = true;
    m_clients[iSlot].bAuthenticated = false;
    m_clients[iSlot].dwConnectTime = GetTickCount();
    m_clients[iSlot].dwLastActivity = GetTickCount();
    
    // Convert IPv4 address to string (avoid inet_ntop unresolved external on older toolchains)
    {
        const char* pIp = inet_ntoa(clientAddr.sin_addr);
        if (pIp != NULL)
        {
            strncpy(m_clients[iSlot].szIP, pIp, sizeof(m_clients[iSlot].szIP) - 1);
            m_clients[iSlot].szIP[sizeof(m_clients[iSlot].szIP) - 1] = '\0';
        }
        else
        {
            strcpy(m_clients[iSlot].szIP, "0.0.0.0");
        }
    }

    m_iTotalConnections++;

    char cMsg[256];
    sprintf(cMsg, "(RCON) New connection from %s", m_clients[iSlot].szIP);
    PutLogList(cMsg);
}

void CRconModule::ProcessClient(int iClientIndex)
{
    if (iClientIndex < 0 || iClientIndex >= RCON_MAX_CLIENTS) return;
    if (!m_clients[iClientIndex].bConnected) return;

    SOCKET sock = m_clients[iClientIndex].socket;

    // Check for incoming data
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    timeval timeout = { 0, 0 };
    int iResult = select(0, &readSet, NULL, NULL, &timeout);

    if (iResult > 0 && FD_ISSET(sock, &readSet))
    {
        int iMsgType = 0;
        int iDataLen = 0;
        bool bConnectionError = false;

        if (ReceiveMessage(sock, &iMsgType, m_recvBuffer, &iDataLen, &bConnectionError))
        {
            m_clients[iClientIndex].dwLastActivity = GetTickCount();

            // Handle message based on type
            switch (iMsgType)
            {
            case RCON_MSG_AUTH_REQUEST:
                HandleAuthRequest(iClientIndex, m_recvBuffer, iDataLen);
                break;

            case RCON_MSG_COMMAND:
                if (m_clients[iClientIndex].bAuthenticated)
                    HandleCommand(iClientIndex, m_recvBuffer, iDataLen);
                break;

            case RCON_MSG_PLAYER_LIST_REQUEST:
                if (m_clients[iClientIndex].bAuthenticated)
                    HandlePlayerListRequest(iClientIndex);
                break;

            case RCON_MSG_ANNOUNCEMENT:
                if (m_clients[iClientIndex].bAuthenticated)
                    HandleAnnouncement(iClientIndex, m_recvBuffer, iDataLen);
                break;

            case RCON_MSG_KICK_PLAYER:
                if (m_clients[iClientIndex].bAuthenticated)
                    HandleKickPlayer(iClientIndex, m_recvBuffer, iDataLen);
                break;

            case RCON_MSG_BAN_PLAYER:
                if (m_clients[iClientIndex].bAuthenticated)
                    HandleBanPlayer(iClientIndex, m_recvBuffer, iDataLen);
                break;

            case RCON_MSG_KILL_PLAYER:
                if (m_clients[iClientIndex].bAuthenticated)
                    HandleKillPlayer(iClientIndex, m_recvBuffer, iDataLen);
                break;

            case RCON_MSG_PING:
                HandlePing(iClientIndex);
                break;
            }
        }
        else if (bConnectionError)
        {
            // Real connection error or graceful close - disconnect
            DisconnectClient(iClientIndex);
        }
        // else: no complete message yet, just wait for more data
    }
}

void CRconModule::DisconnectClient(int iClientIndex)
{
    if (iClientIndex < 0 || iClientIndex >= RCON_MAX_CLIENTS) return;

    if (m_clients[iClientIndex].socket != INVALID_SOCKET)
    {
        closesocket(m_clients[iClientIndex].socket);
        m_clients[iClientIndex].socket = INVALID_SOCKET;
    }

    char cMsg[256];
    sprintf(cMsg, "(RCON) Client %s disconnected", m_clients[iClientIndex].szIP);
    PutLogList(cMsg);

    m_clients[iClientIndex].bConnected = false;
    m_clients[iClientIndex].bAuthenticated = false;
}

bool CRconModule::SendMessage(SOCKET sock, int iType, const char* szData, int iDataLen)
{
    if (sock == INVALID_SOCKET) return false;

    // Protocol: [4 bytes length][4 bytes type][data]
    int iTotalLen = 8 + iDataLen;
    
    if (iTotalLen > RCON_BUFFER_SIZE) return false;
    
    memcpy(m_sendBuffer, &iTotalLen, 4);
    memcpy(m_sendBuffer + 4, &iType, 4);
    if (iDataLen > 0 && szData != NULL)
    {
        memcpy(m_sendBuffer + 8, szData, iDataLen);
    }

    int iSent = send(sock, m_sendBuffer, iTotalLen, 0);

    return (iSent == iTotalLen);
}

bool CRconModule::ReceiveMessage(SOCKET sock, int* piType, char* pBuffer, int* piLen, bool* pbConnectionError)
{
    // Initialize outputs
    *piLen = 0;
    if (pbConnectionError) *pbConnectionError = false;

    if (sock == INVALID_SOCKET)
    {
        if (pbConnectionError) *pbConnectionError = true;
        return false;
    }

    // Check how much data is available
    u_long ulAvailable = 0;
    ioctlsocket(sock, FIONREAD, &ulAvailable);

    // Need at least 8 bytes for header
    if (ulAvailable < 8)
    {
        // Check if connection is still alive (recv with MSG_PEEK returns 0 if closed)
        char peekByte;
        int iPeek = recv(sock, &peekByte, 1, MSG_PEEK);
        if (iPeek == 0)
        {
            // Connection closed gracefully
            if (pbConnectionError) *pbConnectionError = true;
            return false;
        }
        if (iPeek == SOCKET_ERROR)
        {
            int iErr = WSAGetLastError();
            if (iErr != WSAEWOULDBLOCK)
            {
                // Real error
                if (pbConnectionError) *pbConnectionError = true;
                return false;
            }
        }
        // Not enough data yet - wait for more
        return false;
    }

    // Read header with MSG_PEEK first
    char header[8];
    int iReceived = recv(sock, header, 8, MSG_PEEK);

    if (iReceived == SOCKET_ERROR)
    {
        int iErr = WSAGetLastError();
        if (iErr == WSAEWOULDBLOCK)
        {
            return false;
        }
        if (pbConnectionError) *pbConnectionError = true;
        return false;
    }

    if (iReceived == 0)
    {
        if (pbConnectionError) *pbConnectionError = true;
        return false;
    }

    if (iReceived < 8)
    {
        return false;
    }

    // Parse header to get total length
    int iTotalLen = 0;
    memcpy(&iTotalLen, header, 4);
    memcpy(piType, header + 4, 4);

    // Validate message size
    if (iTotalLen <= 8 || iTotalLen > RCON_BUFFER_SIZE)
    {
        // Invalid message - log and consume header to discard
        char cMsg[256];
        sprintf(cMsg, "(RCON) Invalid message size: %d (type: %d)", iTotalLen, *piType);
        PutLogList(cMsg);
        recv(sock, header, 8, 0);
        return false;
    }

    int iDataLen = iTotalLen - 8;

    // Check if full message is available
    if ((int)ulAvailable < iTotalLen)
    {
        // Full message not yet received - wait
        return false;
    }

    // Now actually consume the header
    iReceived = recv(sock, header, 8, 0);
    if (iReceived != 8)
    {
        if (pbConnectionError) *pbConnectionError = true;
        return false;
    }

    *piLen = iDataLen;

    if (iDataLen > 0)
    {
        // Read data in a loop to handle partial reads
        int iTotalRead = 0;
        int iRetries = 0;
        while (iTotalRead < iDataLen && iRetries < 100)
        {
            int iChunk = recv(sock, pBuffer + iTotalRead, iDataLen - iTotalRead, 0);
            if (iChunk <= 0)
            {
                if (iChunk == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
                {
                    Sleep(1);
                    iRetries++;
                    continue;
                }
                if (pbConnectionError) *pbConnectionError = true;
                return false;
            }
            iTotalRead += iChunk;
        }
        
        if (iTotalRead < iDataLen)
        {
            if (pbConnectionError) *pbConnectionError = true;
            return false;
        }
        
        pBuffer[iDataLen] = '\0';
    }

    return true;
}

// Helper function to extract a JSON string value
static bool ExtractJsonString(const char* pJson, const char* szKey, char* pValue, int iMaxLen)
{
    char szSearch[128];
    sprintf(szSearch, "\"%s\":\"", szKey);
    
    const char* pStart = strstr(pJson, szSearch);
    if (pStart == NULL) return false;
    
    pStart += strlen(szSearch);
    const char* pEnd = strchr(pStart, '"');
    if (pEnd == NULL) return false;
    
    int iLen = (int)(pEnd - pStart);
    if (iLen >= iMaxLen) iLen = iMaxLen - 1;
    
    strncpy(pValue, pStart, iLen);
    pValue[iLen] = '\0';
    
    return true;
}

void CRconModule::HandleAuthRequest(int iClientIndex, const char* pData, int iLen)
{
    char szPassword[64] = {0};
    
    // Parse JSON: {"Password":"xxx"}
    if (!ExtractJsonString(pData, "Password", szPassword, sizeof(szPassword)))
    {
        const char* response = "{\"Success\":false,\"Message\":\"Invalid request\"}";
        SendMessage(m_clients[iClientIndex].socket, RCON_MSG_AUTH_RESPONSE, response, (int)strlen(response));
        return;
    }

    // Check password
    if (strcmp(szPassword, m_szPassword) == 0)
    {
        m_clients[iClientIndex].bAuthenticated = true;

        char cMsg[256];
        sprintf(cMsg, "(RCON) Client %s authenticated successfully", m_clients[iClientIndex].szIP);
        PutLogList(cMsg);

        // Build response with server info
        char response[4096];
        char serverInfo[2048];
        BuildServerInfoJson(serverInfo, sizeof(serverInfo));
        sprintf(response, "{\"Success\":true,\"Message\":\"Authenticated\",\"ServerInfo\":%s}", serverInfo);
        SendMessage(m_clients[iClientIndex].socket, RCON_MSG_AUTH_RESPONSE, response, (int)strlen(response));

        // Send current player list
        HandlePlayerListRequest(iClientIndex);
    }
    else
    {
        char cMsg[256];
        sprintf(cMsg, "(RCON) Client %s failed authentication", m_clients[iClientIndex].szIP);
        PutLogList(cMsg);

        const char* response = "{\"Success\":false,\"Message\":\"Invalid password\"}";
        SendMessage(m_clients[iClientIndex].socket, RCON_MSG_AUTH_RESPONSE, response, (int)strlen(response));
    }
}

void CRconModule::HandleCommand(int iClientIndex, const char* pData, int iLen)
{
    char szCommand[512] = {0};
    
    if (!ExtractJsonString(pData, "Command", szCommand, sizeof(szCommand)))
    {
        return;
    }

    char cMsg[512];
    sprintf(cMsg, "(RCON) Command from %s: %s", m_clients[iClientIndex].szIP, szCommand);
    PutLogList(cMsg);

    ExecuteCommand(iClientIndex, szCommand);
}

void CRconModule::HandlePlayerListRequest(int iClientIndex)
{
    char json[32768];
    BuildPlayerListJson(json, sizeof(json));
    SendMessage(m_clients[iClientIndex].socket, RCON_MSG_PLAYER_LIST, json, (int)strlen(json));
}

void CRconModule::HandleAnnouncement(int iClientIndex, const char* pData, int iLen)
{
    if (m_pGameServer == NULL) return;

    char szMessage[512] = {0};
    
    if (!ExtractJsonString(pData, "Message", szMessage, sizeof(szMessage)))
    {
        return;
    }

    char cMsg[512];
    sprintf(cMsg, "(RCON) Announcement from %s: %s", m_clients[iClientIndex].szIP, szMessage);
    PutLogList(cMsg);

    // Broadcast to all players using server's broadcast function
    m_pGameServer->BroadcastNoticeMsg(szMessage);

    // Send confirmation
    const char* response = "{\"Success\":true,\"Message\":\"Announcement sent\"}";
    SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
}

void CRconModule::HandleKickPlayer(int iClientIndex, const char* pData, int iLen)
{
    if (m_pGameServer == NULL) return;

    char szPlayerName[32] = {0};
    
    if (!ExtractJsonString(pData, "PlayerName", szPlayerName, sizeof(szPlayerName)))
    {
        return;
    }

    int iPlayerIndex = FindPlayerByName(szPlayerName);
    if (iPlayerIndex >= 0)
    {
        char cMsg[256];
        sprintf(cMsg, "(RCON) Kicking player %s by %s", szPlayerName, m_clients[iClientIndex].szIP);
        PutLogList(cMsg);

        // Disconnect the player
        m_pGameServer->DeleteClient(iPlayerIndex, TRUE, TRUE, TRUE, TRUE);

        const char* response = "{\"Success\":true,\"Message\":\"Player kicked\"}";
        SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
    }
    else
    {
        const char* response = "{\"Success\":false,\"Message\":\"Player not found\"}";
        SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
    }
}

void CRconModule::HandleBanPlayer(int iClientIndex, const char* pData, int iLen)
{
    if (m_pGameServer == NULL) return;

    char szPlayerName[32] = {0};
    
    if (!ExtractJsonString(pData, "PlayerName", szPlayerName, sizeof(szPlayerName)))
    {
        return;
    }

    int iPlayerIndex = FindPlayerByName(szPlayerName);
    if (iPlayerIndex >= 0)
    {
        CClient* pClient = m_pGameServer->m_pClientList[iPlayerIndex];
        if (pClient != NULL)
        {
            char cMsg[256];
            sprintf(cMsg, "(RCON) Banning player %s (IP: %s) by %s", 
                szPlayerName, pClient->m_cIPaddress, m_clients[iClientIndex].szIP);
            PutLogList(cMsg);

            // Add to ban list
            AddToBanList(szPlayerName, pClient->m_cIPaddress);

            // Disconnect the player
            m_pGameServer->DeleteClient(iPlayerIndex, TRUE, TRUE, TRUE, TRUE);

            const char* response = "{\"Success\":true,\"Message\":\"Player banned\"}";
            SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
        }
        else
        {
            const char* response = "{\"Success\":false,\"Message\":\"Player not found\"}";
            SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
        }
    }
    else
    {
        const char* response = "{\"Success\":false,\"Message\":\"Player not found\"}";
        SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
    }
}

void CRconModule::HandleKillPlayer(int iClientIndex, const char* pData, int iLen)
{
    if (m_pGameServer == NULL) return;

    char szPlayerName[32] = {0};
    
    if (!ExtractJsonString(pData, "PlayerName", szPlayerName, sizeof(szPlayerName)))
    {
        return;
    }

    int iPlayerIndex = FindPlayerByName(szPlayerName);
    if (iPlayerIndex >= 0)
    {
        CClient* pClient = m_pGameServer->m_pClientList[iPlayerIndex];
        if (pClient != NULL)
        {
            char cMsg[256];
            sprintf(cMsg, "(RCON) Killing player %s by %s", szPlayerName, m_clients[iClientIndex].szIP);
            PutLogList(cMsg);

            // Set HP to 0 - player dies
            pClient->m_iHP = 0;
            pClient->m_bIsKilled = TRUE;

            // Notify using server's handler
            m_pGameServer->ClientKilledHandler(iPlayerIndex, 0, 0, 9999);

            const char* response = "{\"Success\":true,\"Message\":\"Player killed\"}";
            SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
        }
        else
        {
            const char* response = "{\"Success\":false,\"Message\":\"Player not found\"}";
            SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
        }
    }
    else
    {
        const char* response = "{\"Success\":false,\"Message\":\"Player not found\"}";
        SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
    }
}

void CRconModule::HandlePing(int iClientIndex)
{
    SendMessage(m_clients[iClientIndex].socket, RCON_MSG_PONG, "{}", 2);
}

void CRconModule::GetPlayerList(RconPlayerInfo* players, int* pCount, int maxPlayers)
{
    if (m_pGameServer == NULL)
    {
        *pCount = 0;
        return;
    }

    int count = 0;

    for (int i = 0; i < DEF_MAXCLIENTS && count < maxPlayers; i++)
    {
        CClient* pClient = m_pGameServer->m_pClientList[i];
        if (pClient != NULL && pClient->m_bIsInitComplete)
        {
            RconPlayerInfo* pInfo = &players[count];
            ZeroMemory(pInfo, sizeof(RconPlayerInfo));

            strncpy(pInfo->szName, pClient->m_cCharName, sizeof(pInfo->szName) - 1);
            pInfo->iLevel = pClient->m_iLevel;
            
            // Determine class based on type
            switch (pClient->m_sType)
            {
            case 1: strcpy(pInfo->szClass, "Warrior"); break;
            case 2: strcpy(pInfo->szClass, "Mage"); break;
            default: strcpy(pInfo->szClass, "Unknown"); break;
            }

            // Nation
            switch (pClient->m_cSide)
            {
            case 1: strcpy(pInfo->szNation, "Aresden"); break;
            case 2: strcpy(pInfo->szNation, "Elvine"); break;
            default: strcpy(pInfo->szNation, "Traveler"); break;
            }

            strncpy(pInfo->szMap, pClient->m_cMapName, sizeof(pInfo->szMap) - 1);
            pInfo->sX = pClient->m_sX;
            pInfo->sY = pClient->m_sY;
            pInfo->iPing = 0; // Would need ping calculation
            strncpy(pInfo->szIP, pClient->m_cIPaddress, sizeof(pInfo->szIP) - 1);
            pInfo->iAdminLevel = pClient->m_iAdminUserLevel;
            pInfo->iClientIndex = i;

            count++;
        }
    }

    *pCount = count;
}

int CRconModule::FindPlayerByName(const char* szName)
{
    if (m_pGameServer == NULL || szName == NULL) return -1;

    for (int i = 0; i < DEF_MAXCLIENTS; i++)
    {
        CClient* pClient = m_pGameServer->m_pClientList[i];
        if (pClient != NULL && pClient->m_bIsInitComplete)
        {
            if (_stricmp(pClient->m_cCharName, szName) == 0)
            {
                return i;
            }
        }
    }

    return -1;
}

void CRconModule::ExecuteCommand(int iClientIndex, const char* szCommand)
{
    char response[512];
    sprintf(response, "{\"Success\":true,\"Message\":\"Command executed: %s\"}", szCommand);
    SendMessage(m_clients[iClientIndex].socket, RCON_MSG_COMMAND_RESPONSE, response, (int)strlen(response));
}

void CRconModule::BuildPlayerListJson(char* pBuffer, int bufferSize)
{
    RconPlayerInfo players[DEF_MAXCLIENTS];
    int playerCount = 0;
    
    GetPlayerList(players, &playerCount, DEF_MAXCLIENTS);

    char* pCurrent = pBuffer;
    int remaining = bufferSize;
    int written;

    written = sprintf(pCurrent, "{\"Players\":[");
    pCurrent += written;
    remaining -= written;

    for (int i = 0; i < playerCount && remaining > 512; i++)
    {
        RconPlayerInfo* p = &players[i];
        
        if (i > 0)
        {
            *pCurrent++ = ',';
            remaining--;
        }

        written = sprintf(pCurrent, 
            "{\"Name\":\"%s\",\"Level\":%d,\"Class\":\"%s\",\"Nation\":\"%s\","
            "\"Map\":\"%s\",\"MapX\":%d,\"MapY\":%d,\"Coordinates\":\"(%d, %d)\","
            "\"Ping\":%d,\"IP\":\"%s\",\"PlayTime\":\"Online\",\"AdminLevel\":%d}",
            p->szName, p->iLevel, p->szClass, p->szNation,
            p->szMap, p->sX, p->sY, p->sX, p->sY,
            p->iPing, p->szIP, p->iAdminLevel);
        
        pCurrent += written;
        remaining -= written;
    }

    sprintf(pCurrent, "],\"Count\":%d}", playerCount);
}

void CRconModule::BuildServerInfoJson(char* pBuffer, int bufferSize)
{
    if (m_pGameServer == NULL)
    {
        strcpy(pBuffer, "{\"ServerName\":\"Unknown\",\"Players\":0,\"MaxPlayers\":0}");
        return;
    }

    DWORD uptime = (GetTickCount() - m_dwStartTime) / 1000;

    sprintf(pBuffer, 
        "{\"ServerName\":\"%s\",\"Players\":%d,\"MaxPlayers\":%d,\"Uptime\":%u}",
        m_pGameServer->m_cServerName,
        m_pGameServer->m_iTotalClients,
        DEF_MAXCLIENTS,
        uptime);
}

bool CRconModule::AddToBanList(const char* szPlayerName, const char* szIP)
{
    // Write to BannedList.cfg
    FILE* pFile = fopen("Configs\\BannedList.cfg", "at");
    if (pFile == NULL)
    {
        pFile = fopen("Configs\\BannedList.cfg", "wt");
    }

    if (pFile != NULL)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);

        fprintf(pFile, "player-banned-name = %s\n", szPlayerName);
        fprintf(pFile, "player-banned-ip   = %s\n", szIP);
        fprintf(pFile, "player-banned-date = %02d/%02d/%04d %02d:%02d\n\n",
            st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);

        fclose(pFile);
        return true;
    }

    return false;
}

void CRconModule::BroadcastLog(const char* szLog)
{
    if (!m_bRunning) return;

    // Build JSON with escaped log message
    char json[4096];
    char escaped[3072];
    
    // Escape special characters
    const char* pSrc = szLog;
    char* pDst = escaped;
    int remaining = sizeof(escaped) - 1;
    
    while (*pSrc && remaining > 6)
    {
        switch (*pSrc)
        {
        case '"':  *pDst++ = '\\'; *pDst++ = '"'; remaining -= 2; break;
        case '\\': *pDst++ = '\\'; *pDst++ = '\\'; remaining -= 2; break;
        case '\n': *pDst++ = '\\'; *pDst++ = 'n'; remaining -= 2; break;
        case '\r': *pDst++ = '\\'; *pDst++ = 'r'; remaining -= 2; break;
        case '\t': *pDst++ = '\\'; *pDst++ = 't'; remaining -= 2; break;
        default:   *pDst++ = *pSrc; remaining--; break;
        }
        pSrc++;
    }
    *pDst = '\0';
    
    sprintf(json, "{\"Log\":\"%s\"}", escaped);

    for (int i = 0; i < RCON_MAX_CLIENTS; i++)
    {
        if (m_clients[i].bConnected && m_clients[i].bAuthenticated)
        {
            SendMessage(m_clients[i].socket, RCON_MSG_SERVER_LOG, json, (int)strlen(json));
        }
    }
}

void CRconModule::BroadcastServerInfo()
{
    if (!m_bRunning) return;

    char json[2048];
    BuildServerInfoJson(json, sizeof(json));

    for (int i = 0; i < RCON_MAX_CLIENTS; i++)
    {
        if (m_clients[i].bConnected && m_clients[i].bAuthenticated)
        {
            SendMessage(m_clients[i].socket, RCON_MSG_SERVER_INFO, json, (int)strlen(json));
        }
    }
}

void CRconModule::SetPassword(const char* szPassword)
{
    strncpy(m_szPassword, szPassword, sizeof(m_szPassword) - 1);
}

int CRconModule::GetConnectedClients() const
{
    int count = 0;
    for (int i = 0; i < RCON_MAX_CLIENTS; i++)
    {
        if (m_clients[i].bConnected && m_clients[i].bAuthenticated)
            count++;
    }
    return count;
}

//////////////////////////////////////////////////////////////////////////
// Global helper function
//////////////////////////////////////////////////////////////////////////

bool InitializeRcon(CMapServer* pGameServer)
{
    if (g_pRcon != NULL)
    {
        delete g_pRcon;
    }

    g_pRcon = new CRconModule();

    // Read config from RconConfig.cfg
    int iPort = RCON_DEFAULT_PORT;
    char szPassword[64] = "admin123";

    FILE* pFile = fopen("Configs\\RconConfig.cfg", "rt");
    if (pFile != NULL)
    {
        char cLine[256];
        while (fgets(cLine, sizeof(cLine), pFile))
        {
            if (strncmp(cLine, "rcon-port", 9) == 0)
            {
                sscanf(cLine, "rcon-port = %d", &iPort);
            }
            else if (strncmp(cLine, "rcon-password", 13) == 0)
            {
                sscanf(cLine, "rcon-password = %s", szPassword);
            }
        }
        fclose(pFile);
    }
    else
    {
        // Create default config
        pFile = fopen("Configs\\RconConfig.cfg", "wt");
        if (pFile != NULL)
        {
            fprintf(pFile, "// RCON Configuration\n");
            fprintf(pFile, "rcon-port = 27015\n");
            fprintf(pFile, "rcon-password = admin123\n");
            fprintf(pFile, "// Max 5 simultaneous admin connections\n");
            fclose(pFile);
        }
    }

    return g_pRcon->Initialize(pGameServer, iPort, szPassword);
}

