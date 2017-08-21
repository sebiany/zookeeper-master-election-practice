#ifndef __ELECTION_SINGLE_MATER
#define __ELECTION_SINGLE_MATER

#include <stdlib.h>
#include <vector>
#include <algorithm>
#include "election.h"
using namespace std;


class ElectionSingleMaster : public Election {
public:
	ElectionSingleMaster(const string& hosts, int timeout, const string& path, const string& node_id): 
															Election(hosts, timeout, path),
															node_id_(node_id), 
															master_node_id(path+"_master"),
															slave_parent_node_id(path+"_slave_list") {}

	virtual ~ElectionSingleMaster() {}

public:
	virtual int MasterNodeRegister();
	virtual int SlaveNodeRegister();
	virtual int Initial();
	virtual int ReadMasterAndSlaveList() { return 0; }

	virtual void Dump();
	virtual const string GetNodeID() { return node_id_; }
	virtual const string GetMasterNodeID() { return master_node_id_; }
	virtual const string GetSlaveParentNodeID() { return slave_parent_node_id_; }
	virtual const string GetSlaveNodeID() { return (slave_parent_node_id_+"/"+node_id_); }

public:
	// virtual int working(void* parameter);

public:
	int ReadMasterNodeData();
	int ReadSlaveParentChildren();
	int SlaveParentNodeRegister();
	// int RegisterMasterDelay(int millisecond);

	bool IsSlaveParentChild();
	void Watcher(int type, int state, const char* node);
	void MasterWatcher(int type, int state);
	void SlaveParentWatcher(int type, int state);

	void MasterRegCompletion(int rc);
	void SlaveParentCompletion(int rc);
	void SlaveNodeRegCompletion(int rc);
	void RegCompletion(int rc, OPERATION op);

public:
	static void Watcher(zhandle_t* zh, int type, int state, const char* node, void* watcher_ctx);
	static void RegisterCompletion(int rc, const char* node, const void* data);

private:
	string node_id_;
	string master_node_id_;
	string slave_parent_node_id_;
};


#endif