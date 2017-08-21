#include "election_single_master.h"

int ElectionSingleMaster::Initial() {
	master_id = "";
	is_master_ = false;
	slave_list_.clear();

	if (zk_handle_ == NULL) {
		cout << "ElectionSingleMaster initial zookeeper connection ..." << endl;
		int rc = InitialZoo(reinterpret_cast<void*>(this), REGISTER_MASTER);
		if (rc != 0) {
			cerr << "ElectionSingleMaster initial zookeeper connection failed result:" << rc << endl;
			return rc;
		}

		return 0;
	}

	int rc = WatchingNode(master_node_id_, ElectionSingleMaster::Watcher, reinterpret_cast<void*>(this));
	if (rc != 0) {
		cerr << "ElectionSingleMaster watching master node:" << master_node_id_ << " failed reesult:" << rc << endl;
		return rc;
	}
	cout << "ElectionSingleMaster watching master node:" << master_node_id_ << " success ..." << endl;

	rc = WatchingNode(slave_parent_node_id_, ElectionSingleMaster::Watcher, reinterpret_cast<void*>(this));
	if (rc != 0) {
		cerr << "ElectionSingleMaster watching slave parent node:" << slave_parent_node_id_ << " failed result:" << rc << endl;
		return rc;
	}
	cout << "ElectionSingleMaster watching slave parent node:" << slave_parent_node_id_ << " success ..." << endl;

	return 0;
}


int ElectionSingleMaster::MasterNodeRegister() {
	if (zk_handle_ == NULL) {
		cout << "ElectionSingleMaster initial zookeeper connection ..." << endl;
		int rc = InitialZoo(reinterpret_cast<void*>(this), REGISTER_SLAVE);
		if (rc != 0) {
			cerr << "ElectionSingleMaster initial zookeeper connection failed result:" << rc << endl;
			return rc;
		}

		return 0;
	}

	RegisterCompletionData* reg_data = new RegisterCompletionData();
	reg_data->op_ = REGISTER_MASTER;
	reg_data->election_ = reinterpret_cast<void*>(this);

	int rc = zoo_acreate(zk_handle_, master_node_id_.c_str(), node_id_.c_str(), node_id_.length(), 
							&ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, ElectionSingleMaster::RegisterCompletion, 
							reinterpret_cast<void*>(reg_data));
	if (rc != 0) {
		cerr << "ElectionSingleMaster register master node:" << master_node_id_ << " failed result:" << rc << endl;
		return rc;
	}

	cout << "ElectionSingleMaster register master node:" << master_node_id_ << " node id:" << node_id_ << endl;
	return 0;
}

int ElectionSingleMaster::SlaveNodeRegister() {
	if (zk_handle_ == NULL) {
		cerr << "ElectionSingleMaster initial zookeeper connection ..." << endl;
		int rc = InitialZoo(reinterpret_cast<void*>(this), REGISTER_SLAVE);
		if (rc != 0) {
			cerr << "ElectionSingleMaster initial zookeeper connection failed result:" << rc << endl;
			return rc;
		}

		return 0;
	}

	if (!IsNodeExists(slave_parent_node_id_)) {
		int rc = SlaveParentNodeRegister();
		if (rc != 0) {
			cerr << "ElectionSingleMaster register slave parent node:" << slave_parent_node_id_ << " failed result:" << rc << endl;
		}
		cout << "ElectionSingleMaster register slave parent node:" << slave_parent_node_id_ << endl;
		return 0;
	}

	RegisterCompletionData* reg_data = new RegisterCompletionData();
	reg_data->op_ = REGISTER_SLAVE;
	reg_data->election_ = reinterpret_cast<void*>(this);
	const string child_node = slave_parent_node_id_+"/"+node_id_;
	int result =  zoo_acreate(zk_handle_, child_node.c_str(), node_id_.c_str(), node_id_.length(),
								&ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, ElectionSingleMaster::RegisterCompletion,
								reinterpret_cast<void*>(reg_data));

	if (rc != 0) {
		cerr << "ElectionSingleMaster register slave node:" << child_node << " failed result:" << rc << endl;
		return rc;
	}

	cout << "ElectionSingleMaster register slave node:" << child_node << endl;
	return 0;
}


