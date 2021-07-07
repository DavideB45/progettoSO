#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include "files.h"

//c alberto

typedef int _Bool;

typedef struct nodeLRU{
	struct nodeLRU* moreRecent;
	struct nodeLRU* lessRecent;
	TreeNode* locate;

}nodeLRU;

typedef struct LRU{
	nodeLRU* mostRecent;
	nodeLRU* leastRecent;
}listLRU;

typedef struct TreeNode{
	struct TreeNode* leftPtr;
	struct TreeNode* rightPtr;
	_Bool flagReal;// 0 => tutto NULL tranne elementi neccessari per funzionamento albero
	char* name;
	nodeLRU* useLRU;
	ServerFile *sFile;
}TreeNode;

typedef struct TreeFile{
	pthread_mutex_t lock;
	pthread_cond_t waitAccess;// non sono sicuro che serva
	int nodeCount;
	int fileCount;
	TreeNode* root;
}TreeFile;