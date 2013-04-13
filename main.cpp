#include "TM_Server.h"

using namespace std;

struct inputArgs
{
    int memSize;
    unsigned int port;
    string addr;
};

bool HandleArgs(int argc, char *argv[], inputArgs &input);

int main (int argc, char *argv[])
{
//{{{
    inputArgs cmdInput;
        cmdInput.memSize = 10;      //set default memory size

    if(HandleArgs(argc, argv, cmdInput))
        return 0;

    TM_Server server(cmdInput.memSize);

    server.Start_Server();
   
    return 0;
//}}}
}

bool HandleArgs(int argc, char *argv[], inputArgs &input)
{
//{{{
    for(int i = 0; i < argc; i++)
    {
       if(strcmp(argv[i], "-h") == 0)
        {
            cout<<"\t---Transactional Memory Server Usage--"<<endl;
            cout<<"-ip\t\t Set listen ip address, defaults to \"any\""<<endl;
            cout<<"-port\t\t Set listen port, default is 1337"<<endl;
            cout<<"-m\t\tNumber of memory locations, indexed from 0"<<endl;
            return true;
        }

        if(strcmp(argv[i], "-m") == 0)
            input.memSize = atoi(argv[i+1]);
        if(strcmp(argv[i], "-ip") == 0)
            input.addr = argv[i+1];
        if(strcmp(argv[i], "-port") == 0)
            input.port = atoi(argv[i+1]);
    }
    return false;
//}}}
}
