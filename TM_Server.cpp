#include <mutex>
#include <cstring>
#include <string>
#include "TM_Server.h"

using namespace std;

    //--------------------------
    //Static Var define
    //--------------------------
    //{{{
    std::vector<unsigned int> TM_Server::memory;
    //std::mutex TM_Server::cache_lock;
    AccessCache TM_Server::access_cache(0, opt_md);
    std::vector<Connected_Client> TM_Server::connected_clients;
    NC_Server TM_Server::master_server;
    unsigned int TM_Server::port;
    string TM_Server::address;
    bool TM_Server::done;

    static pthread_mutex_t cache_lock;
    static pthread_mutex_t mem_lock;
    //}}}
    //--------------------------


TM_Server::TM_Server()
{
//{{{
    //set hosting address and port
    //TM_Server::address = "192.168.1.33";
    TM_Server::address = "127.0.0.1";
    TM_Server::port = 1337;

    //setup socket (if not already), set adress, and bind socket
    TM_Server::master_server.Init(true, TM_Server::address, TM_Server::port);
    cout<<"Server initialized on "<<TM_Server::address<<":"<<TM_Server::port<<endl;

    //don't stop until done is true
    TM_Server::done = false;

    pthread_mutex_init(&cache_lock, NULL);
    pthread_mutex_init(&mem_lock, NULL);

    for(int i = 0; i < MEMORY_SIZE; i++)
    {
        memory.push_back(0);
    }
//}}}
}

TM_Server::TM_Server(int memorySize)
{
//{{{
    //set hosting address and port
    //TM_Server::address = "192.168.1.33";
    TM_Server::address = "127.0.0.1";
    TM_Server::port = 1337;

    //setup socket (if not already), set adress, and bind socket
    TM_Server::master_server.Init(true, TM_Server::address, TM_Server::port);
    cout<<"Server initialized on "<<TM_Server::address<<":"<<TM_Server::port<<endl;

    //don't stop until done is true
    TM_Server::done = false;

    pthread_mutex_init(&cache_lock, NULL);
    pthread_mutex_init(&mem_lock, NULL);

    for(int i = 0; i < memorySize; i++)
    {
        memory.push_back(0);
    }
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
        //connected_clients[i].client_thread.join();
        pthread_join(connected_clients[i].client_thread, NULL);
    }

    if(pthread_mutex_destroy(&cache_lock))
        printf("Error destorying cache mutex!\n");
    if(pthread_mutex_destroy(&mem_lock))
        printf("Error destorying cache mutex!\n");

//}}}
}

void TM_Server::SendMessage(TM_Message out_message, unsigned char out_buffer[], int client_id)
{
//{{{
        //clear data buffer
        bzero(out_buffer, sizeof(out_buffer));
    
       //get string version of data
       int size 
           = sprintf((char*)out_buffer, "%c:%u:%u", out_message.code, out_message.address,out_message.value);

        //send out the formatted string
        TM_Server::master_server.Send(out_buffer, size + 1, client_id);
//}}}
}

void TM_Server::ReceiveMessage(string in_buffer, int client_id)
{
//{{{
    unsigned pos1, pos2;
    TM_Message temp_message;

    //make sure buffer is empty
    in_buffer.clear();

    //receive a message from the client
    int bytes_recv = TM_Server::master_server.Receive(&in_buffer, 1024, client_id);

    #if DEBUG
        cout<<"Bytes received in receive message: "<<bytes_recv<<endl;
    #endif

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

        #if DEBUG
            cout<<"CLIENT "<<client_id<<": length = "<<in_buffer.length()<<endl;
            cout<<hex<<"\tcode: "<<(unsigned int)temp_message.code<<endl;
            cout<<hex<<"\taddr: "<<temp_message.address<<endl;
            cout<<hex<<"\tvalue: "<<temp_message.value<<endl;
        #endif
    }

    connected_clients[client_id].in_message = temp_message;
//}}}
}

