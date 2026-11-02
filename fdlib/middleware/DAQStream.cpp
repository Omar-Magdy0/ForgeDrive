#include "DAQStream.h"
#include <string.h>
using namespace daq;
void Session::annonate()
{
    uint8_t cid = 0;
    id = cid++;
    for(size_t i = 0; i < streams.size(); i++)
    {
        streams[i].id = cid++;
        for(size_t x = 0; x < streams[i].channels.size(); x++)
        {
            streams[i].channels[x].id = cid++;
        }
    }
}

DAQ_ERR Session::discovery_req()
{


}

DAQ_ERR Session::discovery_respond()
{

    annonate();
   
    return DAQ_ERR();
}

uint8_t Session::process(const uint8_t *data, uint16_t data_len)
{

    return 0;
}


