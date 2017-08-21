#ifndef __ELECTION_H
#define __ELECTION_H

#include <stdlib.h>
#include <vector>
#include <zookeeper/zookeeper.h>
using namespace std;

class Election {
public:
	typedef vector<string> MasterList;
	typedef vector<string> SlaveList;
	typedef string Master;

	enum OPERATION {
		REGISTER_MASTER = 1,
		REGISTER_SLAVE = 2,
		REGISTER_SLAVE_PARENT = 3,
		WATCHING_MASTER = 4,
		WATCHING_SLAVE = 5,
		DELETE_SLAVE = 6,
		READ_MASTER_SLAVE_LIST = 7,
		OP_UNKNOWN = 99
	};

	typedef struct register_completion_data_ {
		OPERATION op_;
		void* election_;

		register_completion_data_() {
			op_ = OP_UNKNOWN;
			election_ = NULL;
		}
	} RegisterCompletionData;

public:
	Election(const string& hosts, int timeout, const string& path): path_(path), hosts_(hosts), timeout_(timeout),
																	is_master_(false), master_id_(""), 
																	success_op_(OP_UNKNOWN), failed_op_(OP_UNKNOWN) {}

	virtual ~Election() {Destory(); }

	virtual int MasterNodeRegister() = 0;
	virtual int SlaveNodeRegister() = 0;
	virtual int Initial() = 0;
	virtual int ReadMasterAndSlaveList() = 0;

public:
	int InitialZoo(void* data, OPERATION success_op);
	bool IsNodeExists(const string& node);
	int WatchingNode(const string& node, watcher_fn watcher, void* watcher_ctx);
	int WatchingChildNodeList(const string& node, watcher_fn watcher, void* watcher_ctx);
	int ReadNode(const string& node, int watcher, char* buffer, int* size);
	int SetNode(const string& node, int watcher, char* buffer, int size);
	int GetChildren(const string& node, int watcher, struct String_vector* strings);
	int DeleteNode(const string& node, int version);

public:
	virtual void Dump() = 0;
	virtual const string GetNodeID() = 0;
	virtual const string GetMasterNodeID() = 0;
	virtual const string GetSlaveParentNodeID() = 0;
	virtual const string GetSlaveNodeID() = 0;

public:
	bool IsMaster() { return is_master_; }
	const string GeetMasterID() { return master_id_; }
	const SlaveList GetSlaveList() { return slave_list_; }

public:
	void InitialZookeeper(int type, int state);
	static void InitialZookeeperWatcher(zhandle_t* zh, int type, int state, const char* path, void* watcher_ctx);
	void Destory();

protected:
	string path_;
	string hosts_;
	int timeout_;
	bool is_master_;
	Master master_id_;
	OPERATION success_op_;
	OPERATION failed_op_;
	SlaveList slave_list_;

	zhandle_t* zk_handle_;
};


#endif