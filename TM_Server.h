#include <iostream>
#include <thread>
#include "../NetComm.git/NC_Server.h"

struct Connected_Client
{
    std::thread *client_thread;
    std::string name;
    unsigned int id;
    //NC_Server *connection;
};

class TM_Server
{
    private:
        static bool done;

        static std::string address;
        static unsigned int port;
        static NC_Server master_server;
        static std::vector<Connected_Client> connected_clients;

    public:

        TM_Server();
        ~TM_Server();

        void Start_Server();

        void LaunchClient(Connected_Client *client);
};

