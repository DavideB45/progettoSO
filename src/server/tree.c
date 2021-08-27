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
	
	newTree->maxUsedFile = 0;
	newTree->maxUsedSpace = 0;

	newTree->maxFileDim = 0;
	newTree->maxFileNum = 0;

	newTree->fileCount = 0;
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
// return 0 se gia' esiste
// return 1 se inserimento
static int noMutexInsert(TreeNode* root, TreeNode** newNode){
	
	int compare = strcmp(root->name, (*newNode)->name);
	if(compare < 0){
		if(root->rightPtr == NULL){
			root->rightPtr = *newNode;
			errno = 0;
			return 1;
		}
		return noMutexInsert(root->rightPtr, newNode);
	}
	if(compare > 0){
		if(root->leftPtr == NULL){
			root->leftPtr = *newNode;
			errno = 0;
			return 1;
		}
		return noMutexInsert(root->leftPtr, newNode);
	}
	if(Pthread_mutex_lock( &(root->lock) ) != 0){
		errno = EPERM;
		return 0;
	}
	if(root->sFile == NULL){
		root->sFile = (*newNode)->sFile;
		(*newNode)->sFile = NULL;
		destroyTreeNode(*newNode);
		*newNode = root;
		Pthread_mutex_unlock( &(root->lock) );
		errno = 0;
		return 1;
	} else {
		Pthread_mutex_unlock( &(root->lock) );
		errno = 0;
		return 0;//elemento gia' presente
	}
}

