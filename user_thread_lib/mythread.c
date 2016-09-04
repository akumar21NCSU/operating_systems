#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>

typedef ucontext_t* MyThread;

typedef struct Node{
	ucontext_t* context;	
	struct Node* next;	
	struct Node* parent;
	int activeChildren;
	int isBlockedonJoin;
	int childBlockedonSemaphore;
	int isBlockingParent;
	
	int isZombie;
}Node;

typedef struct Queue{
	Node* front;
	Node* rear;
}Queue;

static MyThread tmain,destroyer_thread;
static Queue* readyQ;
static Queue* blockedQ;
static Queue* zombieQ;

typedef struct semaphore{
	int value;
	Queue* semaQueue;
}semaphore;

typedef semaphore* MySemaphore;

void enQueue(Queue* q, Node* n){
	n->next=NULL;
	if (q->front == NULL){
		q->front = n;
		q->rear = n;
		return;
	}
	q->rear->next = n;
	q->rear = n;
}

Node* deQueue(Queue* q){
	if (q->front == NULL){
		return NULL;	
	}
	Node* head = q->front;
	
	if(q->front == q->rear){
		q->front=NULL;
		q->rear =NULL;
		head->next = NULL;
		return head;
	}
	
	q->front = (q->front)->next;
	head->next=NULL;
	return head;
}


Node* peek(Queue* q){
	Node* temp = NULL;
	if(q!= NULL)		
		return q->front;
	else{
		printf("Error\n");
		return temp;

	}
}


void addChild(Node* newChild){
	
	Node* running = peek(readyQ);
	newChild->parent = running;
	running->activeChildren = running->activeChildren+1;
}

void schedule(){
	//printf("Enter shcedule()\n");
	if(readyQ->front != NULL){
		
		Node* head = peek(readyQ);	
		if(setcontext(head->context) == -1){
			printf("error in swapping - main\n");
		}
	}else{
		//Todo: clear zombie and blocked queues.		
		setcontext(tmain);
	}
}

MySemaphore MySemaphoreInit(int initialValue){

	MySemaphore mysem = NULL;
	if (initialValue <= 0)
		return mysem;

	Queue* squeue = (Queue*)malloc(sizeof(Queue));
	squeue->front=NULL;
	squeue->rear=NULL;

	mysem = (MySemaphore)malloc(sizeof(struct semaphore));
	mysem->value = initialValue;
	mysem->semaQueue = squeue;
	
	return mysem;
}

int MySemaphoreDestroy(MySemaphore sem){
	
	if(sem == NULL)
		return -1;

	if(sem->semaQueue->front != NULL)
		return -1;

	free(sem->semaQueue);
	free(sem);

	return 0;

}

void MySemaphoreSignal(MySemaphore sem){
	
	if(sem == NULL)
		return;
	sem->value = sem->value +1;	
	if(sem->semaQueue->front != NULL){
		Node* temp = deQueue(sem->semaQueue);
		(temp->parent)->childBlockedonSemaphore = (temp->parent)->childBlockedonSemaphore -1;
		enQueue(readyQ,temp);
	}
}

void MySemaphoreWait(MySemaphore sem){

	if(sem == NULL)
		return;	
	sem->value = sem->value -1;	
	//printf("sem's value = %d\n",sem->value+1);
	if(sem->value+1 <= 0){
		Node* temp = deQueue(readyQ);
		(temp->parent)->childBlockedonSemaphore = (temp->parent)->childBlockedonSemaphore +1;
		enQueue(sem->semaQueue,temp);
		schedule();
	}
}

void MyThreadYield(){
	if (readyQ->front == NULL || readyQ->front == readyQ->rear)
		return;
	else{
		MyThread next_ct;
		Node* next_node;
		Node* running_node;

		running_node = deQueue(readyQ);
		next_node = peek(readyQ);

		next_ct = next_node->context;			
		//running_node->context = current_ct;
	 	running_node->next = NULL;
		enQueue(readyQ, running_node);

		if(getcontext(running_node->context) == -1){
			printf("Error in context get inside yield");
			return;
		}
		swapcontext(running_node->context,next_ct);
	}
}

void removeNode(Queue* q, Node* n){
	if(q->front == NULL)
		return ;

	Node* cur = peek(q);
	if (cur == n){
		deQueue(q);	
		return ;		
	}
	Node* prev = cur;
	cur = cur->next;
	while(cur != NULL){
		if(cur == n){
			prev->next = cur->next;
			return ;
		}
		prev=cur;
		cur=cur->next;
	}
	return ;
}

void unblockNode(Node* parent){
	parent->isBlockedonJoin = parent->isBlockedonJoin -1;
	if(parent->isBlockedonJoin ==0 && parent->childBlockedonSemaphore == 0){
		removeNode(blockedQ,parent);
		enQueue(readyQ,parent);
	}
}


