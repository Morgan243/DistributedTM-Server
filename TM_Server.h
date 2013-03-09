#include <iostream>
#include <thread>

#include "../NetComm.git/NC_Server.h"

struct TM_Message
{
    unsigned char code;
    unsigned int address;
    unsigned int value;
};

struct Connected_Client
{
    bool client_done;
    std::thread *client_thread;
    std::string name;
    unsigned int id;
    TM_Message out_message, in_message;
    std::string in_buffer;
    unsigned char out_buffer[1024];
};


class TM_Server
{
    private:
        static bool done;


        static std::string address;
        static unsigned int port;
        static NC_Server master_server;
        static std::vector<Connected_Client> connected_clients;


        void SendMessage(TM_Message out_message, unsigned char out_buffer[]);
        TM_Message ReceiveMessage(std::string in_buffer, int client_id);

    public:

        TM_Server();
        ~TM_Server();

        void Start_Server();

        void LaunchClient(Connected_Client *client);
};