// inserisce il nuovo nodo nell'albero
// ritorna una lista di file rimossi per fare spazio
// puo' ritornare NUll anche in caso di successo
// setta errno
// insert non garantice che il puntatore al nodo sia lo stesso
TreeNode* TreeFileinsert(TreeFile* tree , TreeNode** newNode, ServerFile** removed){
	if(newNode == NULL || *newNode == NULL || (*newNode)->name == NULL \
		|| (*newNode)->sFile == NULL || tree == NULL){
		errno = EFAULT;
		return NULL;
	}
	if( startMutexTreeFile(tree) != 0){
		errno = EPERM;
		return NULL;
	}

	TreeNode* toRet = NULL;
	GeneralList* toRemove = NULL;
	if(tree->fileCount == tree->maxFileNum){
		toRemove = makeSpace(tree, 1, 0);
		printf("to remove %p\n", (void*) toRemove);
		if(toRemove == NULL){
			endMutexTreeFile(tree);
			errno = ENOMEM;
			return NULL;
		}
		toRet = generalListPop(toRemove);
		generalListDestroy(toRemove);
		if(Pthread_mutex_lock( &(toRet->lock) ) != 0){
			errno = EPERM;
			return NULL;
		}
		*removed = toRet->sFile;
		removeFromLRU(tree, toRet);
		toRet->sFile = NULL;
		Pthread_mutex_unlock( &(toRet->lock) );
	}
	
	
	if(tree->root == NULL){
		tree->root = *newNode;
		insertToFrontLRU(tree, *newNode);
		if(tree->maxUsedFile == 0){
			tree->maxUsedFile = 1;
		}
		
		//per LRU e' sia primo che ultimo
		endMutexTreeFile(tree);
		errno = 0;
		return toRet;
	}

	if(noMutexInsert(tree->root, newNode) == 1){
		insertToFrontLRU(tree, *newNode);
		if(tree->maxUsedFile < tree->fileCount){
			tree->maxUsedFile = tree->fileCount;
		}
		endMutexTreeFile(tree);
		errno = 0;
		return toRet;
	} else {
		if(errno == 0){
			if(Pthread_mutex_lock( &(toRet->lock) ) != 0){
				errno = EPERM;
				return toRet;
			}
			toRet->sFile = *removed;
			*removed = NULL;
			insertToFrontLRU(tree, toRet);
			Pthread_mutex_unlock( &(toRet->lock) );
			errno = EEXIST;
		} else {
			if(Pthread_mutex_lock( &(toRet->lock) ) != 0){
				errno = EPERM;
				return toRet;
			}
			toRet->sFile = *removed;
			*removed = NULL;
			insertToFrontLRU(tree, toRet);
			Pthread_mutex_unlock( &(toRet->lock) );
			errno = EPERM;
		}
		return NULL;	
	}
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
ServerFile* TreeFileRemove(TreeFile* tree, TreeNode* node){
	if(tree == NULL || node == NULL){
		printf("tree remove NULL\n");
		errno  = EFAULT;
		return NULL;
	}

	if( startMutexTreeFile(tree) != 0){
		errno = EPERM;
		return NULL;
	}
	if( Pthread_mutex_lock( &(node->lock) ) != 0){
		endMutexTreeFile(tree);
		errno = EPERM;
		return NULL;
	}
	
	ServerFile* toRet = node->sFile;
	removeFromLRU(tree, node);
	node->sFile = NULL;

	Pthread_mutex_unlock( &(node->lock) );
	endMutexTreeFile(tree);
	errno = 0;
	return toRet;
}

//find stub
//NULL = invalid argument/ not found
TreeNode* TreeFileFind(TreeFile* tree, char* name){
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
	
	if(node == NULL){
		endMutexTreeFile(tree);
		errno = 0;
		return NULL;
	}
	printf("prendo la lock\n");
	if(Pthread_mutex_lock( &(node->lock) ) != 0){
		endMutexTreeFile(tree);
		errno = EPERM;
		return NULL;
	}
	printf("ho la lock\n");
	fflush(stdout);
	if(node->sFile == NULL){
		Pthread_mutex_unlock( &(node->lock) );
		endMutexTreeFile(tree);
		errno = 0;
		return NULL;
	}

	Pthread_mutex_unlock( &(node->lock) );
	endMutexTreeFile(tree);
	
	errno = 0;
	return node;
}


char* getNElement(int* dim, TreeFile* tree, int* N){
	if(Pthread_mutex_lock( &(tree->lock) ) != 0){
		// errno settato
		return NULL;
	}
	*dim = 0;
	int numFile = 0;
	char* allFile = calloc(1, sizeof(int));
	if(allFile == NULL){
		return NULL;
	}
	
	int allFileDim = sizeof(int);
	char* newPtr = NULL;
	int buffInt;
	TreeNode* currPtr = tree->leastRecentLRU;
	while(*N != 0 && currPtr != NULL){
		if(Pthread_mutex_lock( &(currPtr->lock) ) == 0){
			if(currPtr->sFile != NULL && currPtr->sFile->flagUse == 0){
				buffInt = strlen(currPtr->name) + 1;
				newPtr = realloc(allFile, allFileDim + 2*sizeof(int) + currPtr->sFile->dim +buffInt);
				if(newPtr == NULL){
					*dim = allFileDim;
					*N = numFile;
					Pthread_mutex_unlock( &(currPtr->lock) );
					Pthread_mutex_unlock( &(tree->lock) );
					return allFile;
				} else {
					(*N)--;
					allFile = newPtr;
					numFile++;
					memcpy(allFile + allFileDim, &buffInt, sizeof(int));
					allFileDim += sizeof(int);
					memcpy(allFile + allFileDim, currPtr->name, buffInt);
					allFileDim += buffInt;
					memcpy(allFile + allFileDim, &(currPtr->sFile->dim), sizeof(int));
					allFileDim += sizeof(int);
					memcpy(allFile + allFileDim, currPtr->sFile->data, currPtr->sFile->dim);
					allFileDim += currPtr->sFile->dim;
				}
				Pthread_mutex_unlock( &(currPtr->lock) );
			}
		}
		currPtr = currPtr->moreRecentLRU;
	}
	*dim = allFileDim;
	*N = numFile;
	Pthread_mutex_unlock( &(tree->lock) );
	return allFile;
}


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
	if( Pthread_mutex_init( &(newNode->lock)) != 0){
		free(newNode);
		return NULL;
	}
	newNode->sFile = sFile;
	
	newNode->leftPtr = NULL;
	newNode->rightPtr = NULL;
	newNode->lessRecentLRU = NULL;
	newNode->moreRecentLRU = NULL;

	int nameL = strlen(name);
	newNode->name = malloc(nameL + 1);
	if(newNode->name == NULL){
		perror("no space for name");
		pthread_mutex_destroy( &(newNode->lock) );
		free(newNode);
		errno = ENOMEM;
		return NULL;
	}
	newNode->name = strncpy(newNode->name, name, nameL + 1);
	return newNode;
}

