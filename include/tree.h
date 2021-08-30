#ifndef TREE_H
#define TREE_H

#pragma once

#include <pthread.h>
#include <files.h>


typedef struct TreeNode{
	pthread_mutex_t lock;

	struct TreeNode* leftPtr;
	struct TreeNode* rightPtr;
	struct TreeNode* moreRecentLRU;
	struct TreeNode* lessRecentLRU;

	char* name;
	ServerFile *sFile;
}TreeNode;

typedef struct TreeFile{
	pthread_mutex_t lock;

	struct TreeNode* mostRecentLRU;
	struct TreeNode* leastRecentLRU;

	int maxUsedSpace;
	int maxUsedFile;

	int maxFileNum;
	int maxFileDim;

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
// ritorna una lista di file rimossi per fare spazio
// puo' ritornare NUll anche in caso di successo
// setta errno
// insert non garantice che il puntatore al nodo sia lo stesso
TreeNode* TreeFileinsert(TreeFile* tree , TreeNode** newNode, ServerFile** removed);

// rimuove un nodo e ritorna il puntatore al file che conteneva
// NULL -> non trovato/bad address
// setta errno
ServerFile* TreeFileRemove(TreeFile* tree, TreeNode* node);

// trova e ritorna il file identificato con nome
// ritorna puntatore al file
// NULL = non trovato / bad address
TreeNode* TreeFileFind(TreeFile* tree, char* name);

// getNElements
// restituisce un puntatore ad una stringa 
// che puo' essere usata per rispondere al client
char* getNElement(int* dim, TreeFile* tree, int *N);



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
// ritorna una lista di nodi contenti i file da rimuovere
// modificare con le nuove politiche delle lock
GeneralList* makeSpace(TreeFile* tree, int nFile, int dimSpace);

#endif