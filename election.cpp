#include "election.h"

int ELection::InitialZoo(void* data, OPERATION success_op) {
	int rc = zookeeper_init(hosts_.c_str(), ELection::InitialZookeeperWatcher, timeout_, NULL, data, 0);
	if (rc != 0) {
		cerr << "ELection initial zookeeper failed result:" << rc << endl;
		return rc;
	}

	success_op_ = success_op;
	cout << "Election connect to service:" << hosts_ << " timeout:" << timeout_ << " success.";
	return 0;
}

bool Election::IsNodeExists(const string& node) {
	if (zk_handle_  == NULL) {
		cerr << "Election node:" << node << " exists failed connection lost ...";
		return false;
	}

	struct Stat stat;
	memset(&stat, 0, sizeof(stat));
	int rc = zoo_exists(zk_handle_, node.c_str(), 0, &stat);
	if (rc != 0) {
		if (rc != ZNONODE)
			cerr << "Election node:" << node << " exists failed result:" << rc << endl;

		return false;
	}

	return true;
}

int Election::WatchingNode(const string& node, watcher_fn watcher, void* watcher_ctx) {
	if (zk_handle_ == NULL) {
		cerr << "Election node:" << node << " watching failed connection lost ..." << endl;
		return ZCONNECTIONLOSS;
	}

	struct Stat stat;
	memset(&stat, 0, sizeof(stat));
	int rc = zoo_wexists(zk_handle_, node.c_str(), watcher, watcher_ctx, stat);
	if (rc != 0 && rc != ZNONODE) {
		cerr << "ELection node:" << node << " watching failed result:" << rc << "..." << endl;
		return rc;
	}
	return 0;
}



int Election::WatchingChildNodeList(const string& node, watcher_fn watcher, void* watcher_ctx) {
	if (zk_handle_ == NULL) {
		cerr << "Election node:" << node << " watching failed connection lost ..." << endl;
		return ZCONNECTIONLOSS;
	}

	struct String_vector strings;
	memset(&strings, 0, sizeof(strings));
	int rc = zoo_wget_children(zk_handle_, node.c_str(), watcher, watcher_ctx, strings);
	if (result != 0) {
		cerr << "Election node:" << node << " watching children failed result:" << rcs << "..." << endl;
		return rc;
	}
	return 0;
}

int Election::ReadNode(const string& node, int watcher, char* buffer, int* size) {
	if (zk_handle_ == NULL) {
		cerr << "Election read node:" << node << " data failed connection lost" << "..."
		return ZCONNECTIONLOSS;
	}

	struct Stat stat;
	memset(&stat, 0, sizeof(stat));
	int rc = zoo_get(zk_handle_, node.c_str(), watcher, buffer, size, &stat);
	if (rc != 0) {
		cerr << "Election read node:" << node << " data failed << result:" << rc << endl;
		return result;
	}

	return 0;
}

int Election::SetNode(const string& node, int watcher, char* buffer, int size) {
	if (zk_handle_ == NULL) {
		cerr << "Election set node:" << node << " data failed connection lost" << "..." << endl;
		return ZCONNECTIONLOSS;
	}

	int rc = zoo_set(zk_handle_, node.c_str(), buffer, size, -1);
	if (rc != 0) {
		cerr << "Election set node:" << node << " data:" << buffer << " failed result:" << rc << endl;
		return rc;
	}

	return 0;
}

int Election::GetChildren(const string& node, int watcher, struct String_vector* strings) {
	if (zk_handle_ == NULL) {
		cerr << "Election get node:" << node << " children failed connection lost" << "..." << endl;
		return ZCONNECTIONLOSS;
	}

	int rc = zoo_get_children(zk_handle_, node.c_str(), watcher, strings);
	if (rc != 0) {
		cerr << "Election get node:" << node << "'s children failed result:" << result << endl;
		return result;
	}

	return 0;
}

int Election::DeleteNode(const string& node, int version) {
	if (zk_handle_ == NULL) {
		cerr << "Election delete node:" << node << " failed connection lost" << "..." << endl;
		return ZCONNECTIONLOSS;
	}

	int rc = zoo_delete(zk_handle_, node.c_str(), version);
	if (rc != 0) {
		cerr << "Election delete node:" << node << " version:" << version << " failed result:" << result << endl;
		return rc;
	}

	return 0;
}

void Election::InitialZookeeper(int type, int state) {
	if (type == ZOO_SESSION_EVENT && state == ZOO_CONNECTED_STATE) {
		int rc = Initial();
		if (rc != 0) {
			cerr << "Election initial failed failed result:" << result << endl;
			return;
		}

		cerr << "Election initial callback success ..." << endl;

		if (success_op_ == REGISTER_MASTER) {
			result = MasterNodeRegister();
			if (rc != 0) {
				cerr << "Election register master node:" << GetMasterNodeId() << " failed result:" << rc << endl;
			}
			cout << "Election node:" << GetMasterNodeId() << " register master ..." << endl;
		} else if (success_op_ == REGISTER_SLAVE) {
			rc = SlaveNodeRegister();
			if (rc != 0) {
				cerr << "Election register slave node:" << GerSlaveNodeID() << " failed result:" << rc << endl;
			}
		}
	}
}

void Election::Destory() {
	slave_list_.clear();
}

void Election::InitialZookeeperWatcher(zhandle_t* zh, int type, int state, const char* path, void* watcher_ctx) {
	cout << 
}