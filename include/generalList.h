typedef struct GeneralListNode{
	void* elem;
	struct GeneralListNode *nextPtr;
}GeneralListNode;

typedef struct GeneralList{
	GeneralListNode *head;
	GeneralListNode *queue;
	int (*compFun)(void*, void*);
	void (*freeFun)(void*);
}GeneralList;
