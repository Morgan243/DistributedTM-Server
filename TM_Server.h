#include <mutex>
#include <iostream>
#include <pthread.h>
#include <thread>
#include <queue>

#include "../NetComm.git/NC_Server.h"
#include "../sw_access_cache.git/AccessCache.h"

#ifndef TM_SERVER_H
#define TM_SERVER_H

#define READ 0x01
#define WRITE 0x02
#define COMMIT 0x04
#define ABORT 0x08
#define SYNC 0x10
#define MUTEX 0x20
#define INIT 0x40

#define MEMORY_SIZE 4
#define USING_CACHE 1
#define DEBUG 0
#define PROMPT 0

//holds data that goes between client and server
struct TM_Message
{
//{{{
    unsigned char code;         //operation (read, write, commit, etc.)
    unsigned int address;       //address of TM
    unsigned int value;         //value of TM, not always used
//}}}
};

//holds everything corresponding to a single client
struct Connected_Client
{
//{{{
    bool client_done;                   //has the client disconnected
    std::string name;                   //friendly name of client
    unsigned int id;                    //id used by network back end

    TM_Message out_message, in_message; //temporary sending/receiving messages
    std::string in_buffer;              //raw input string
    unsigned char out_buffer[1024];     //output buffer
    std::vector<int> client_ops;             //all operations of client
    pthread_t client_thread;
//}}}
};

//holds things to be sent to the display client
struct Display_Data
{
//{{{
    unsigned int node_id;
    std::string client_name;
    TM_Message client_request, server_response;
    unsigned int client_commits, client_aborts;
//}}}
};

//represents a connected display client
struct Connected_Display
{
//{{{
    bool display_done;
    std::string name;
    unsigned int id;
    
    std::queue<Display_Data> outgoing;
    TM_Message out_message, in_message; //temporary sending/receiving messages
    std::string in_buffer;              //raw input string
    unsigned char out_buffer[1024];     //output buffer

    pthread_t display_thread;
//}}}
};

class TM_Server
{
    private:
        //is the server shutting down
        static bool done;
        static bool display_connected;

        //listen address and port
        static std::string address;
        static unsigned int port;

        //static Cache access_cache;
        static AccessCache access_cache;

        //networking back end, will store sockets for all connected clients
        static NC_Server master_server;

        //every clients client-specific data
        static std::vector<Connected_Client> connected_clients;

        static std::vector<Connected_Display> connected_displays;

        //shared memory; index is address
        static std::vector<unsigned int> memory;

        void FullInit(int memorySizem, std::string address, unsigned int port);

        //parse and send out a TM_Message
        void SendMessage(TM_Message out_message, unsigned char out_buffer[], int client_id);

        //receive data and put together a TM_Message
        void ReceiveMessage(std::string in_buffer, int client_id);

    public:
        TM_Server();
        TM_Server(int memorySize);
        TM_Server(int memorySize, std::string address, unsigned int port);
        ~TM_Server();

        //start main loop, spawning threads for new clients
        void Start_Server();

        //threads for each client fun here
        void LaunchClient(int client_id);

        //determine which *Attempt method to call 
        void HandleRequest(int client_id);

        //Try to do what the client requested, *Attempt methods
        void WriteAttempt(int client_id);
        void ReadAttempt(int client_id);
        void CommitAttempt(int client_id);
        void SyncAttempt(int client_id);
        void InitAttempt(int client_id);
};

struct help_launchArgs
{
    TM_Server *context;
    int id;
    //Connected_Client *client;
};

static void* help_launchThread(void *arg)
{
    help_launchArgs input = *((help_launchArgs *)arg);
    input.context->LaunchClient(input.id);
}
#endif
