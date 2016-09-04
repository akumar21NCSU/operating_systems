

#define FUSE_USE_VERSION 30

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include<stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

#define BUFF 1024

typedef struct file_system{
	char * root_path;
	int fs_size;
}Ramdisk;


typedef struct node_t{
	char * fname;
	char type;
	char *buffer;
	int capacity;
	struct node_t* parent;
	struct node_t* children;
	struct node_t* sibling;

}Node;

Node* root;

static char *my_root_path = "";

static Node* getChildNode(Node* parent,char* name){

	Node* cur = parent->children;
	while(cur!= NULL){
		if(strcmp(cur->fname,name) == 0)
			return cur;
		cur = cur->sibling;
	}

	return NULL;

}

Node* getNode(const char * path){

	if(strcmp(path,"/") == 0)
		return root;

	Node* cur = root;
	char * temp = (char*)malloc(sizeof(char)*(strlen(path)+1));
	char* tok = (char*)malloc(sizeof(char) * (strlen(path) + 1));
	strcpy(temp, path);

	tok = strtok(temp, "/");
	while(tok){
		cur = getChildNode(cur,tok);
		if(cur == NULL){
			return NULL;	
		}

		tok = strtok(NULL, "/");
	}
	free(temp); free(tok); 
	return cur;

}

static int fill_size(Node* node){

	int size=0;
	if(node->type == 'D')
		return 150;
	else{
		if(node->buffer == NULL){
			node->buffer = (char*)malloc(node->capacity+1);
			size = 0;
		}
		else{
			size = strlen(node->buffer)+1;
		}
	}
	return size;
}


void fill_system_stat(struct stat *stbuf, Node* node)
{
	struct fuse_context* fc = fuse_get_context();

	stbuf -> st_dev = 100;
	//stbuf -> st_ino = 00102;
	
	stbuf -> st_size = fill_size(node);
	stbuf -> st_uid = fc->uid;
	stbuf -> st_gid = fc->gid;
	stbuf -> st_blksize = 512;
	stbuf -> st_blocks = 1;
	stbuf -> st_atime = stbuf -> st_ctime = stbuf -> st_mtime = time(NULL);

	if(node->type == 'D'){
		stbuf -> st_mode = S_IFDIR | 0755;
		stbuf -> st_nlink = 2;
	}
	else if(node->type == 'F'){
		stbuf -> st_mode = S_IFREG | 0755;
		stbuf -> st_nlink = 1;
	}
}

static int my_opendir(const char *path, struct fuse_file_info *fi)
{
	return 0;
}


static char* dirName(const char *path){

	char * temp = (char*)malloc(sizeof(char)*(strlen(path)+1));
	char* tok = (char*)malloc(sizeof(char) * (strlen(path) + 1));
	char* temp_tok = (char*)malloc(sizeof(char) * (strlen(path) + 1));
	strcpy(temp, path);

	tok = strtok(temp, "/");
	while(tok){
		strcpy(temp_tok, tok);
		tok = strtok(NULL, "/");
	}
	free(temp); free(tok); 
	return temp_tok;

}

static Node* getParent(const char *path){

	Node* pNode = root;
	
	char * temp = (char*)malloc(sizeof(char)*(strlen(path)+1));
	char* tok = (char*)malloc(sizeof(char) * (strlen(path) + 1));
	strcpy(temp, path);

	tok = strtok(temp, "/");
	while(tok){
		Node* tempP = getChildNode(pNode,tok);
		if(tempP == NULL){
			if(strtok(NULL, "/") != NULL){
				printf("Parent found\n");
				return NULL;
			}else{
				printf("Returning parent\n");
				return pNode;
			}			
		}

		pNode = tempP;
		tok = strtok(NULL, "/");
	}
	free(temp); free(tok); 
	return pNode;

}


static int my_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	Node* node = getNode(path);
	
	if(node != NULL)
		fill_system_stat(stbuf,node);
	else
		res = -ENOENT;

	return res;
}


static int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	
	(void) offset;
	(void) fi;	
	
	printf("Inside readdir path=%s \n",path);	

	Node* cur = getNode(path);
	if(cur == NULL)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	struct stat stbuf;
	cur = cur->children;

	while (cur != NULL) {
		printf("Filling stat for %s\n",cur->fname);
		memset(&stbuf, 0, sizeof(struct stat));		
		fill_system_stat(&stbuf,cur);
		filler(buf, cur->fname, &stbuf, 0);
		cur = cur->sibling;
	}
	
	printf("Exiting readdir \n");
	return 0;
}

static int my_mknod(const char *path, mode_t mode, dev_t rdev)
{
	printf(" MkNod Path = %s\n",path);
	if(root->capacity < 100)
		return -ENOMEM;
	
	char* tmp = (char*)malloc(sizeof(char)*(strlen(path) + 5));
	strcpy(tmp,path);

	char* dName = dirName(path);
	printf("MkNod - new file name = %s\n",dName);

	
	Node* pNode = getParent(path);
	if(pNode == NULL)
		return -ENOENT;

	Node* newNode = (Node*)malloc(sizeof(struct node_t));
	newNode->fname= dName;
	newNode->type = 'F';
	newNode->capacity = BUFF;
	newNode->buffer = NULL;
	newNode->parent = pNode;
	newNode->sibling = NULL;
	newNode->children = NULL;

	if(pNode->children == NULL){
		pNode->children = newNode;
		printf("Allocating to parent - first child\n");
	}
	else{
		printf("Allocating to sibling\n");
		Node* lastChild = pNode->children;
		while(lastChild->sibling != NULL)
			lastChild = lastChild->sibling;

		lastChild->sibling = newNode;
	}
	
	printf("Decreasing root capacity in mknod\n");
	root->capacity = root->capacity -100 - BUFF;
	printf("Exiting mknod\n");

	return 0;
}