int ElectionSingleMaster::SlaveParentNodeRegister() {
	if (zk_handle_ == NULL) {
		cout << "ElectionSingleMaster initial zookeeper connection ..." << endl;
		int rc = InitialZoo(reinterpret_cast<void*>(this), REGISTER_SLAVE);
		if (rc != 0) {
			cerr << "ElectionSingleMaster initial zookeeper connection failed result:" << rc << endl;
			return rc;
		}

		return 0;
	}

	RegisterCompletionData* reg_data = new RegisterCompletionData();
	reg_data->op_ = REGISTER_SLAVE_PARENT;
	reg_data->election_ = reinterpret_cast<void*>(this);

	int rc = zoo_acreate(slave_parent_node_id_.c_str(), "", 0, 
							&ZOO_OPEN_ACL_UNSAFE, 0, ElectionSingleMaster::RegisterCompletion,
							reinterpret_cast<void*>(reg_data));

	if (rc != 0) {
		cerr << "ElectionSingleMaster register slave parent node:" << slave_parent_node_id_ << " failed result:" << rc;
		return rc;
	}

	cout << "ElectionSingleMaster register slave parent nodes:" << slave_parent_node_id_ << endl;
	return 0;
}

int ElectionSingleMaster::ReadMasterNodeData() {
	char buffer[10*1024] = {0};
	int size = sizeof (buffer);
	int rc = ReadNode(master_node_id_, 0, buffer, &size);
	if (rc != 0) {
		cerr << "ElectionSingleMaster node:" << master_node_id_ << " exists " << " and get node data failed result:" << rc << endl;
		return rc;
	}

	master_id_ = buffer;
	int master_id_len = master_id_.length();
	int node_id_len = node_id_.length();
	if (master_id_len == node_id_len && strncasecmp(master_id_.c_str(), node_id_.c_str(), node_id_len)) {
		is_master_ = true;
	} else {
		is_master_ = false;
	}
}

bool ElectionSingleMaster::IsSlaveParentChild() {
	const string child_node = slave_parent_node_id_ + "/" + node_id_;
	return IsNodeExists(child_node);
}

int ElectionSingleMaster::ReadSlaveParentChildren() {
	struct String_vector strings;
	memset(&strings, 0, sizeof(strings));
	int rc = GetChildren(slave_parent_node_id_, 0, &string);
	if (rc != 0) {
		cerr << "ElectionSingleMaster node:" << slave_parent_node_id_ << " exists " 
								<< " and read children failed result:" << rc << endl;
	}

	slave_list_.clear();
	for (int i = 0; i < strings.count; i++) {
		cout << "index:" << i << " node:" << strings.data[i] << " #############" << endl;
		slave_list_.push_back(string.data[i]);
	}

	if (is_master_ && (find(slave_list_.begin(), slave_list_.end(), node_id_) != slave_list_.end())) {
		cout << "ElectionSingleMaster node:" << node_id_ << " is master and find in slave list ..." << endl;
		rc = ReadMasterNodeData();
		if (rc != 0) {
			cerr << "ElectionSingleMaster read master node:" << master_node_id_ << " failed result:" << rc << endl;
			return rc;
		}
	}

	if (!is_master_ && (find(slave_list_.begin(), slave_list_.end(), node_id_)==slave_list_.end())) {
		cout << "ElectionSingleMaster node:" << node_id_ << " is slave and not found in slave list ..." << endl;
		rc = SlaveNodeRegister();
		if (rc != 0) {
			cerr << "ElectionSingleMaster register slave node:" << node_id_ << " failed result:" << rc << endl;
			return rc;
		}
	}
	return 0;
}


// int ElectionSingleMaster::Working(void* parameter) {
// }

// int ElectionSingleMaster::RegisterMasterDelay(int millisecond) {
// }

void ElectionSingleMaster::Dump() {
	cout << "*****************************************************************" << endl;
	cout << "hosts:" << hosts_ << " timeout:" << timeout_ << " node:" << node_id_ << endl;
	cout << "is master:" << is_master_ << " master node id:" << master_id_ << endl;
	int slave_size = slave_list_.size();
	for (int i = 0; i < slave_size; i++) {
		cout << "index:" << i << " slave node id:" << slave_list_[i] << endl;
	}
	cout << "*****************************************************************" << endl; 
}

