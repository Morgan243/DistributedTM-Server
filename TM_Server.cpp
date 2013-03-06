#include "TM_Server.h"

using namespace std;

    //--------------------------
    //Static Var define
    //--------------------------
    //{{{
    std::vector<Connected_Client> TM_Server::connected_clients;
    NC_Server TM_Server::master_server;
    unsigned int TM_Server::port;
    string TM_Server::address;
    bool TM_Server::done;
    //}}}
    //--------------------------


TM_Server::TM_Server()
{
    TM_Server::address = "127.0.0.1";
    TM_Server::port = 1337;
    TM_Server::master_server.Init(true, TM_Server::address, TM_Server::port);
    cout<<"Server initialized on "<<TM_Server::address<<":"<<TM_Server::port<<endl;
    TM_Server::done = false;
}

TM_Server::~TM_Server()
{
//{{{
    //tell threads to stop
    this->done = true;

    //join all threads: threads might be blocking on receive, probably disconnect clients first
    for( int i = 0; i < connected_clients.size(); i++)
    {
        connected_clients[i].client_thread->join();
    }
//}}}
}

void TM_Server::Start_Server()
{
//{{{
    int temp_socket;
    Connected_Client temp_client;

   cout<<"Beginning listen..."<<endl; 
       TM_Server::master_server.Listen(20);
   
   while(!done)
   {
       cout<<"Accepting new client..."<<endl;
            temp_socket = TM_Server::master_server.Accept();

       cout<<"Creating client threads back end..."<<endl;
            //create a network point for this thread
            temp_client.connection = new NC_Server(temp_socket);

            //set addresses for reference, just in case
            temp_client.connection->SetupAddress(TM_Server::address, TM_Server::port);

            connected_clients.push_back(temp_client);

            connected_clients.back().id = connected_clients.size() - 1;

       cout<<"Launching client thread..."<<endl;
            connected_clients.back().client_thread = new std::thread(&TM_Server::LaunchClient, this, &connected_clients.back());
   }

   //}}}
}


void TM_Server::LaunchClient(Connected_Client *client)
{
//{{{
    unsigned char in_buffer[1024], out_buffer[1024];
    cout<<"Thread handling client with id "<<client->id<<endl;

    while(!this->done)
    {
        client->connection->Receive(in_buffer, 1024);
        cout<<"Thread received "<<in_buffer<<" from client "<<client->id<<endl;
    }
//}}}
}