static int my_open(const char *path, struct fuse_file_info *fi)
{
	printf("Entering open for path=%s\n",path);
	Node* n = getNode(path);
	if(n == NULL)
		return -ENOENT;

	printf("Exiting open\n");
	return 0;
}

static int read_file(Node* n, char* buf, size_t size, off_t offset){

	int s = strlen(n->buffer);

	if(offset > s)
		return -1;

	if(offset+size > s)
		size = s- offset;

	int count=0;
	while(count < size){
		buf[count] = n->buffer[offset+count];
		count++;
	}
	buf[count] = '\0';
	printf("Buffer is %s\n",buf);
	return count;
}



static int my_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	(void) fi;
	Node* n = getNode(path);
	if(n == NULL)
		return -ENOENT;

	int len = read_file(n,buf,size,offset);

	return len;
}

static int write_file(Node* n,const char* buf,size_t size,off_t offset){
	
	char* temp = NULL;
	if(offset + size > n->capacity){
		printf("Decreasing root capacity in write file\n");
		root->capacity = root->capacity + n->capacity;
		n->capacity = offset+size+BUFF;
		root->capacity = root->capacity - n->capacity;
	}
	temp = (char*)malloc(n->capacity);

	if(offset > 0){
		offset = offset-1;
		memcpy(temp,n->buffer,offset);

	}

 
	int count=0;
	while(count < size){
		
		if(buf[count] != '\0'){
			temp[offset+count] = buf[count];
		}else{
			temp[offset+count] = '\0';
			break;
		}
		count++;
	}	
	
	temp[offset+count] = '\0';	
	char* delTemp = n->buffer;
	n->buffer = temp;
	free(delTemp);
	printf("Exiting write file - size = %d\n",count);	
	
	return count;
}

static int my_write(const char *path, const char *buf, size_t size,
		     off_t offset, struct fuse_file_info *fi)
{
	Node* n = getNode(path);
	if(n == NULL)
		return -ENOENT;

	int len = write_file(n,buf,size,offset);

	return len;
}


static int my_mkdir(const char *path, mode_t mode)
{
	printf(" Mkdir Path = %s and root capacity is %d\n",path,root->capacity);
	
	if(root->capacity < 100)
		return -ENOMEM;

	char* tmp = (char*)malloc(sizeof(char)*(strlen(path) + 5));
	strcpy(tmp,path);

	char* dName = dirName(path);
	printf("Mkdir - new dir name = %s\n",dName);

	
	Node* pNode = getParent(path);
	if(pNode == NULL)
		return -ENOENT;

	Node* newNode = (Node*)malloc(sizeof(struct node_t));
	newNode->fname= dName;
	newNode->type = 'D';
	newNode->capacity=100;
	newNode->buffer = NULL;
	newNode->parent = pNode;
	newNode->sibling = NULL;
	newNode->children = NULL;

	if(pNode->children == NULL){
		pNode->children = newNode;
		printf("Allocating to root\n");
	}
	else{
		printf("Allocating to sibling\n");
		Node* lastChild = pNode->children;
		while(lastChild->sibling != NULL)
			lastChild = lastChild->sibling;

		lastChild->sibling = newNode;
	}
	printf("Decreasing root capacity in mkdir\n");
	root->capacity = root->capacity -100;
	printf("Exiting mkdir\n");

	return 0;
}
static void removeFromParent(Node* par,char* name){
	
	Node* parent = par;

	Node* cur = parent->children;
	Node* prev= cur;

	if(strcmp(cur->fname,name) == 0){
		cur=cur->sibling;		
		parent->children = cur;
		return;
	}

	cur=cur->sibling;
	while(cur!= NULL){
		if(strcmp(cur->fname,name) == 0){
			prev->sibling = cur->sibling;
			break;
		}
		prev=cur;
		cur=cur->sibling;
	}

	
}

static int my_rmdir(const char *path)
{

	Node* n = getNode(path);
	if(n == NULL)
		return -ENOENT;

	if(n->children != NULL){
		return -ENOTEMPTY;
	}

	removeFromParent(n->parent,dirName(path));
	free(n);
	root->capacity = root->capacity +100;

	return 0;
}

static int my_unlink(const char *path)
{

	Node* n = getNode(path);
	if(n == NULL)
		return -ENOENT;

	removeFromParent(n->parent,dirName(path));
	root->capacity = root->capacity + n->capacity + 100;
	free(n);
	
	return 0;
}

static int my_truncate(const char *path, off_t size)
{
	
	return 0;
}


static struct fuse_operations my_rmdk = {
	.getattr	= my_getattr,
	.readdir	= my_readdir,
	.open		= my_open,
	.read		= my_read,
	.mkdir		= my_mkdir,
	.opendir	= my_opendir,
	.rmdir		= my_rmdir,
	.unlink		= my_unlink,
	.mknod		= my_mknod,
	.write		= my_write,
	.truncate	= my_truncate,
};

int main(int argc, char *argv[])
{
	if(argc <3){
		printf("Usage - ./ramdisk /dir size\n");
		exit(-1);
	}
	umask(000);

	my_root_path = argv[1];	
	int size = atoi(argv[2]);


	argc =2;	

	root = (Node*)malloc(sizeof(struct node_t));
	root->fname= "/";
	root->type = 'D';
	root->capacity = size*1024*1024;
	root->buffer = NULL;
	root->parent = NULL;
	root->sibling = NULL;

	return fuse_main(argc, argv, &my_rmdk, NULL);
}
   