void ElectionSingleMaster::MasterRegCompletion(int rc) {
	cout << "ElectionSingleMaster MasterRegCompletion rc:" << rc << endl;
	if (rc == 0) {
		is_master_ = true;
		master_id_ = node_id_;

		if (!IsSlaveParentChild()) {
			return;
		}

		const string child_node = slave_parent_node_id_ + "/" + node_id_;
		int result = DeleteNode(child_node, -1);
		if (result != 0) {
			cerr << "ElectionSingleMaster delete node:" << child_node << " failed result:" << result << endl;
		} else {
			cout << "ElectionSingleMaster MasterRegCompletion delete slave node:" << child_node << " success";
		}

		return;
	}

	int result = ReadMasterNodeData();
	if (result != 0) {
		cerr << "ElectionSingleMaster read master node:" << master_node_id_ << " failed result:"
														<< result << " ..." << endl;
		return;
	}

	if (is_master_) {
		cout << "ElectionSingleMaster MasterRegCompletion node:" << node_id_ << " is master ..." << endl;
		return;
	}

	const string child_node = slave_parent_node_id_ + "/" + node_id_;
	if (IsNodeExists(child_node)) {
		cout << "ElectionSingleMaster MasterRegCompletion node:" << node_id_ << " is slave do not need register ..." << endl;
		return;
	}

	result = SlaveNodeRegister();
	if (result != 0 ) {
		cerr << "ElectionSingleMaster register slave node:" << node_id_ << " failed result:" << result << endl;
	}
	cout << "ElectionSingleMaster register node:" << node_id_ << " being slave ..." << endl;
}


void ElectionSingleMaster::SlaveParentRegCompletion(int rc) {
	cout << "ElectionSingleMaster SlaveParentRegCompletion rc:" << rc << endl;
	if (is_master_) {
		cout << "ElectionSingleMaster node:" << node_id_ << " is master ..." << endl;
		return;
	}

	int result = SlaveNodeRegister();
	if (result != 0) {
		cerr << "ElectionSingleMaster register slave node:" << node_id_ << " failed result:" << result << endl;
	}

	cout << "ElectionSingleMaster register node:" << node_id_ << " being slave ..." << endl;
}

void ElectionSingleMaster::SlaveNodeRegCompletion(int rc) {
	cout << "ElectionSingleMaster SlaveNodeRegCompletion rc:" << rc << endl;
	if (rc == ZNODEEXISTS) {
		cout << "ElectionSingleMaster register node:" << node_id_ << " is exists ..." << endl;
	} else if (rc == 0) {
		cout << "ElectionSingleMaster register node:" << node_id_ << " success ..." << endl;
	}

	int result = ReadSlaveParentChildren();
	if (result != 0) {
		cerr << "ElectionSingleMaster read slave parent node:" << slave_parent_node_id_ << " children failed result:"
																						<< result << endl;
	}

	cout << "ElectionSingleMaster slave node:" << slave_parent_node_id_ << " register success ..." << endl;
}

void ElectionSingleMaster::RegCompletion(int rc, OPERATION op) {
	if (op == REGISTER_MASTER) {
		MasterRegCompletion(rc);
	} else if (op == REGISTER_SLAVE_PARENT) {
		SlaveParentRegCompletion(rc);
	} else if (op == REGISTER_SLAVE) {
		SlaveNodeRegCompletion(rc);
	} else {
		cout << "ElectionSingleMaster register op:" << op << " is undefined ..." << endl;
	}
}


