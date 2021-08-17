#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// #include "../../include/files.h"
// #include "../../include/tree.h"
#include <files.h>
#include <tree.h>
#include <utils.h>
#include <pthread.h>

//miltiple reader
//le modifiche dei nodi vanno fatte in mutua escluione



////////////////////////////////// TREE /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

// create
// ritorna NULL in caso di errori puntatore altrimenti
TreeFile* newTreeFile(){
	TreeFile* newTree = malloc(sizeof(TreeFile));
	if(newTree == NULL){
		perror("malloc newTree");
		return NULL;
	}
	if( Pthread_mutex_init( &(newTree->lock)) != 0){
		free(newTree);
		return NULL;
	}
	
	newTree->maxFileDim = 0;
	newTree->maxFileNum = 0;

	newTree->fileCount = 0;
	newTree->nodeCount = 0;
	newTree->filedim = 0;

	newTree->root = NULL;
	newTree->leastRecentLRU = NULL;
	newTree->mostRecentLRU = NULL;

	return newTree;
}


// chiamato direttamente puo' generare errori
// usato  per distruggere l'albero
static void recursiveRemove(TreeNode* root){
	if(root == NULL){
		return;
	}
	recursiveRemove(root->leftPtr);
	recursiveRemove(root->rightPtr);
	destroyTreeNode(root);
	return;
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
/*retun 0 on success else -1*/
int startMutexTreeFile(TreeFile* tree){
	if(tree == NULL){
		return -1;
	}
	return Pthread_mutex_lock( &(tree->lock) );
}

// stop mutex
/*retun 0 on success else -1*/
int endMutexTreeFile(TreeFile* tree){
	if(tree == NULL){
		return -1;
	}
	return Pthread_mutex_unlock( &(tree->lock) );
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
//errori Node = 2   nullTree = 3   alreadyExist = 0   noLock = 4
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
	if( startMutexTreeFile(tree) != 0){
		return 4;
	}
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
//NULL -> non trovato/bad address/no Lock
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
	if( startMutexTreeFile(tree) != 0){
		errno = EPERM;
		return NULL;
	}

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

//find stub
//NULL = invalid argument/ not found
ServerFile* TreeFileFind(TreeFile* tree, char* name){
	if(tree == NULL){
		errno  = EFAULT;
		return NULL;
	}
	if(name == NULL){
		errno  = EFAULT;
		return NULL;
	}
	if( startMutexTreeFile(tree) != 0){
		errno = EPERM;
		return NULL;
	}
		TreeNode* node = noMutexGetNode(tree->root, name);

		ServerFile* toRet = NULL;
		if(node != NULL){
			if(node->flagReal == 1){
//-/-/-/-/-/-/-/moveToFrontLRU(tree, node);/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-/-
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
		errno = ENOMEM;
		return NULL;
	}
	if(sFile != NULL){
		newNode->flagReal = 1;
		newNode->sFile = sFile;
	} else {
		newNode->sFile = NULL;
		newNode->flagReal = 0;
		// newNode->useLRU = NULL;
	}
	newNode->leftPtr = NULL;
	newNode->rightPtr = NULL;
	int nameL = strlen(name);
	newNode->name = malloc(nameL + 1);
	if(newNode->name == NULL){
		perror("no space for name");
		free(newNode);
		errno = ENOMEM;
		return NULL;
	}
	newNode->name = strcpy(newNode->name, name);
	return newNode;
}

//destroy
void destroyTreeNode(TreeNode* node){
		if(node == NULL){
			return;
		}

		free(node->name);
		destroyServerFile(node->sFile);
		free(node);
		node = NULL;
		return;
}

/////////////////////////////////// LRU /////////////////////////////////////////
///////////////////////////////// REPLACE ///////////////////////////////////////

// deve aggiornare max dim e max n file

// controllare che non si abbiano puntatori a file NULL

// funzioni da chiamare dentro la mutua esclusione

// sposta un file/nodo in testa alla lista LRU
// 0 success -1 err
int moveToFrontLRU(TreeFile* tree, TreeNode* node){
	if(node == NULL || tree == NULL || tree->mostRecentLRU == NULL){
		errno = EINVAL;
		return -1;
	}

	if(node->moreRecentLRU == NULL){
		/* e' gia' la testa */
		errno = 0;
		return 0;
	}
	
	if(node->lessRecentLRU == NULL){
		/* e' il meno rec usato */
		tree->leastRecentLRU = node->moreRecentLRU;// non succede LRU = NULL
	} else {
		node->lessRecentLRU->moreRecentLRU = node->moreRecentLRU;// prec punta succ
	}
	node->moreRecentLRU->lessRecentLRU = node->lessRecentLRU;// succ punta prec
	
	node->lessRecentLRU = tree->mostRecentLRU;// prec = ex MRU
	tree->mostRecentLRU->moreRecentLRU = node;// MRU punta a node come MRU
	tree->mostRecentLRU = node;// diventa MRU
	node->moreRecentLRU = NULL;
	
	errno = 0;
	return 0;
}

// rimuove un nodo dalla LRU
// lo lascia nell'albero
// 0 success -1 err
int removeFromLRU(TreeFile* tree, TreeNode* node){
	if(node == NULL || tree == NULL || node->sFile == NULL){
		errno = EINVAL;
		return -1;
	}
	// aggiorna spazio
	(tree->fileCount)--;
	// nessuno puo' scrivere mentre faccio la remove
	// No perche' FILE_DELETED e' usato solo quando 
	// ho la mutua esclusione sul file
	(tree->filedim) -= node->sFile->dim;
	
	if(tree->mostRecentLRU == node){
		/* era la testa */
		tree->mostRecentLRU = node->lessRecentLRU;
		if(node->lessRecentLRU == NULL){
			/* era l'unico */
			tree->leastRecentLRU = NULL;
		} else {
			/* sistemo nuovo MRU */
			tree->mostRecentLRU->moreRecentLRU = NULL;
		}
		node->lessRecentLRU = NULL;
		node->moreRecentLRU = NULL;
		errno = 0;
		return 0;
	}
	
	if(tree->leastRecentLRU == node){
		/* era l'ultimo (almeno uno davanti) */
		tree->leastRecentLRU = node->moreRecentLRU;
		tree->leastRecentLRU->lessRecentLRU = NULL;
		node->lessRecentLRU = NULL;
		node->moreRecentLRU = NULL;
		errno = 0;
		return 0;
	}
	
	node->lessRecentLRU->moreRecentLRU = node->moreRecentLRU;
	node->moreRecentLRU->lessRecentLRU = node->lessRecentLRU;
	node->lessRecentLRU = NULL;
	node->moreRecentLRU = NULL;
	errno = 0;
	return 0;
}

// inserisce un nodo nella lista LRU
// 0 success -1 err
int insertToFrontLRU(TreeFile* tree, TreeNode* node){
	if(tree == NULL || node == NULL || node->sFile == NULL){
		errno = EINVAL;
		return -1;
	}
	(tree->fileCount)++;
	// nessuno puo' scrivere mentre faccio la remove
	// perche' da quando il file e' nell'albero
	// non ho lasciato la lock
	(tree->filedim) += node->sFile->dim;// la dim sara' 0
	
	node->moreRecentLRU = NULL;
	if(tree->leastRecentLRU == NULL){
		/* non ci sono file */
		tree->leastRecentLRU = node;
		tree->mostRecentLRU = node;
		tree->leastRecentLRU = NULL;
		errno = 0;
		return 0;
	}

	tree->mostRecentLRU->moreRecentLRU = node;
	node->lessRecentLRU = tree->mostRecentLRU;
	tree->mostRecentLRU->moreRecentLRU = node;
	errno = 0;
	return 0;
	
}

void fakeFree(void* ptr){
	return;
}

// cerca di liberare dimSpace byte di memoria e nFile
// ritorna 0 successo -1 se non e' stato possibile rimuovere
// modificare con le nuove politiche delle lock
int makeSpace(TreeFile* tree, int nFile, int dimSpace){
	if(tree == NULL || nFile < 0 || dimSpace < 0){
		errno = EINVAL;
		return -1;
	}
	int nVictim = 0;
	int spaceVictim = 0;
	TreeNode* currVictim = tree->leastRecentLRU;
	ServerFile* tempPtr = NULL;
	GeneralList* listVictim = newGeneralList(fakeComp, fakeFree);
	if(listVictim == NULL){
		// errno settato
		return -1;
	}
	while( currVictim != NULL && (nVictim < nFile || spaceVictim < dimSpace) ){
		if(currVictim->sFile != NULL){// dovrebbe essere sempre vera
			if( Pthread_mutex_lock( &(currVictim->sFile->lock) ) == 0){
				if(currVictim->sFile->flagUse == 0){
					currVictim->sFile->flagUse == 1;
					generalListInsert( (void*) currVictim->sFile, listVictim);
					if(errno != 0){
						// devo sistemare i file 1 alla volta
						Pthread_mutex_lock( &(currVictim->sFile->lock) );

					}
					nVictim++;
					spaceVictim += currVictim->sFile->dim;
				}
				Pthread_mutex_unlock( &(currVictim->sFile->lock) );
			}
		}
	}
	
	
}