void MyThreadExit(){
	
	//printf("Entring exit\n");
	Node* deleted_node = deQueue(readyQ);		
	Node* parent = deleted_node->parent;

	if(parent != NULL){		
		parent->activeChildren = parent->activeChildren -1;			
		// unblock from blockedQ or delete from zombie queue, if present
		if(parent->isZombie && parent->activeChildren == 0){
			removeNode(zombieQ,parent);
			free(parent);
		}
		if (deleted_node->isBlockingParent == 1)
			unblockNode(parent);		
	}

	if(deleted_node->activeChildren == 0){		
		free(deleted_node);		
	}else{
		deleted_node->isZombie=1;
		enQueue(zombieQ,deleted_node);
	}
	schedule();
}

void destroy(){
	MyThreadExit();
}

MyThread MyThreadCreate(void(*start_funct)(), void *args){

	//printf("Entring mythreadcreate\n");

	MyThread new_thread = (MyThread)malloc(sizeof(ucontext_t));	
	char* new_stack = (char*)malloc(sizeof(char)*16384);	
		
	if(getcontext(new_thread) == -1){
		printf("Error creating new thread");
		return new_thread;
	}

	new_thread->uc_stack.ss_sp = new_stack;
	new_thread->uc_stack.ss_size = sizeof(char)*16384;
	new_thread->uc_link = destroyer_thread;

	makecontext(new_thread, start_funct,1,args);
	Node* new_node = (Node*)malloc(sizeof(Node));
	new_node->context= new_thread;
	new_node->next= NULL;
	new_node->activeChildren =0;
	new_node->isBlockedonJoin=0;
	new_node->isBlockingParent=0;
	new_node->childBlockedonSemaphore=0;
	new_node->isZombie=0;	
	
	addChild(new_node);
	enQueue(readyQ, new_node);
	
	//printf("Exiting mythread create\n");
	return new_thread;
}

void MyThreadInit (void(*start_funct)(void *), void *args){
	
	destroyer_thread = (MyThread)malloc(sizeof(ucontext_t));
	if(getcontext(destroyer_thread) == -1){
		printf("Error getting root context");
		return;
	}

	char* destroyer_stack = (char*)malloc(sizeof(char)*16384);
	destroyer_thread->uc_stack.ss_sp = destroyer_stack;
	destroyer_thread->uc_stack.ss_size = sizeof(char)*16384;
	destroyer_thread->uc_link = NULL;

	makecontext(destroyer_thread,destroy,0);	

	//printf("Entering mythreadinit\n");
	tmain = (MyThread)malloc(sizeof(ucontext_t));
	if(getcontext(tmain) == -1){
		printf("Error getting root context");
		return;
	}
	if(readyQ != NULL){		
		return;
	}

	readyQ = (Queue *)malloc(sizeof(Queue));
	readyQ->front = NULL;
	readyQ->rear = NULL;
	
	blockedQ = (Queue *)malloc(sizeof(Queue));
	blockedQ->front = NULL;
	blockedQ->rear = NULL;

	zombieQ = (Queue *)malloc(sizeof(Queue));
	zombieQ->front = NULL;
	zombieQ->rear = NULL;
	
	MyThread root_thread = (MyThread)malloc(sizeof(ucontext_t));

	if(getcontext(root_thread) == -1){
		printf("Error getting root context");
		return;
	}
	char* root_stack = (char*)malloc(sizeof(char)*16384);
	root_thread->uc_stack.ss_sp = root_stack;
	root_thread->uc_stack.ss_size = sizeof(char)*16384;
	root_thread->uc_link = destroyer_thread;	

	makecontext(root_thread,start_funct,1,args);	

	Node* root_node = (Node*)malloc(sizeof(Node));	
	root_node->context = root_thread;
	root_node->next= NULL;	
	root_node->activeChildren =0;
	root_node->isBlockedonJoin=0;
	root_node->isBlockingParent=0;
	root_node->childBlockedonSemaphore=0;
	root_node->isZombie=0;	
	enQueue(readyQ,root_node);
	setcontext(root_thread);
	
}

Node* getNode(MyThread t){

	Node* cur= readyQ->front;
	while(cur != NULL){
		if(cur->context == t)
			return cur;
		cur=cur->next;
	}
	return NULL;
}
void MyThreadJoinAll(){
	Node* running_node = peek(readyQ);	
	if(running_node->activeChildren == 0)
		return;

	int count=0;
	Node* cur = running_node->next;
	while(cur!= NULL){

		if(cur->parent == running_node){
			count++;
			cur->isBlockingParent=1;			
		}
		cur = cur->next;
	}	
	
	if(count>0){
		running_node->isBlockedonJoin=count;				
		enQueue(blockedQ,deQueue(readyQ));
		swapcontext(running_node->context,peek(readyQ)->context);
	}
}

int MyThreadJoin(MyThread thread){
	//printf("Entring join\n");
	Node* running_node = peek(readyQ);	
	Node* thread_node = getNode(thread);

	if(thread_node != NULL){
		if(thread_node->parent != running_node)
			return -1;
		else{
				thread_node->isBlockingParent=1;
				running_node->isBlockedonJoin=1;				
				enQueue(blockedQ,deQueue(readyQ));
				swapcontext(running_node->context,(peek(readyQ))->context);
			}
	}

	return 0;
}
 