void ElectionSingleMaster::MasterWatcher(int type, int state) {
	cout << "ElectionSingleMaster master watcher type:" << type << " state:" << state << endl;
	int result = WatchingNode(master_node_id_, ElectionSingleMaster::Watcher, reinterpret_cast<void*>(this));
	if (result != 0) {
		cerr << "ElectionSingleMaster watching master node:" << master_node_id_ << " failed result:" << result << endl;
	}

	if (type == ZOO_CREATED_EVENT) {
		cout << "ElectionSingleMaster master node:" << master_node_id_ << " has been created ..." << endl;
		result = ReadMasterNodeData();
		if (result != 0) {
			cerr << "ElectionSingleMaster read master node:" << master_node_id_ << " failed result" << result << " ..." << endl;
			return;
		}

		if (!is_master_)
			return;

		if (!IsSlaveParentChild())
			return;

		const string child_node = slave_parent_node_id_+"/"+node_id_;
		result = DeleteNode(child_node, -1);
		if (result != 0) {
			cerr << "ElectionSingleMaster delete node:" << child_node << " failed result:" << result << endl;
		} 
	} else if (type == ZOO_DELETED_EVENT) {
		cout << "ElectionSingleMaster event is master node:" << master_node_id_ << " has been deleted" << endl;
		if (is_master_) {
			result = MasterNodeRegister();
			if (result != 0) {
				cerr << "ElectionSingleMaster register master node:" << master_node_id_ << " failed result:" << result << endl;
			}
			cout << "ElectionSingleMaster node:" << master_node_id_ << " register master ..." << endl;
			return;
		}
	} else if (type == ZOO_CHANGED_EVENT) {
		cout << "ElectionSingleMaster master node:" << master_node_id_ << " has been changed" << endl;
		result = ReadMasterNodeData();
		if (result != 0) {
			cout << "ElectionSingleMaster read master node:" << master_node_id_ << " failed result:" << result << endl;
			return;
		}
		if (!is_master_)
			return;
		if (!IsSlaveParentChild())
			return;
		const string child_node = slave_parent_node_id_+"/"+node_id_;
		result = DeleteNode(child_node, -1);
		if (result != 0) {
			cerr << "ElectionSingleMaster delete node:" << child_node << " failed result:" << result << endl;
		}
	} else if (type == ZOO_CHILD_EVENT) {
		cout << "ElectionSingleMaster master node:" << master_node_id_ << " children has been changed" << endl;
	} else if (type == ZOO_SESSION_EVENT) {
		cout << "ElectionSingleMaster master node:" << master_node_id_ << " session has lost" << endl;
		zookeeper_close(zk_handle_);
		zk_handle_ = NULL;
		result = Initial();
		if (result != 0) {
			cerr << "ElectionSingleMaster initial failed result:" << result << endl;
		}
		cout << "ElectionSingleMaster initial success" << endl;
	} else if (type == ZOO_NOTWATCHING_EVENT) {
		cout << "ElectionSingleMaster master node:" << master_node_id_ << " watcher is removed" << endl;
	} else {
		cerr << "ElectionSingleMaster master node:" << master_node_id_ << " type:" << type << " state:" << state << endl;
	}
}