void TM_Server::Start_Server()
{
//{{{
       
    cout<<"Beginning listen..."<<endl; 

    //prepare for clients
    TM_Server::master_server.Listen(20);
   
    //loop continues until end
    while(!done)
    {
        Connected_Client temp_client;
        help_launchArgs temp_args;

        temp_client.in_buffer.clear();
        temp_client.name.clear();
        temp_client.client_done = false;

        cout<<"Accepting new client..."<<endl;

        //accept a client, get its id
        temp_client.id = TM_Server::master_server.Accept();

        //receive name of client
        TM_Server::master_server.Receive(&temp_client.in_buffer, 1024, temp_client.id);

        
        cout<<">>Clent name declared as: "<<temp_client.in_buffer<<endl;
        temp_client.name = temp_client.in_buffer;
        if(temp_client.name != "DISPLAY")
        {
            //initial operation is zero
            //TM_Server::access_cache.AddProcessor(temp_client.name, 0); 
            access_cache.AddStores();

            //store the client in vector
            connected_clients.push_back(temp_client);


            cout<<"Launching client thread..."<<endl;

            //setup pthreads helper function arguments
            //temp_args.client = &connected_clients.back();
            temp_args.id = connected_clients.size() - 1;
            temp_args.context = this;

            //launch the clients thread into LaunchClient
            pthread_create(&connected_clients.back().client_thread, NULL, help_launchThread, &temp_args);
        }
        else
        {
            cout<<"Display server connected!"<<endl;

        }
    }
   //}}}
}


void TM_Server::LaunchClient(int client_id)
{
//{{{
    string user_in;
    connected_clients[client_id].client_done = false;

    #if DEBUG
        cout<<"Thread handling client with id "<<connected_clients[client_id].id<<endl;
    #endif

    //stop only if server stops or client shutdowns
    while(!this->done && !connected_clients[client_id].client_done)
    {
        //clear the string buffer
        connected_clients[client_id].in_buffer.erase();

        #if DEBUG
            cout<<"Receiving..."<<endl;
        #endif

        //BLOCKING: receive and parse a single message
        TM_Server::ReceiveMessage(connected_clients[client_id].in_buffer,client_id);

        //check for error(==0)
        if (!connected_clients[client_id].in_message.code)
        {
            cout<<"ERROR, ignoring..."<<endl;
        }
        //check for shutdown code from client
        else if(connected_clients[client_id].in_message.code == 0xFF)
        {
            connected_clients[client_id].client_done = true;
        }
        //otherwise, parse the message and respond
        else
        {
            //decide what to do
            HandleRequest(client_id);

            //send message
            TM_Server::SendMessage(connected_clients[client_id].out_message, connected_clients[client_id].out_buffer, client_id);
        }
    }

    #if DEBUG
        cout<<"Client Sent SHUTDOWN command, terminating thread..."<<endl;
    #endif
//}}}
}

void TM_Server::HandleRequest(int client_id)
{
//{{{
    //check access cache here differently depending on request
    if(connected_clients[client_id].in_message.code == WRITE)
    {
        WriteAttempt(client_id);
    }
    else if(connected_clients[client_id].in_message.code == READ)
    {
        ReadAttempt(client_id);
    }
    else if(connected_clients[client_id].in_message.code == COMMIT)
    {
        CommitAttempt(client_id);
    }
    else if(connected_clients[client_id].in_message.code == SYNC)
    {
        SyncAttempt(client_id);
    }
    else if(connected_clients[client_id].in_message.code == MUTEX)
    {

    }
    else if(connected_clients[client_id].in_message.code & INIT)
    {
        InitAttempt(client_id);
    }
    else if(connected_clients[client_id].in_message.code == (COMMIT | WRITE))
    {
        //{{{
        #if DEBUG
            cout<<connected_clients[client_id].name<<" Address "<<connected_clients[client_id].in_message.address<<" being WRITE committed..."<<endl;
        #endif

        pthread_mutex_lock(&cache_lock);
        
        //set the value in memory
        memory[connected_clients[client_id].in_message.address] = connected_clients[client_id].in_message.value;

        pthread_mutex_unlock(&cache_lock);

        connected_clients[client_id].out_message = connected_clients[client_id].in_message;

        #if DEBUG
            cout<<"\tCommit finished..."<<endl;
        #endif
        //}}}
    }
    else if(connected_clients[client_id].in_message.code == (COMMIT | READ))
    {
        //{{{
        #if DEBUG
            cout<<connected_clients[client_id].name<<" Address "<<connected_clients[client_id].in_message.address<<" being READ committed..."<<endl;
        #endif

        connected_clients[client_id].out_message = connected_clients[client_id].in_message;

        #if DEBUG
            cout<<"\tCommit finished..."<<endl;
        #endif
        //}}}
    }
//}}}
}


