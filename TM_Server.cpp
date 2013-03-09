#include <cstring>
#include <string>
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
    int temp_socket, client_id;
    Connected_Client temp_client;

   cout<<"Beginning listen..."<<endl; 
       TM_Server::master_server.Listen(20);
   
   while(!done)
   {
       cout<<"Accepting new client..."<<endl;
            client_id = TM_Server::master_server.Accept();

            temp_client.id = client_id;

            connected_clients.push_back(temp_client);


       cout<<"Launching client thread..."<<endl;
            connected_clients.back().client_thread = new std::thread(&TM_Server::LaunchClient, this, &connected_clients.back());
   }

   //}}}
}


void TM_Server::LaunchClient(Connected_Client *client)
{
//{{{
     client->client_done = false;
    string in_buffer, out_buffer[1024];
    int byte_count = 0;

    cout<<"Thread handling client with id "<<client->id<<endl;

    while(!this->done && !client->client_done)
    {
        //bzero(in_buffer.c_str(), 1024);
        in_buffer.erase();

        cout<<"Receiving..."<<endl;
        //byte_count = TM_Server::master_server.Receive((unsigned char *)in_buffer.c_str(), 1024, client->id);
        byte_count = TM_Server::master_server.Receive(&in_buffer, 1024, client->id);


        if(byte_count > 0)
            cout<<"Received: "<<in_buffer<<endl;
        else if (byte_count < 0)
            printf("Error, return is: %d\n", byte_count);

        if(in_buffer.find("SHUTDOWN") != string::npos)
        {
            client->client_done = true;
        }
    }


    cout<<"Client Sent SHUTDOWN command, terminating thread..."<<endl;
//}}}
}