//destroy
void destroyTreeNode(TreeNode* node){
		if(node == NULL){
			return;
		}

		pthread_mutex_destroy( &(node->lock) );
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
// da chiamare con mutua esclusione (su albero e file)
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
// chiamre con mutua esclusione (su albero e file)
int removeFromLRU(TreeFile* tree, TreeNode* node){
	if(node == NULL || tree == NULL || node->sFile == NULL){
		errno = EINVAL;
		return -1;
	}
	// aggiorna spazio
	(tree->fileCount) -= 1;
	// nessuno puo' scrivere mentre faccio la remove
	// No perche' FILE_DELETED e' usato solo quando 
	// ho la mutua esclusione sul file
	(tree->filedim) -= node->sFile->dim;
	printf("non sei ancora crashato 1\n");
	fflush(stdout);
	if(tree->mostRecentLRU == node){
		/* era la testa */
		tree->mostRecentLRU = node->lessRecentLRU;
		printf("non sei ancora crashato 2\n");
		fflush(stdout);
		if(node->lessRecentLRU == NULL){
			/* era l'unico */
			tree->leastRecentLRU = NULL;
		} else {
			/* sistemo nuovo MRU */
			printf("non sei ancora crashato 3\n");
			fflush(stdout);
			tree->mostRecentLRU->moreRecentLRU = NULL;
		}
		printf("non sei ancora crashato 4\n");
		fflush(stdout);
		node->lessRecentLRU = NULL;
		node->moreRecentLRU = NULL;
		errno = 0;
		return 0;
	}
	
	printf("non sei ancora crashato 5\n");
	fflush(stdout);
	if(tree->leastRecentLRU == node){
		/* era l'ultimo (almeno uno davanti) */
		tree->leastRecentLRU = node->moreRecentLRU;
		printf("non sei ancora crashato 6\n");
		fflush(stdout);
		tree->leastRecentLRU->lessRecentLRU = NULL;
		node->lessRecentLRU = NULL;
		node->moreRecentLRU = NULL;
		errno = 0;
		return 0;
	}
	printf("non sei ancora crashato 7\n");
	fflush(stdout);
	if(node->lessRecentLRU != NULL){
		node->lessRecentLRU->moreRecentLRU = node->moreRecentLRU;
	}
	if(node->moreRecentLRU != NULL){
		node->moreRecentLRU->lessRecentLRU = node->lessRecentLRU;
	}
	
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
		node->lessRecentLRU = NULL;
		errno = 0;
		return 0;
	}

	node->lessRecentLRU = tree->mostRecentLRU;
	tree->mostRecentLRU->moreRecentLRU = node;
	tree->mostRecentLRU = node;
	errno = 0;
	return 0;
	
}

void fakeFree(void* ptr){
	if(Pthread_mutex_lock( &(( (TreeNode*) ptr)->lock) ) == 0){
		( (TreeNode*) ptr)->sFile->flagUse = 0;
		Pthread_mutex_unlock( &(( (TreeNode*) ptr)->lock) );
	}
	return;
}

// cerca di liberare dimSpace byte di memoria e nFile
// ritorna una lista di nodi contenti i file da rimuovere
// modificare con le nuove politiche delle lock
GeneralList* makeSpace(TreeFile* tree, int nFile, int dimSpace){
	if(tree == NULL || nFile < 0 || dimSpace < 0){
		errno = EINVAL;
		return NULL;
	}
	int nVictim = 0;
	int spaceVictim = 0;
	int err = 0;;
	TreeNode* currVictim = tree->leastRecentLRU;
	GeneralList* listVictim = newGeneralList(fakeComp, fakeFree);
	if(listVictim == NULL){
		// errno settato
		return NULL;
	}
	while( currVictim != NULL && (nVictim < nFile || spaceVictim < dimSpace) ){
		printf("esamino %p, dim %d \n", (void*) currVictim, currVictim->sFile->dim);
		if( Pthread_mutex_lock( &(currVictim->lock) ) == 0){
			if(currVictim->sFile != NULL && currVictim->sFile->flagUse == 0){
				currVictim->sFile->flagUse = 1;
				generalListInsert( (void*) currVictim, listVictim);
				if(errno != 0){
					err = errno;
					generalListDestroy(listVictim);
					errno = err;
					return NULL;
				}
				nVictim++;
				spaceVictim += currVictim->sFile->dim;
			}
			Pthread_mutex_unlock( &(currVictim->lock) );
		}
		currVictim = currVictim->moreRecentLRU;
	}
	if(nVictim < nFile || spaceVictim < dimSpace){
		generalListDestroy(listVictim);
		errno = 0;
		return NULL;
	}
	
	// se qualcuno ricercasse uno di questi elem lo risposterebbe in cima alla LRU
	// ..v rimuovo dopo da LRU
	// ..x sposto moveToFrontLRU
	// ..x metto un bit flagReal
	errno = 0;
	return listVictim;
}