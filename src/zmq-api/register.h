#ifndef BITCOIN_ZMQAPI_REGISTER_H
#define BITCOIN_ZMQAPI_REGISTER_H

class CZMQTable;

/** Register block chain RPC commands */
// void RegisterBlockchainRPCCommands(CRPCTable &tableRPC);
// list function to register
void SettingModuleZMQReqRep(CZMQTable &tableZMQ); // defined in ./settingModule.cpp
// example: void f(CZMQTable &tableZMQ); // defined in ./f.cpp


static inline void RegisterAllCoreZMQCommands(CZMQTable &tableZMQ)
{
	// TODO register some method here
	SettingModuleZMQReqRep(tableZMQ);
	// f(tableZMQ);
}

#endif // BITCOIN_ZMQAPI_REGISTER_H
