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

void TM_Server::SendMessage(TM_Message out_message, unsigned char out_buffer[])
{
        //clear data buffer
        bzero(out_buffer, sizeof(out_buffer));
    
       //get string version of data
       int size = sprintf((char*)out_buffer, "%c:%u:%u", out_message.code, out_message.address,out_message.value);

        cout<<"Message to send: "<<out_buffer<<endl;
        TM_Server::master_server.Send(out_buffer, size);
}

TM_Message TM_Server::ReceiveMessage(string in_buffer, int client_id)
{
//{{{
    unsigned pos1, pos2;
    TM_Message temp_message;

    in_buffer.clear();

    TM_Server::master_server.Receive(&in_buffer, 1024, client_id);

    if(in_buffer != "SHUTDOWN")
    {
        pos1 = in_buffer.find_first_of(":");

        temp_message.code = in_buffer[pos1 - 1];
        
        in_buffer[pos1] = '-';

        pos2 = in_buffer.find_first_of(":");

        temp_message.address = (unsigned int) atoi(in_buffer.substr(pos1+1, pos2-1).c_str());
        
        temp_message.address = (unsigned int) atoi(in_buffer.substr(pos2+1, in_buffer.length()).c_str());

        cout<<hex<<"\tcode: "<<(unsigned int)temp_message.code<<endl;
        cout<<hex<<"\taddr: "<<temp_message.address<<endl;
        cout<<hex<<"\tvalue: "<<temp_message.value<<endl;
    }
    else
    {
        temp_message.code = 0xFF;
        temp_message.address = 0;
        temp_message.value = 0;
    }

    return temp_message;
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
    string user_in;
    client->client_done = false;
    string in_buffer, out_buffer[1024];
    int byte_count = 0;

    cout<<"Thread handling client with id "<<client->id<<endl;

    while(!this->done && !client->client_done)
    {
        //bzero(in_buffer.c_str(), 1024);
        in_buffer.erase();

        cout<<"Receiving..."<<endl;
        //byte_count = TM_Server::master_server.Receive(&in_buffer, 1024, client->id);

        client->in_message = TM_Server::ReceiveMessage(client->in_buffer, client->id);

        if(byte_count > 0)
            cout<<"Received: "<<in_buffer<<endl;
        else if (byte_count < 0)
            printf("Error, return is: %d\n", byte_count);

        if(client->in_message.code == 0xFF)
        {
            client->client_done = true;
        }
        else
        {
            cout<<"Allow this operation? (y/n)"<<endl;
            cin>>user_in;
            
            if(user_in == "y")
            {
                client->out_message = client->in_message;
                TM_Server::SendMessage(client->out_message, client->out_buffer); //echo back for ACK
            }
            else if (user_in == "n")
            {
                client->out_message = client->in_message;
                client->out_message.code = 0x66;
                TM_Server::SendMessage(client->out_message, client->out_buffer); //echo back for ACK
            }
        }
    }


    cout<<"Client Sent SHUTDOWN command, terminating thread..."<<endl;
//}}}
}

