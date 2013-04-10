#include "TM_Server.h"

using namespace std;

struct inputArgs
{
    int memSize;
};

bool HandleArgs(int argc, char *argv[], inputArgs &input);

int main (int argc, char *argv[])
{
    inputArgs cmdInput;
        cmdInput.memSize = 10;      //set default memory size

    if(HandleArgs(argc, argv, cmdInput))
        return 0;

    TM_Server server(cmdInput.memSize);

    server.Start_Server();
   
    return 0;
}

bool HandleArgs(int argc, char *argv[], inputArgs &input)
{
    for(int i = 0; i < argc; i++)
    {
       if(strcmp(argv[i], "-h") == 0)
        {
            cout<<"\t---Transactional Memory Server Usage--"<<endl;
            cout<<"-m\t\tNumber of memory locations, indexed from 0"<<endl;
            return true;
        }

        if(strcmp(argv[i], "-m") == 0)
            input.memSize = atoi(argv[i+1]);

    }
    return false;
}
