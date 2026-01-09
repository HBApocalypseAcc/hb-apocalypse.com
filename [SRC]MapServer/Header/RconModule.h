// RconModule.h - RCON Administration Module for Helbreath MapServer
// Allows remote administration via HBAdmin client
// Author: HBAdmin System
// 
//////////////////////////////////////////////////////////////////////

#ifndef RCON_MODULE_H
#define RCON_MODULE_H

#pragma once

// Forward declaration
class CMapServer;
class CClient;

// RCON Configuration
#define RCON_MAX_CLIENTS        5
#define RCON_BUFFER_SIZE        65536
#define RCON_DEFAULT_PORT       27015

// RCON Message Types
enum RconMessageType
{
    RCON_MSG_AUTH_REQUEST = 1,
    RCON_MSG_AUTH_RESPONSE = 2,
    RCON_MSG_COMMAND = 3,
    RCON_MSG_COMMAND_RESPONSE = 4,
    RCON_MSG_SERVER_LOG = 5,
    RCON_MSG_PLAYER_LIST = 6,
    RCON_MSG_PLAYER_LIST_REQUEST = 7,
    RCON_MSG_ANNOUNCEMENT = 8,
    RCON_MSG_KICK_PLAYER = 9,
    RCON_MSG_BAN_PLAYER = 10,
    RCON_MSG_KILL_PLAYER = 11,
    RCON_MSG_SERVER_INFO = 12,
    RCON_MSG_PING = 13,
    RCON_MSG_PONG = 14
};

// Player info structure for RCON
struct RconPlayerInfo
{
    char szName[12];
    int iLevel;
    char szClass[20];
    char szNation[12];
    char szMap[12];
    short sX;
    short sY;
    int iPing;
    char szIP[21];
    int iPlayTime;
    int iAdminLevel;
    int iClientIndex;
};

// RCON Client connection
struct RconClient
{
    SOCKET socket;
    bool bAuthenticated;
    bool bConnected;
    char szIP[21];
    DWORD dwConnectTime;
    DWORD dwLastActivity;
};

// Main RCON Module Class
class CRconModule
{
public:
    CRconModule();
    ~CRconModule();

    // Initialization
    bool Initialize(CMapServer* pGameServer, int iPort, const char* szPassword);
    void Shutdown();

    // Main processing (call from game loop)
    void Process();

    // Send log to all authenticated clients
    void BroadcastLog(const char* szLog);

    // Send server info update
    void BroadcastServerInfo();

    // Configuration
    void SetPassword(const char* szPassword);
    bool IsRunning() const { return m_bRunning; }
    int GetConnectedClients() const;

private:
    // Socket operations
    bool StartListening();
    void AcceptConnections();
    void ProcessClient(int iClientIndex);
    void DisconnectClient(int iClientIndex);

    // Message handling
    bool SendMessage(SOCKET sock, int iType, const char* szData, int iDataLen);
    bool ReceiveMessage(SOCKET sock, int* piType, char* pBuffer, int* piLen, bool* pbConnectionError = NULL);

    // Command handlers
    void HandleAuthRequest(int iClientIndex, const char* pData, int iLen);
    void HandleCommand(int iClientIndex, const char* pData, int iLen);
    void HandlePlayerListRequest(int iClientIndex);
    void HandleAnnouncement(int iClientIndex, const char* pData, int iLen);
    void HandleKickPlayer(int iClientIndex, const char* pData, int iLen);
    void HandleBanPlayer(int iClientIndex, const char* pData, int iLen);
    void HandleKillPlayer(int iClientIndex, const char* pData, int iLen);
    void HandlePing(int iClientIndex);

    // Helper functions
    void GetPlayerList(RconPlayerInfo* players, int* pCount, int maxPlayers);
    int FindPlayerByName(const char* szName);
    void ExecuteCommand(int iClientIndex, const char* szCommand);
    void BuildPlayerListJson(char* pBuffer, int bufferSize);
    void BuildServerInfoJson(char* pBuffer, int bufferSize);

    // Ban list management
    bool AddToBanList(const char* szPlayerName, const char* szIP);

private:
    CMapServer* m_pGameServer;
    SOCKET m_listenSocket;
    RconClient m_clients[RCON_MAX_CLIENTS];
    
    char m_szPassword[64];
    int m_iPort;
    
    volatile bool m_bRunning;

    // Buffers
    char m_recvBuffer[RCON_BUFFER_SIZE];
    char m_sendBuffer[RCON_BUFFER_SIZE];

    // Stats
    DWORD m_dwStartTime;
    int m_iTotalConnections;
};

// Global RCON instance
extern CRconModule* g_pRcon;

// Helper function to initialize RCON from config
bool InitializeRcon(CMapServer* pGameServer);

#endif // RCON_MODULE_H
