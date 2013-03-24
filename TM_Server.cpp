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
    Cache TM_Server::access_cache;
    std::vector<Connected_Client> TM_Server::connected_clients;
    NC_Server TM_Server::master_server;
    unsigned int TM_Server::port;
    string TM_Server::address;
    bool TM_Server::done;

    static pthread_mutex_t cache_lock;
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

    for(int i = 0; i < MEMORY_SIZE; i++)
    {
        access_cache.AddCacheLine();
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

       cout<<"OUT BUFFER : "<<out_buffer<<endl;

        //send out the formatted string
        TM_Server::master_server.Send(out_buffer, size + 1, client_id);

//}}}
}

void TM_Server::ReceiveMessage(string in_buffer, int client_id)
{
//{{{
    unsigned pos1, pos2;
    TM_Message temp_message;

    in_buffer.clear();

    int bytes_recv = TM_Server::master_server.Receive(&in_buffer, 1024, client_id);

    cout<<"Bytes received in receive message: "<<bytes_recv<<endl;

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
        cout<<"CLIENT "<<client_id<<": length = "<<in_buffer.length()<<endl;
        cout<<hex<<"\tcode: "<<(unsigned int)temp_message.code<<endl;
        cout<<hex<<"\taddr: "<<temp_message.address<<endl;
        cout<<hex<<"\tvalue: "<<temp_message.value<<endl;
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

        //initial operation is zero
        TM_Server::access_cache.AddProcessor(temp_client.name, 0); 

        //store the client in vector
        connected_clients.push_back(temp_client);

        cout<<"Launching client thread..."<<endl;

        //setup pthreads helper function arguments
        //temp_args.client = &connected_clients.back();
        temp_args.id = connected_clients.size() - 1;
        temp_args.context = this;

        //launch the clients thread into LaunchClient
        pthread_create(&connected_clients.back().client_thread, NULL, help_launchThread, &temp_args);
                        //= new std::thread(&TM_Server::LaunchClient, this, &connected_clients.back());
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
        //connected_clients[client_id].in_message = TM_Server::ReceiveMessage(connected_clients[client_id].in_buffer,client_id);
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
    //check access cache here
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
        //cache_lock.lock();
        //set the value in memory
        memory[connected_clients[client_id].in_message.address] = connected_clients[client_id].in_message.value;

        //clear the operation bits
        access_cache.SetProcessorOperation(connected_clients[client_id].in_message.address, connected_clients[client_id].name, 0);
        //cache_lock.unlock();
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

        //cache_lock.lock();
        pthread_mutex_lock(&cache_lock);
        //clear the operation bits
        access_cache.SetProcessorOperation(connected_clients[client_id].in_message.address, connected_clients[client_id].name, 0);
        //cache_lock.unlock();
        pthread_mutex_unlock(&cache_lock);

        connected_clients[client_id].out_message = connected_clients[client_id].in_message;
        //SendMessage(client->in_message, client->out_buffer);

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
        //cache_lock.lock();
        //check access cache
        if(access_cache.GetMemoryOperations(connected_clients[client_id].in_message.address, READ_SET).empty()
           || access_cache.GetMemoryOperations(connected_clients[client_id].in_message.address, WRITE_SET).empty())
        {
            #if DEBUG
                cout<<"\tAllowing..."<<endl;
            #endif

            //echo the message: output = input
            connected_clients[client_id].out_message = connected_clients[client_id].in_message;
            access_cache.SetProcessorOperation(connected_clients[client_id].in_message.address, connected_clients[client_id].name, WRITE_SET);
        }
        else
        {
            #if DEBUG
                cout<<"\tAborting..."<<endl;
            #endif

            //set out message
            connected_clients[client_id].out_message = connected_clients[client_id].in_message;

            //send some abort code
            connected_clients[client_id].out_message.code = ABORT;
        }
        //cache_lock.unlock();
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
            if(access_cache.GetMemoryOperations(connected_clients[client_id].in_message.address, READ_SET).empty()
               || access_cache.GetMemoryOperations(connected_clients[client_id].in_message.address, WRITE_SET).empty())
            {
                #if DEBUG
                    cout<<"\tAllowing..."<<endl;
                #endif
                //echo the message: output = input
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
                connected_clients[client_id].out_message.value = memory[connected_clients[client_id].out_message.address];
               
            }
            else
            {
                #if DEBUG
                    cout<<"\tAborting..."<<endl;
                #endif
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
            pthread_mutex_lock(&cache_lock);
            //cache_lock.unlock();
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

            //first, get all addresses client is using in transaction
            connected_clients[client_id].client_ops = access_cache.GetProcessorOperations(connected_clients[client_id].name);
            for(int i = 0; (i < connected_clients[client_id].client_ops.size()) && !abort; i++)
            {
                //make sure that only this processor has sets: this logic needs to be much more complex
                if(connected_clients[client_id].client_ops[i] == READ_SET)
                {
                    //should check commit write set here in order to be more than just r/w lock
                    if(access_cache.GetMemoryOperations(i, WRITE_SET).size())
                    {
                        #if DEBUG
                            cout<<"i = "<<i<<", Aborted READ due to WRTIE"<<endl;
                        #endif
                        abort = true;
                    }
                }
                else if(connected_clients[client_id].client_ops[i] == WRITE_SET)
                {
                    //should be checking write sets; (>1) to account for itself
                    if(access_cache.GetMemoryOperations(i, READ_SET).size() 
                    || access_cache.GetMemoryOperations(i, WRITE_SET).size() > 1)
                    {
                        #if DEBUG
                            cout<<"Aborted WRITE due to WRTIE"<<endl;
                        #endif
                        abort = true;
                    }
                }
            }
            pthread_mutex_unlock(&cache_lock);

            //check access cache
            if(!abort)
            {
                #if DEBUG
                     cout<<"\tAllowing..."<<endl;
                #endif

                //echo the message: output = input
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
               
            }
            else
            {
                #if DEBUG
                    cout<<"\tAborting..."<<endl;
                #endif

                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
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
            //cache_lock.lock();
            //check access cache; should check commit write set
            if(access_cache.GetMemoryOperations(connected_clients[client_id].in_message.address, WRITE_SET).empty())
            {
                #if DEBUG
                    cout<<"\tAllowing..."<<endl;
                #endif
                //echo the message: output = input
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;
                connected_clients[client_id].out_message.value = memory[connected_clients[client_id].out_message.address];
                access_cache.SetProcessorOperation(connected_clients[client_id].in_message.address, connected_clients[client_id].name,READ_SET);
            }
            else
            {
                #if DEBUG
                    cout<<"\tAborting..."<<endl;
                #endif
                //set out message
                connected_clients[client_id].out_message = connected_clients[client_id].in_message;

                //send some abort code
                connected_clients[client_id].out_message.code = ABORT;
            }
            pthread_mutex_unlock(&cache_lock);
            //cache_lock.unlock();
            //}}}
        #endif
        //}}}
}



//void TM_Server::WriteAttempt(Connected_Client *client, TM_Message *in_msg, TM_Message *out_msg)
//{
//    //{{{
//    #if DEBUG
//        cout<<client->name<<" attempting write on "<<hex<<client->in_message.address<<endl;
//    #endif
//
//    #if PROMPT
//        //{{{
//        cout<<"\tAllow? (y/n)"<<endl;
//        cin>>user_in;
//        if(user_in == "y")
//        {
//            cout<<"\tAllowing..."<<endl;
//            //echo the message: output = input
//            client->out_message = client->in_message;
//        }
//        else
//        {
//            cout<<"\tAborting..."<<endl;
//            //set out message
//            client->out_message = client->in_message;
//
//            //send some abort code
//            client->out_message.code = ABORT;
//        }
//        //}}}
//    #else
//        //{{{
//        pthread_mutex_lock(&cache_lock);
//        //cache_lock.lock();
//        //check access cache
//        if(access_cache.GetMemoryOperations(client->in_message.address, READ_SET).empty()
//           || access_cache.GetMemoryOperations(client->in_message.address, WRITE_SET).empty())
//        {
//            #if DEBUG
//                cout<<"\tAllowing..."<<endl;
//            #endif
//
//            //echo the message: output = input
//            client->out_message = client->in_message;
//            access_cache.SetProcessorOperation(client->in_message.address, client->name, WRITE_SET);
//        }
//        else
//        {
//            #if DEBUG
//                cout<<"\tAborting..."<<endl;
//            #endif
//
//            //set out message
//            client->out_message = client->in_message;
//
//            //send some abort code
//            client->out_message.code = ABORT;
//        }
//        //cache_lock.unlock();
//        pthread_mutex_unlock(&cache_lock);
//        //}}}
//    #endif
//    //}}}
//}
//
//void TM_Server::ReadAttempt(Connected_Client *client, TM_Message *in_msg, TM_Message *out_msg)
//{
//        //{{{
//        #if DEBUG
//            cout<<client->name<<" attempting read on "<<hex<<client->in_message.address<<endl;
//        #endif
//
//        #if PROMPT
//            //{{{
//            cout<<"\tAllow? (y/n)"<<endl;
//            cin>>user_in;
//            if(user_in == "y")
//            {
//                cout<<"\tAllowing..."<<endl;
//                //echo the message: output = input
//                client->out_message = client->in_message;
//                client->out_message.value = memory[client->out_message.address];
//            }
//            else
//            {
//                cout<<"\tAborting..."<<endl;
//                //set out message
//                client->out_message = client->in_message;
//
//                //send some abort code
//                client->out_message.code = ABORT;
//            }
//            //}}}
//        #else
//            //{{{
//            //cache_lock.lock();
//            pthread_mutex_lock(&cache_lock);
//            //check access cache
//            if(access_cache.GetMemoryOperations(client->in_message.address, READ_SET).empty()
//               || access_cache.GetMemoryOperations(client->in_message.address, WRITE_SET).empty())
//            {
//                #if DEBUG
//                    cout<<"\tAllowing..."<<endl;
//                #endif
//                //echo the message: output = input
//                client->out_message = client->in_message;
//                client->out_message.value = memory[client->out_message.address];
//               
//            }
//            else
//            {
//                #if DEBUG
//                    cout<<"\tAborting..."<<endl;
//                #endif
//                //set out message
//                client->out_message = client->in_message;
//
//                //send some abort code
//                client->out_message.code = ABORT;
//            }
//            pthread_mutex_lock(&cache_lock);
//            //cache_lock.unlock();
//            //}}}
//        #endif
//        //}}}
//}
//
//void TM_Server::CommitAttempt(Connected_Client *client, TM_Message *in_msg, TM_Message *out_msg)
//{
//        //{{{
//        #if DEBUG
//            cout<<client->name<<" attempting to begin commit..."<<endl;
//        #endif
//
//        #if PROMPT
//            //{{{
//            cout<<"\tAllow? (y/n)"<<endl;
//            cin>>user_in;
//            if(user_in == "y")
//            {
//                cout<<"\tAllowing..."<<endl;
//                //echo the message: output = input
//                client->out_message = client->in_message;
//            }
//            else
//            {
//                cout<<"\tAborting..."<<endl;
//                //set out message
//                client->out_message = client->in_message;
//
//                //send some abort code
//                client->out_message.code = ABORT;
//            }
//            //}}}
//        #else
//            //{{{
//            bool abort = false;
//            pthread_mutex_lock(&cache_lock);
//
//            //first, get all addresses client is using in transaction
//            client->client_ops = access_cache.GetProcessorOperations(client->name);
//            for(int i = 0; (i < client->client_ops.size()) && !abort; i++)
//            {
//                //make sure that only this processor has sets: this logic needs to be much more complex
//                if(client->client_ops[i] == READ_SET)
//                {
//                    //should check commit write set here in order to be more than just r/w lock
//                    if(access_cache.GetMemoryOperations(i, WRITE_SET).size())
//                    {
//                        #if DEBUG
//                            cout<<"i = "<<i<<", Aborted READ due to WRTIE"<<endl;
//                        #endif
//                        abort = true;
//                    }
//                }
//                else if(client->client_ops[i] == WRITE_SET)
//                {
//                    //should be checking write sets; (>1) to account for itself
//                    if(access_cache.GetMemoryOperations(i, READ_SET).size() 
//                    || access_cache.GetMemoryOperations(i, WRITE_SET).size() > 1)
//                    {
//                        #if DEBUG
//                            cout<<"Aborted WRITE due to WRTIE"<<endl;
//                        #endif
//                        abort = true;
//                    }
//                }
//            }
//            pthread_mutex_unlock(&cache_lock);
//
//            //check access cache
//            if(!abort)
//            {
//                #if DEBUG
//                     cout<<"\tAllowing..."<<endl;
//                #endif
//
//                //echo the message: output = input
//                client->out_message = client->in_message;
//               
//            }
//            else
//            {
//                #if DEBUG
//                    cout<<"\tAborting..."<<endl;
//                #endif
//
//                //set out message
//                client->out_message = client->in_message;
//
//                //send some abort code
//                client->out_message.code = ABORT;
//            }
//            //}}}
//        #endif
//        //}}}
//}
//
//void TM_Server::SyncAttempt(Connected_Client *client, TM_Message *in_msg, TM_Message *out_msg)
//{
//}
//
//void TM_Server::InitAttempt(Connected_Client *client, TM_Message *in_msg, TM_Message *out_msg)
//{
//        //{{{
//        #if DEBUG
//            cout<<client->name<<" attempting init on "<<hex<<client->in_message.address<<endl;
//        #endif
//
//        #if PROMPT
//            //{{{
//            cout<<"\tAllow? (y/n)"<<endl;
//            cin>>user_in;
//            if(user_in == "y")
//            {
//                cout<<"\tAllowing..."<<endl;
//                //echo the message: output = input
//                client->out_message = client->in_message;
//                client->out_message.value = memory[client->out_message.address];
//            }
//            else
//            {
//                cout<<"\tAborting..."<<endl;
//                //set out message
//                client->out_message = client->in_message;
//
//                //send some abort code
//                client->out_message.code = ABORT;
//            }
//            //}}}
//        #else
//            //{{{
//            pthread_mutex_lock(&cache_lock);
//            //cache_lock.lock();
//            //check access cache; should check commit write set
//            if(access_cache.GetMemoryOperations(client->in_message.address, WRITE_SET).empty())
//            {
//                #if DEBUG
//                    cout<<"\tAllowing..."<<endl;
//                #endif
//                //echo the message: output = input
//                client->out_message = client->in_message;
//                client->out_message.value = memory[client->out_message.address];
//                access_cache.SetProcessorOperation(client->in_message.address, client->name,READ_SET);
//            }
//            else
//            {
//                #if DEBUG
//                    cout<<"\tAborting..."<<endl;
//                #endif
//                //set out message
//                client->out_message = client->in_message;
//
//                //send some abort code
//                client->out_message.code = ABORT;
//            }
//            pthread_mutex_unlock(&cache_lock);
//            //cache_lock.unlock();
//            //}}}
//        #endif
//        //}}}
//}
