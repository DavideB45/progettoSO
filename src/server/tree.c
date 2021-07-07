#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "files.c"
// #include "LRU.c"
#include <pthread.h>

//miltiple reader
//le modifiche dei nodi vanno fatte in mutua escluione

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

//create
// ritorna NULL in caso di errori puntatore altrimenti
TreeFile* newTreeFile(){
	TreeFile* newTree = malloc(sizeof(TreeFile));
	if(newTree == NULL){
		perror("malloc newTree");
		return NULL;
	}
	if(pthread_mutex_init( &(newTree->lock), NULL )){
		pthread_mutex_destroy( &(newTree->lock) );
		free(newTree);
		return NULL;
	}
	if(pthread_cond_init(&(newTree->waitAccess), NULL)){
		pthread_mutex_destroy( &(newTree->lock) );
		pthread_cond_destroy( &(newTree->waitAccess) );
		free(newTree);
		return NULL;
	}
	newTree->fileCount = 0;
	newTree->nodeCount = 0;
	newTree->root = NULL;
	return newTree;
}

//destroy

//start mutex
void startMutexTreeFile(TreeFile* tree){
	Pthread_mutex_lock( &(tree->lock) );
	return;
}

//stop mutex
void endMutexTreeFile(TreeFile* tree){
	Pthread_mutex_unlock( &(tree->lock) );
	return;
}

//non funziona correttamente se chiamata direttamente
// return 5 se ho sostituito
// return 0 se gia' esiste
// return 1 se inserimento classico
static int noMutexInsert(TreeNode* root, TreeNode* newNode){
	if( !(root->flagReal) ){
		//lo metto al posto di un nodo che c'era ma non era reale
		free(root->name);
		//il file deve essere NULL (fatto nella remove) 
		root->name = newNode->name;
		root->flagReal = 1;
		root->sFile = newNode->sFile;
		root->useLRU = newNode->useLRU;
		free(newNode);
		return 5;
	}
	
	int compare = strcmp(root->name, newNode->name);
	if(compare < 0){
		if(root->rightPtr == NULL){
			root->rightPtr = newNode;
			return 1;
		}
		return noMutexInsert(root->rightPtr, newNode);
	}
	if(compare > 0){
		if(root->leftPtr == NULL){
			root->leftPtr = newNode;
			return 1;
		}
		return noMutexInsert(root->leftPtr, newNode);
	}
	return 0;//elemento gia' presente
}

//insert stub non garantice che il puntatore al nodo sia lo stesso
// inserzione riuscita = 1
//errori Node = 2   nullTree = 3   alreadyExist = 0
int TreeFileinsert(TreeFile* tree , TreeNode* newNode){
	if(newNode == NULL){
		errno = EFAULT;
		return 2;
	}
	if(newNode->name == NULL){
		errno = EFAULT;
		return 2;
	}
	if(tree == NULL){
		errno = EFAULT;
		return 3;
	}
	startMutexTreeFile(tree);
		if(tree->root == NULL){
			tree->root = newNode;
			endMutexTreeFile(tree);
			return 1;
		}

		int ret = noMutexInsert(tree->root, newNode);
		if (ret != 4){
			(tree->fileCount)++;
			if(ret == 1){
				(tree->nodeCount)++;
			} else {
				ret = 1;
			}
		}
		
	endMutexTreeFile(tree);
	errno = 0;
	return ret;
}


//non funziona correttamente se chiamata direttamente
static ServerFile* noMutexRemove(TreeNode* root, char* name){
	if(root == NULL){
		return NULL;
	}
	int compare = strcmp(root->name, name);
	if(compare < 0){
		return noMutexRemove(root->rightPtr, name);
	}
	if(compare > 0){
		return noMutexInsert(root->leftPtr, name);
	}

	if(root->flagReal == 0){
		return NULL;
	} else {
		root->flagReal = 0;
		ServerFile* toRet = root->sFile;
///////////////////////////////////////////////////////////////////////////
		// remove(root->useLRU);
///////////////////////////////////////////////////////////////////////////
		root->sFile == NULL;
		return toRet;
	}
}

//remove stub
//NULL -> non trovato/bad address
ServerFile* TreeFileRemove(TreeFile* tree, char* name){
	if(tree == NULL){
		printf("tree remove NULL\n");
		errno  = EFAULT;
		return NULL;
	}
	if(name == NULL){
		printf("name remove NULL\n");
		errno  = EFAULT;
		return NULL;
	}
	startMutexTreeFile(tree);
		ServerFile* toRet = noMutexRemove(tree->root, name);
	endMutexTreeFile(tree);
	errno = 0;
	return toRet;
}

//non funziona correttamente se chiamata direttamente
static ServerFile* noMutexFind(TreeNode* root, char* name){
	if(root == NULL){
		return NULL;
	}
	int compare = strcmp(root->name, name);
	if(compare < 0){
		return noMutexRemove(root->rightPtr, name);
	}
	if(compare > 0){
		return noMutexInsert(root->leftPtr, name);
	}

	if(root->flagReal == 0){
		return NULL;
	} else {
///////////////////////////////////////////////////////////////////////////
		// moveToFront(root->useLRU);
///////////////////////////////////////////////////////////////////////////
		return root->sFile;;
	}
}

//find stub
//NULL = invalid argument/ not found
static ServerFile* TreeFileFind(TreeFile* tree, char* name){
	if(tree == NULL){
		printf("tree Find NULL\n");
		errno  = EFAULT;
		return NULL;
	}
	if(name == NULL){
		printf("name Find NULL\n");
		errno  = EFAULT;
		return NULL;
	}
	startMutexTreeFile(tree);
		ServerFile* toRet = noMutexFind(tree->root, name);
	endMutexTreeFile(tree);
	errno = 0;
	return toRet;
}

//getNElements

//refactor

////////////////////////////////// TREE /////////////////////////////////////////
////////////////////////////////// NODE /////////////////////////////////////////

//create

//destroy
