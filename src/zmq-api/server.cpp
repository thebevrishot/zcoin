#include "zmq-api/server.h"

static bool fZMQRunning = false;

bool IsZMQRunning() {
	return fZMQRunning;
}

CZMQTable::CZMQTable()
{
	// TODO add some default command
}

const CZMQCommand *CZMQTable::operator[](const std::string &name) const
{
	std::map<std::string, const CZMQCommand*>::const_iterator it = mapCommands.find(name);
	if (it == mapCommands.end())
		return NULL;
	return (*it).second;
}

bool CZMQTable::appendCommand(const std::string& name, const CZMQCommand* pcmd)
{
    if (IsZMQRunning())
        return false;

    // don't allow overwriting for now
    std::map<std::string, const CZMQCommand*>::const_iterator it = mapCommands.find(name);
    if (it != mapCommands.end())
        return false;

    mapCommands[name] = pcmd;
    return true;
}

// TODO define UniValue CZMQTable::execute() (const std::string &method, const UniValue &params) const;
// to call command that have been registered

CZMQTable tableZMQ;