void TM_Server::WriteAttempt(int client_id)
{
    //{{{
    #if DEBUG
        cout<<connected_clients[client_id].name<<" attempting write on "<<hex<<connected_clients[client_id].in_message.address<<endl;
    #endif

    #if PROMPT
        //{{{
        cout<<"\tAllow? (y/n)"<<endl;
        cin>>user_in;
        if(user_in == "y")
        {
            cout<<"\tAllowing..."<<endl;
            //echo the message: output = input
            connected_clients[client_id].out_message = connected_clients[client_id].in_message;
        }
        else
        {
            cout<<"\tAborting..."<<endl;
            //set out message
            connected_clients[client_id].out_message = connected_clients[client_id].in_message;

            //send some abort code
            connected_clients[client_id].out_message.code = ABORT;
        }
        //}}}
    #else
        //{{{
        pthread_mutex_lock(&cache_lock);
            //check access cache
            access_cache.setRegs(client_id, WRITE_T, connected_clients[client_id].in_message.address);
            if(access_cache.RunFSM())
            {
                //echo message back 
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
            }
            else
            {
                //echo back but with the abort code set
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
        pthread_mutex_unlock(&cache_lock);
        //}}}
    #endif
    //}}}
}

void TM_Server::ReadAttempt(int client_id)
{
        //{{{
        #if DEBUG
            cout<<connected_clients[client_id].name<<" attempting read on "<<hex<<connected_clients[client_id].in_message.address<<endl;
        #endif

        #if PROMPT
            //{{{
            cout<<"\tAllow? (y/n)"<<endl;
            cin>>user_in;
            if(user_in == "y")
            {
                cout<<"\tAllowing..."<<endl;
                //echo the message: output = input
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
                connected_clients[client_id].out_message.value = memory[connected_clients[client_id].out_message.address];
            }
            else
            {
                cout<<"\tAborting..."<<endl;
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
            //}}}
        #else
            //{{{
            //cache_lock.lock();
            pthread_mutex_lock(&cache_lock);
            //check access cache
            access_cache.setRegs(client_id, READ_T, connected_clients[client_id].in_message.address);
            if(access_cache.RunFSM())
            {
                //echo message back but with th value included
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
                connected_clients[client_id].out_message.value 
                    = memory[connected_clients[client_id].out_message.address];
                
            }
            else
            {
                //echo back but with the abort code set
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
            pthread_mutex_unlock(&cache_lock);
            //}}}
        #endif
        //}}}
}

void TM_Server::CommitAttempt(int client_id)
{
        //{{{
        #if DEBUG
            cout<<connected_clients[client_id].name<<" attempting to begin commit..."<<endl;
        #endif

        #if PROMPT
            //{{{
            cout<<"\tAllow? (y/n)"<<endl;
            cin>>user_in;
            if(user_in == "y")
            {
                cout<<"\tAllowing..."<<endl;
                //echo the message: output = input
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
            }
            else
            {
                cout<<"\tAborting..."<<endl;
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
            //}}}
        #else
            //{{{
            bool abort = false;
            pthread_mutex_lock(&cache_lock);
            //
            //check access cache
            access_cache.setRegs(client_id, COMMIT_T, connected_clients[client_id].in_message.address);
            if(access_cache.RunFSM())
            {
                //echo message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
            }
            else
            {
                //echo back but with the abort code set
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
            pthread_mutex_unlock(&cache_lock);
            //}}}
        #endif
        //}}}
}

void TM_Server::SyncAttempt(int client_id)
{
}

void TM_Server::InitAttempt(int client_id)
{
        //{{{
        #if DEBUG
            cout<<connected_clients[client_id].name<<" attempting init on "<<hex<<connected_clients[client_id].in_message.address<<endl;
        #endif

        #if PROMPT
            //{{{
            cout<<"\tAllow? (y/n)"<<endl;
            cin>>user_in;
            if(user_in == "y")
            {
                cout<<"\tAllowing..."<<endl;
                //echo the message: output = input
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
                connected_clients[client_id].out_message.value = memory[connected_clients[client_id].out_message.address];
            }
            else
            {
                cout<<"\tAborting..."<<endl;
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
            //}}}
        #else
            //{{{
            pthread_mutex_lock(&cache_lock);

            access_cache.setRegs(client_id, READ_T, connected_clients[client_id].in_message.address);
            if(access_cache.RunFSM())
            {
                //echo message back but with th value included
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
                connected_clients[client_id].out_message.value 
                        = memory[connected_clients[client_id].out_message.address];
                
            }
            else
            {
                //echo back but with the abort code set
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
            pthread_mutex_unlock(&cache_lock);
            //}}}
        #endif
        //}}}
}

