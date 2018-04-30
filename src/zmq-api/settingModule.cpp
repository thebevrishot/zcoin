#include <univalue.h>
#include "utilstrencodings.h"
#include "zmq-api/server.h"

// for example ../rpc/register.h

UniValue getSetting(const UniValue& params,bool fHelp)
{
	// TODO
	// ... logic here
	// return result for getSetting
	// return setting value as JSON format
	return 12;
}

// TODO add all command here
static const CZMQCommand commands[] =
{ //  category              name                      actor (function)
  //  --------------------- ------------------------  -----------------------
    { "foo",				"bar",						&getSetting},
};

void SettingModuleZMQReqRep(CZMQTable &tableZMQ)
{
	// NOTE this function append all command in list commands to global tableZMQ
	for(unsigned int vcidx = 0 ;vcidx < ARRAYLEN(commands);vcidx++)
	{
		tableZMQ.appendCommand(commands[vcidx].name,&commands[vcidx]);
	}
}
