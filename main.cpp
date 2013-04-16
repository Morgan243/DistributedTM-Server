#include "TM_Server.h"

using namespace std;

struct inputArgs
{
    int memSize;
    int display_parse_delay;
    unsigned int port;
    string addr;
    bool benchmark;
    Mode lock_mode;
};

bool HandleArgs(int argc, char *argv[], inputArgs &input);

int main (int argc, char *argv[])
{
//{{{
    inputArgs cmdInput;
        cmdInput.memSize = 10;      //set default memory size
        cmdInput.addr = "any";
        cmdInput.port = 1337;
        cmdInput.benchmark = false;
        cmdInput.lock_mode = opt_md;
        cmdInput.display_parse_delay = 5000;

    if(HandleArgs(argc, argv, cmdInput))
        return 0;

    TM_Server server(cmdInput.memSize, cmdInput.addr, cmdInput.port, cmdInput.benchmark, cmdInput.lock_mode);

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
            cout<<"-b\t\tEnable critical section parallelism benchmarking"<<endl;
            cout<<"-cm\t\tConflict detection method: \"mutex\", \"rw\" (read/write), \"opt\" (optimistic)"<<endl;
            cout<<"-s\t\tMicroseconds between messages to display clients"<<endl;

            return true;
        }

        if(strcmp(argv[i], "-m") == 0)
            input.memSize = atoi(argv[i+1]);
        if(strcmp(argv[i], "-ip") == 0)
            input.addr = argv[i+1];
        if(strcmp(argv[i], "-port") == 0)
            input.port = atoi(argv[i+1]);
        if(strcmp(argv[i], "-b") == 0)
            input.benchmark = true;
        if(strcmp(argv[i], "-s") == 0)
            input.display_parse_delay = atoi(argv[i+1]);
        if(strcmp(argv[i], "-cm") == 0)
        {
            if(strcmp(argv[i+1], "mutex") == 0)
                input.lock_mode = mutex_md;
            if(strcmp(argv[i+1], "rw") == 0)
                input.lock_mode = rwMutex_md;
            if(strcmp(argv[i+1], "opt") == 0)
                input.lock_mode = opt_md;
        }
    }
    return false;
//}}}
}
