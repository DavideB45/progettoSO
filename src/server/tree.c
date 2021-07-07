#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../../include/files.h"
#include "../../include/tree.h"
// #include <files.h>
// #include <tree.h>
#include <pthread.h>

//miltiple reader
//le modifiche dei nodi vanno fatte in mutua escluione





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
	//sbagliato
	//correggi qui (sono tanti if)
	//usa 2 flag tip dxOk sxOk
	if( !(root->flagReal) && strcmp(root->name, newNode->name) && strcmp(root->name, newNode->name) ){
		//lo metto al posto di un nodo che c'era ma non era reale
		free(root->name);
		root->name = newNode->name;
		root->flagReal = 1;
		// destroyServerFile(root->sFile);
		root->sFile = newNode->sFile;
		// root->useLRU = newNode->useLRU;
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
		// root->useLRU = NULL;
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
		if(toRet != NULL){
			(tree->fileCount)--;
		}
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
		errno  = EFAULT;
		return NULL;
	}
	if(name == NULL){
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
//restituisce un puntatore ad un 
char* getNElement(char* message, int dim, const TreeFile* tree, const int N);

//refactor

////////////////////////////////// TREE /////////////////////////////////////////
////////////////////////////////// NODE /////////////////////////////////////////

//create
TreeNode* newTreeNode(ServerFile* sFile, char* name){
	if(name == NULL){
		errno = EFAULT;
		return NULL;
	}
	TreeNode* newNode = malloc(sizeof(TreeNode));
	if(newNode == NULL){
		perror("new tree node");
		return NULL;
	}
	if(sFile != NULL){
		newNode->flagReal = 0;
		newNode->sFile = sFile;
	} else {
		newNode->sFile = NULL;
		// newNode->useLRU = NULL;
	}
	newNode->leftPtr = NULL;
	newNode->rightPtr = NULL;
	int nameL = strlen(name);
	newNode->name = malloc(nameL + 1);
	if(newNode->name == NULL){
		perror("no space for name");
		free(newNode);
		return NULL;
	}
	newNode->name = strcpy(newNode->name, name);
	return newNode;
}
//destroy
