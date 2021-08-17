#ifndef TREE_H
#define TREE_H

#pragma once

#include <pthread.h>
// #include <files.h>
#include "files.h"



typedef struct TreeNode{
	struct TreeNode* leftPtr;
	struct TreeNode* rightPtr;
	struct TreeNode* moreRecentLRU;
	struct TreeNode* lessRecentLRU;

	_Bool flagReal;// 0 => tutto NULL tranne elementi neccessari per funzionamento albero
	char* name;
	ServerFile *sFile;
}TreeNode;

typedef struct TreeFile{
	pthread_mutex_t lock;

	struct TreeNode* mostRecentLRU;
	struct TreeNode* leastRecentLRU;

	int maxFileNum;
	int maxFileDim;

	int nodeCount;
	int fileCount;
	int filedim;
	TreeNode* root;
}TreeFile;


////////////////////////////////// TREE /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////


// crea un nuovo albero
// ritorna NULL in caso di errori
TreeFile* newTreeFile();

// destroy
// distrugge l'albero passato come argomento
void destroyTreeFile(TreeFile* tree);

// start mutex 
// retun 0 on success else -1
int startMutexTreeFile(TreeFile* tree);

// stop mutex
// retun 0 on success else -1
int endMutexTreeFile(TreeFile* tree);

// inserisce il nuovo nodo nell'albero
// inserzione riuscita = 1   alreadyExist = 0
// errori Node = 2   nullTree = 3   noLock = 4
// insert non garantice che il puntatore al nodo sia lo stesso
int TreeFileinsert(TreeFile* tree , TreeNode* newNode);

// rimuove un nodo e ritorna il puntatore al file che conteneva
// NULL -> non trovato/bad address
// setta errno
ServerFile* TreeFileRemove(TreeFile* tree, char* name);

// trova e ritorna il file identificato con nome
// ritorna puntatore al file
// NULL = non trovato / bad address
ServerFile* TreeFileFind(TreeFile* tree, char* name);

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

// destroy
// libera la memoria occupata da node
// libera eventuale memoria del contenuto
void destroyTreeNode(TreeNode* node);


/////////////////////////////////// LRU /////////////////////////////////////////
///////////////////////////////// REPLACE ///////////////////////////////////////


/* 
	idee per estire la mutua esclusione
	..x passare un _Bool per dire se deve eseguire la lock
	..v creare funzioni particolari noMutex*LRU(TreeFile* tree, TreeNode* node)
	..x vedere se lock ritorna errori utili
*/

// sposta un file/nodo in testa alla lista LRU
// 0 success -1 err
int moveToFrontLRU(TreeFile* tree, TreeNode* node);

// rimuove un nodo dalla LRU
// lo lascia nell'albero
// 0 success -1 err
int removeFromLRU(TreeFile* tree, TreeNode* node);

// inserisce un nodo nella lista LRU
// 0 success -1 err
int insertToFrontLRU(TreeFile* tree, TreeNode* node);

// cerca di liberare dimSpace byte di memoria e nFile
// ritorna 0 successo -1 se non e' stato possibile rimuovere
int makeSpace(TreeFile* tree, int nFile, int dimSpace);

#endif