void ElectionSingleMaster::SlaveParentWatcher(int type, int state) {
	cout << "ElectionSingleMaster slvae parent watcher type:" << type << " state:" << state << endl;
	int result = WatchingNode(slave_parent_node_id_, ElectionSingleMaster::Watcher, reinterpret_cast<void*>(this));
	if (result != 0) {
		cerr << "ElectionSingleMaster watching slave parent node:" << slave_parent_node_id_ << " failed result:" << result << endl
	}

	result = WatchingChildNodeList(slave_parent_node_id_, ElectionSingleMaster::Watcher, reinterpret_cast<void*>(this));
	if (result != 0) {
		cerr << "ElectionSingleMaster watching slave parent node:" << slave_parent_node_id_ << " children failed result:" << result << endl;
	}

	if (type == ZOO_CREATED_EVENT) {
		cout << "ElectionSingleMaster slave parent node:" << slave_parent_node_id_ << " has created" << endl;
		if (is_master_) {
			cout << "ElectionSingleMaster node:" << node_id_ <<" is mater " << endl;
			return;
		}

		result = SlaveNodeRegister();
		if (result != 0) {
			cerr << "ElectionSingleMaster slave node:" << node_id_ << " register failed" << " result:" << result << endl;
		}
		cout << "ElectionSingleMaster slave node:" << node_id_ << " register" << endl;
	} else if (type == ZOO_DELETED_EVENT) {
		cout << "ElectionSingleMaster slave parent node has been delete" << endl;
		result = SlaveParentNodeRegister();
		if (result != 0) {
			cerr << "ElectionSingleMaster register slave parent node:" << slave_parent_node_id_ << " failed result:" << result << endl;
		}
		cout << "ElectionSingleMaster slave parent node:" << slave_parent_node_id_ << " register" << endl;
	} else if (type == ZOO_CHANGED_EVENT) {
		cout << "ElectionSingleMaster slave parent node:" << slave_parent_node_id_ << " has been changed" << endl;
		result = ReadSlaveParentChildren();
		if (result != 0) {
			cerr << "ElectionSingleMaster read slave list node:" << slave_parent_node_id_ << " failed result:" << result << endl; 
		}
		cout << "ElectionSingleMaster read slave parent node:" << slave_parent_node_id_ << " children list size:" << slave_list_.size() << endl;
	} else if (type == ZOO_CHILD_EVENT) {
		cout << "ElectionSingleMaster slave parent node:" << slave_parent_node_id_ << " child has changed" << endl;
		result = ReadSlaveParentChildren();
		if (result != 0) {
			cerr << "ElectionSingleMaster read slave list node:" << slave_parent_node_id_ << " failed result:" << result << endl;
		}
		cout << "ElectionSingleMaster read slave parent node:" << slave_parent_node_id_ << " children list size:" << slave_list_.size() << endl;

	} else if (type == ZOO_SESSION_EVENT) {
		cout << "ElectionSingleMaster slave parent node:" << slave_parent_node_id_ << " session has lost" << endl;
		zookeeper_close(zk_handle_);
		zk_handle_ = NULL;
		result = Initial();
		if (result != 0) {
			cerr << "ElectionSingleMaster initial failed result:" << result << endl;
		}
		cout << "ElectionSingleMaster initial success ..." << endl;
	} else if (type == ZOO_NOTWATCHING_EVENT) {
		cout << "ElectionSingleMaster slave parent node:" << slave_parent_node_id_ << " watcher is removed ..." << endl;
	} else {
		cerr << "ElectionSingleMaster slave parent node:" << slave_parent_node_id_ << " type:" << type << " state:" << state << endl;
	}
}

void ElectionSingleMaster::Watcher(int type, int state, const char* node) {
	int node_len = strlen(node);
	int master_node_id_len = master_node_id_.length();
	int slave_parent_node_len = slave_parent_node_id_.length();
	if (node_len == master_node_id_len && strncasecmp(node, master_node_id_.c_str(), node_len)) {
		MasterWatcher(type, state);
	} else if (node_len == slave_parent_node_len && strncasecmp(node, slave_parent_node_id_.c_str(), node_len)) {
		SlaveParentWatcher(type, state);
	} else {
		cerr << "ElectionSingleMaster node watching error node:" << node << " is invalid ..." << endl;
	}
}

void ElectionSingleMaster::Watcher(zhandle_t* zh, int type, int state, const char* node, void* watcher_ctx) {
	cout << "ElectionSingleMaster Watcher zookeeper handle:" << zh << " type:" << type
															 << " state:" << state
															 << " node:" << node << endl;
	ElectionSingleMaster* election = reinterpret_cast<ElectionSingleMaster*> (const_cast<void*>(watcher_ctx));
	election->Watcher(type, state, node);
	election->Dump();
}

void ElectionSingleMaster::RegisterCompletion(int rc, const char* node, const void* data) {
	cout << "ElectionSingleMaster RegisterCompletion rc:" << rc << " node:" << node << endl;
	RegisterCompletionData* reg_data = reinterpret_cast<RegisterCompletionData*>(const_cast<void*>(data));
	if (reg_data->op_ || reg_data->election_) {
		cerr << "ElectionSingleMaster RegisterCompletion register parameters is invalid" << endl;
		return;
	}

	ElectionSingleMaster* election = reinterpret_cast<ElectionSingleMaster*>(const_cast<void*>(reg_data->election_));

	if (rc == ZNOAUTH) {
		cerr << "ElectionSingleMaster RegisterCompletion rc:" << rc << " node:" << node << " auth failed" << endl;
	} else if (rc != ZOK && rc != ZNODEEXISTS) {
		cerr << "ElectionSingleMaster RegisterCompletion rc:" << rc << " node:" << node << endl;
	} else {
		election->RegCompletion(rc, reg_data->op_);
	}
	election->Dump();

	delete reg_data;
}