#include <iostream>
#include <thread>

#include "../NetComm.git/NC_Server.h"
#include "../access_cache.git/Cache.h"

#ifndef TM_SERVER_H
#define TM_SERVER_H

#define READ 0x01
#define WRITE 0x02
#define COMMIT 0x04
#define ABORT 0x08
#define SYNC 0x10
#define MUTEX 0x20
#define INIT 0x40

#define MEMORY_SIZE 10
#define USING_CACHE 1

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

    std::thread *client_thread;         //thread that's dealing with client
//}}}
};

class TM_Server
{
    private:
        //is the server shutting down
        static bool done;

        //listen address and port
        static std::string address;
        static unsigned int port;

        static Cache access_cache;

        //networking back end, will store sockets for all connected clients
        static NC_Server master_server;

        //every clients client-specific data
        static std::vector<Connected_Client> connected_clients;

        //parse and send out a TM_Message
        void SendMessage(TM_Message out_message, unsigned char out_buffer[]);

        //receive data and put together a TM_Message
        TM_Message ReceiveMessage(std::string in_buffer, int client_id);

    public:
        TM_Server();
        ~TM_Server();

        //start main loop, spawning threads for new clients
        void Start_Server();

        //threads for each client fun here
        void LaunchClient(Connected_Client *client);
};
#endif
