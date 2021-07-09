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

// destroy
// libera la memoria occupata da node
// libera eventuale memoria del contenuto
void destroyTreeNode(TreeNode* node);


/////////////////////////////////// LRU /////////////////////////////////////////
///////////////////////////////// REPLACE ///////////////////////////////////////


int moveToFrontLRU(TreeFile* tree, TreeNode* node);

int removeFromLRU(TreeFile* tree, TreeNode* node);

int insertToFrontLRU(TreeFile* tree, TreeNode* node);

int makeSpace();

#endif