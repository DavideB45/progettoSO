#ifndef TREE_H
#define TREE_H

#pragma once

#include <pthread.h>
// #include <files.h>
#include "files.h"


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


////////////////////////////////// TREE /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


// crea un nuovo albero
// ritorna NULL in caso di errori
TreeFile* newTreeFile();

//destroy


// start mutex da migliorare
void startMutexTreeFile(TreeFile* tree);

// stop mutex da migliorare
void endMutexTreeFile(TreeFile* tree);

// inserisce il nuovo nodo nell'albero
// inserzione riuscita = 1   alreadyExist = 0
// errori Node = 2   nullTree = 3   
// insert non garantice che il puntatore al nodo sia lo stesso
int TreeFileinsert(TreeFile* tree , TreeNode* newNode);

// rimuove un nodo e ritorna il puntatore al file che conteneva
// NULL -> non trovato/bad address
ServerFile* TreeFileRemove(TreeFile* tree, char* name);

// trova e ritorna il file identificato con nome
// ritorna puntatore al file
// NULL = non trovato / bad address
static ServerFile* TreeFileFind(TreeFile* tree, char* name);

// getNElements
// restituisce un puntatore ad un 
char* getNElement(char* message, int dim, const TreeFile* tree, const int N);

//refactor

////////////////////////////////// TREE /////////////////////////////////////////
////////////////////////////////// NODE /////////////////////////////////////////

// crea un nuovo nodo dell'albero
// ritorna NULL in caso di errori
// se sFile non e' nullo e' incluso nella creazione
TreeNode* newTreeNode(ServerFile* sFile, char* name);

//destroy


#endif