#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zookeeper/zookeeper.h>
#include <zookeeper/zookeeper_log.h>
#include <time.h>
#include <iostream>
using namespace std;


void zktest_watcher_g(zhandle_t* zh, int type, int state, const char* path, void* watcherCtx) {
	printf("Something happened.\n");
    printf("type: %d\n", type);
    printf("state: %d\n", state);
    printf("path: %s\n", path);
    printf("watcherCtx: %s\n", (char *)watcherCtx);
}

void zktest_dump_stat(const struct Stat* stat) {
	char tctimes[40];
	char tmtimes[40];
	time_t tctime;
	time_t tmtime;

	if (!stat) {
		fprintf(stderr, "NULL!!!\n");
		return;
	}

	tctime = stat->ctime/1000;
	tmtime = stat->mtime/1000;

	ctime_r(&tmtime, tmtimes);
	ctime_r(&tctime, tctimes);

	fprintf(stderr, "\tctime = %s\tczxid=%llx\n"
    "\tmtime=%s\tmzxid=%llx\n"
    "\tversion=%x\taversion=%x\n"
    "\tephemeralOwner = %llx\n",
     tctimes, stat->czxid,
     tmtimes, stat->mzxid,
    (unsigned int)stat->version, (unsigned int)stat->aversion,
    stat->ephemeralOwner);
}

void zktest_stat_completion(int rc, const struct Stat* stat, const void* data) {
	fprintf(stderr, "%s: rc = %d Stat:\n", (char*)data, rc);
	zktest_dump_stat(stat);
}

void zktest_void_completion(int rc, const void* data) {
	fprintf(stderr, "[%s]: rc = %d\n", (char*)(data==0?"null":data), rc);
}

void zktest_string_completion(int rc, const char* name, const void* data) {
	fprintf(stderr, "[%s]: rc = %d\n", (char*)(data==0?"null":data), rc);
	if (!rc) {
		fprintf(stderr, "\tname = %s\n", name);
	}
}


struct data {
	int a = 1;
	int b = 2;
};

void callback(data* d) {
	cout << d->a << endl;
	cout << d->b << endl;
}


int main() {
	const char* host = "127.0.0.1:40001";
	int timeout = 30000;

	zoo_set_debug_level(ZOO_LOG_LEVEL_WARN);
	char s[100] = "hello zookeeper c api!!!";
	zhandle_t* zkhandle = zookeeper_init(host, zktest_watcher_g, timeout, 0, s, 0);
	if (zkhandle == NULL) {
		fprintf(stderr, "Error when connecting to zookeeper servers...\n");
		exit(EXIT_FAILURE);
	}
	// printf("*****BEFORE");
	// int ret = zoo_acreate(zkhandle, "/xyz", "hello", 5, &ZOO_OPEN_ACL_UNSAFE, 0, zktest_string_completion, "acreate");
	// printf("*****AFTER");
	// if (ret) {
	// 	fprintf(stderr, "Error %d for %s\n", ret, "acreate");
	// 	exit(EXIT_FAILURE);
	// }

	// struct Stat stat;
	// printf("2BEFORE");
	// ret = zoo_exists(zkhandle, "/xy", -1, &stat);
	// printf("2AFTER");

	struct Stat stat;
	data d;
	zoo_wexists(zkhandle, "/xyz", callback, &d, &stat);

	// ret = 0;

	// getchar();

	// ret = zoo_adelete(zkhandle, "/xyz", -1, zktest_void_completion, "adelete");
	// if (ret) {
	// 	fprintf(stderr, "Error %d for %s\n", ret, "adeletes");
	// 	exit(EXIT_FAILURE);
	// }

	// getchar();
	// zookeeper_close(zkhandle);
}

