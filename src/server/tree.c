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
	
	newTree->fileCount = 0;
	newTree->nodeCount = 0;
	newTree->filedim = 0;

	newTree->root = NULL;
	newTree->leastRecentLRU = NULL;
	newTree->mostRecentLRU = NULL;

	return newTree;
}


//chiamato direttamente puo' generare errori
static void recursiveRemove(TreeNode* root){
	if(root == NULL){
		return;
	}
	recursiveRemove(root->leftPtr);
	recursiveRemove(root->rightPtr);
	free(root->name);
	destroyServerFile(root->sFile);
	free(root);
}

//destroy stub
void destroyTreeFile(TreeFile* tree){
	if(tree == NULL){
		return;
	}
	startMutexTreeFile(tree);
		recursiveRemove(tree->root);
	endMutexTreeFile(tree);

	pthread_mutex_destroy( &(tree->lock) );
	free(tree);
	tree = NULL;
	return;
}

//start mutex
void startMutexTreeFile(TreeFile* tree){
	Pthread_mutex_lock( &(tree->lock) );
	return;
}

// stop mutex
void endMutexTreeFile(TreeFile* tree){
	Pthread_mutex_unlock( &(tree->lock) );
	return;
}

// non funziona correttamente se chiamata direttamente
// return 5 se ho sostituito
// return 0 se gia' esiste
// return 1 se inserimento classico
static int noMutexInsert(TreeNode* root, TreeNode* newNode){
	
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
	if(root->flagReal == 0){
		destroyServerFile(root->sFile);///////////////////potrebbe causare problemi?//////////
		root->sFile = newNode->sFile;
		root->flagReal = 1;
		free(newNode->name);
		free(newNode);
		newNode = root;
		return 5;
	} else {
		return 0;//elemento gia' presente
	}
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
			if(newNode->sFile != NULL){
				tree->filedim = newNode->sFile->dim;
				(tree->fileCount)++;
///////////insertToFrontLRU(tree, newNode)//////////////////////////////////////////////////////
			}
			(tree->nodeCount)++;
			//per LRU e' sia primo che ultimo
			endMutexTreeFile(tree);
			return 1;
		}

		int ret = noMutexInsert(tree->root, newNode);
		if (ret != 4){
			if(newNode->sFile != NULL){
				(tree->fileCount)++;
				(tree->filedim) += newNode->sFile->dim;
///////////insertToFrontLRU(tree, newNode)////////////////////////////////////////////////////////////
			}
			
			if(ret == 1){
				(tree->nodeCount)++;
			} else {
				ret = 1;
			}
		} else {
			ret = 0;
		}
		
	endMutexTreeFile(tree);
	errno = 0;
	return ret;
}


// non funziona correttamente se chiamata direttamente
// ritorna NULL se non trovato il puntatore al nodo altrimenti
static TreeNode* noMutexGetNode(TreeNode* root, char* name){
	if(root == NULL){
		return NULL;
	}
	int compare = strcmp(root->name, name);
	if(compare < 0){
		return noMutexGetNode(root->rightPtr, name);
	}
	if(compare > 0){
		return noMutexGetNode(root->leftPtr, name);
	}
	return root;
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

		TreeNode* node = noMutexGetNode(tree->root, name);
		ServerFile* toRet = node->sFile;
		if(toRet != NULL){
			(tree->fileCount)--;
			(tree->filedim) -= toRet->dim;
/////////removeFromLRU(tree, node)/////////////////////////////////////////////////////////////////////
		}
		node->flagReal = 0;
		node->sFile = NULL;
	endMutexTreeFile(tree);
	errno = 0;
	return toRet;
}

//non funziona correttamente se chiamata direttamente
// static ServerFile* noMutexFind(TreeNode* root, char* name){
// 	if(root == NULL){
// 		return NULL;
// 	}
// 	int compare = strcmp(root->name, name);
// 	if(compare < 0){
// 		return noMutexFind(root->rightPtr, name);
// 	}
// 	if(compare > 0){
// 		return noMutexFind(root->leftPtr, name);
// 	}
// 	if(root->flagReal == 0){
// 		return NULL;
// 	} else {
// 		return root->sFile;;
// 	}
// }

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
		TreeNode* node = noMutexGetNode(tree->root, name);

		ServerFile* toRet = NULL;
		if(node != NULL){
			if(node->flagReal == 1){
//-/-/-/-/-/-/-/moveToFront(tree, node);/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-
				toRet = node->sFile;
			}
		}

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
void destroyTreeNode(TreeFile tree ,TreeNode node){
		
}

/////////////////////////////////// LRU /////////////////////////////////////////
///////////////////////////////// REPLACE ///////////////////////////////////////