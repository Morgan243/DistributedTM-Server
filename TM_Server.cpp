#include <cstring>
#include <string>
#include "TM_Server.h"

using namespace std;

    //--------------------------
    //Static Var define
    //--------------------------
    //{{{
    Cache TM_Server::access_cache;
    std::vector<Connected_Client> TM_Server::connected_clients;
    NC_Server TM_Server::master_server;
    unsigned int TM_Server::port;
    string TM_Server::address;
    bool TM_Server::done;
    //}}}
    //--------------------------


TM_Server::TM_Server()
{
//{{{
    //set hosting address and port
    TM_Server::address = "127.0.0.1";
    TM_Server::port = 1337;

    //setup socket (if not already), set adress, and bind socket
    TM_Server::master_server.Init(true, TM_Server::address, TM_Server::port);
    cout<<"Server initialized on "<<TM_Server::address<<":"<<TM_Server::port<<endl;

    //don't stop until done is true
    TM_Server::done = false;

    for(int i = 0; i < MEMORY_SIZE; i++)
        access_cache.AddCacheLine();
//}}}
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
//{{{
        //clear data buffer
        bzero(out_buffer, sizeof(out_buffer));
    
       //get string version of data
       int size 
           = sprintf((char*)out_buffer, "%c:%u:%u", out_message.code, out_message.address,out_message.value);

        cout<<"Message to send: "<<out_buffer<<endl;

        //send out the formatted string
        TM_Server::master_server.Send(out_buffer, size);
//}}}
}

TM_Message TM_Server::ReceiveMessage(string in_buffer, int client_id)
{
//{{{
    unsigned pos1, pos2;
    TM_Message temp_message;

    in_buffer.clear();

    int bytes_recv = TM_Server::master_server.Receive(&in_buffer, 1024, client_id);

    //check for errors
    if(bytes_recv < 0)
    {
        cout<<"Error receiving message from client "<<client_id<<endl;
        
        //indicate error in message struct
        temp_message.code = 0;
        temp_message.address = 0;
        temp_message.value = 0;
    }
    //check for client shutdown
    else if(in_buffer == "SHUTDOWN")
    {
        temp_message.code = 0xFF;
        temp_message.address = 0;
        temp_message.value = 0;
    }
    //everything is good, parse the message
    else
    {
        //find the index of the first colon
        //(it will always be 1 since the first portion is one character)
        //(implemented dynamic way in case the message encoding changes)
        pos1 = in_buffer.find_first_of(":");

        //get first character(currently always index zero)
        temp_message.code = in_buffer[pos1 - 1];
        
        //replace with '-' so find gets the next one
        in_buffer[pos1] = '-';

        //get second position of colon
        pos2 = in_buffer.find_first_of(":");

        //get the address out
        temp_message.address = (unsigned int) atoi(in_buffer.substr(pos1+1, pos2-1).c_str());
        
        //get the value out
        temp_message.value = (unsigned int) atoi(in_buffer.substr(pos2+1, in_buffer.length()).c_str());

        cout<<hex<<"\tcode: "<<(unsigned int)temp_message.code<<endl;
        cout<<hex<<"\taddr: "<<temp_message.address<<endl;
        cout<<hex<<"\tvalue: "<<temp_message.value<<endl;
    }

    return temp_message;
//}}}
}

void TM_Server::Start_Server()
{
//{{{
    Connected_Client temp_client;

    cout<<"Beginning listen..."<<endl; 

    //prepare for clients
    TM_Server::master_server.Listen(20);
   
    //loop continues until end
    while(!done)
    {
        temp_client.in_buffer.clear();
        temp_client.name.clear();
        temp_client.client_done = false;

        cout<<"Accepting new client..."<<endl;

        //accept a client, get its id
        temp_client.id = TM_Server::master_server.Accept();

        TM_Server::master_server.Receive(&temp_client.in_buffer, 1024, temp_client.id);

        cout<<">>Name declared as: "<<temp_client.in_buffer<<endl;
        temp_client.name = temp_client.in_buffer;

        TM_Server::access_cache.AddProcessor(temp_client.name); 

        //store the client in vector
        connected_clients.push_back(temp_client);

        cout<<"Launching client thread..."<<endl;

        //launch the clients thread into LaunchClient
        connected_clients.back().client_thread 
                        = new std::thread(&TM_Server::LaunchClient, this, &connected_clients.back());
    }
   //}}}
}

void TM_Server::LaunchClient(Connected_Client *client)
{
//{{{
    string user_in;
    client->client_done = false;

    cout<<"Thread handling client with id "<<client->id<<endl;

    //stop only if server stops or client shutdowns
    while(!this->done && !client->client_done)
    {
        //bzero(in_buffer.c_str(), 1024);
        client->in_buffer.erase();

        cout<<"Receiving..."<<endl;
        //byte_count = TM_Server::master_server.Receive(&in_buffer, 1024, client->id);

        client->in_message = TM_Server::ReceiveMessage(client->in_buffer, client->id);

        //check for error(==0)
        if (!client->in_message.code)
        {
            cout<<"ERROR, ignoring..."<<endl;
        }
        //check for shutdown code from client
        else if(client->in_message.code == 0xFF)
        {
            client->client_done = true;
        }
        //otherwise, parse the message and respond
        else
        {
            //check access cache here
            cout<<"Allow this operation? (y/n)"<<endl;
            cin>>user_in;
            
            //allow transaction
            if(user_in == "y")
            {
                //echo the message: output = input
                client->out_message = client->in_message;
                TM_Server::SendMessage(client->out_message, client->out_buffer); //echo back for ACK
            }
            //deny transaction
            else if (user_in == "n")
            {
                //set out message
                client->out_message = client->in_message;

                //send some abort code
                client->out_message.code = ABORT;
                TM_Server::SendMessage(client->out_message, client->out_buffer); //echo back for ACK
            }
        }
    }

    cout<<"Client Sent SHUTDOWN command, terminating thread..."<<endl;
//}}}
}
