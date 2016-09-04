/******************************************************************************
 *
 *  File Name........: fork.c
 *
 *  Description......: Simple Ush
 *
 *  Author...........: Abhishek Kumar
 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "parse.h"
#define PERMS 0666 

extern char **environ;

static void myEcho(char * args[]){
	int i=1;
	while(args[i+1] != NULL){
		printf("%s ",args[i]);
		i++;
	}
	if(args[i] != NULL)
		printf("%s",args[i]);

	printf("\n");
}

static int isDir(char* fpath){

	struct stat statbuf;
    stat(fpath, &statbuf);
    return S_ISDIR(statbuf.st_mode);

}

static int myCd(char * args[]){

	char* dir;
	if(args[1] == NULL){
		dir = getenv("HOME");
	}else{
		dir = args[1];
	}
	
	if(chdir(dir)){
		/*if(errno == EACCES){
			fprintf(stderr,"%s\n",strerror(errno));
		}
		else if(errno == ENOTDIR){
			fprintf(stderr,"%s\n",strerror(errno));
		}
		else if(errno == ENOENT){
			fprintf(stderr,"%s\n",strerror(errno));
		}*/
		fprintf(stderr,"%s\n",strerror(errno));
		return -1;
	}
	return 0;
}

static int myNice(char ** args){

	
	if(args[1] == NULL){
		int p = getpriority(PRIO_PROCESS,0);
		printf("%d\n",p);
		return 0;
	}

	int p=4;
	
	int device_num = 0;

	if(sscanf(args[1], "%d", &device_num) != EOF) {
    	p=getpriority(PRIO_PROCESS,0);
		p = 0 -p;
		nice(p);
		if(args[2] == NULL)
			nice(device_num);	
		else
			return 2;	
	}	
	else{
		return 1;
	}
	return 0;

}
static char* formPath(char* a, char* b){

	char * result = (char*)malloc(sizeof(a)+sizeof(b)+sizeof(char));
	strcat(result,a);
	strcat(result,"/");
	strcat(result,b);
	//printf("a = %s",a);
	//printf("Fullpath = %s\n",result);
	return result;

}

static void myWhere(char ** args){

	if(args[1] == NULL)
		return;


	if(strcmp(args[1],"cd") ==0 || strcmp(args[1],"pwd") == 0
		|| strcmp(args[1],"echo") == 0 || strcmp(args[1],"nice")==0
		|| strcmp(args[1],"where") == 0 || strcmp(args[1],"bg")==0
		|| strcmp(args[1],"fg") == 0 || strcmp(args[1],"kill")==0
		|| strcmp(args[1],"setenv") == 0 || strcmp(args[1],"unsetenv")==0
		|| strcmp(args[1],"logout") == 0 || strcmp(args[1],"jobs")==0){
	
		printf("%s is a shell builtin\n",args[1]);

	}

	

	char* pathstr = getenv("PATH");
	char* pathcpy = (char*)malloc(sizeof(pathstr));
	
	pathcpy = strcpy(pathcpy,pathstr);

	char *array[20];
	int i=0;

	array[i] = strtok(pathcpy,":");

	while(array[i]!=NULL)
	{
	   array[++i] = strtok(NULL,":");
	}
	i=0;
	while(array[i] != NULL){
		//printf("%s\n",array[i]);
		char nf[100];
		strcpy(nf,array[i]);
		int begin = strlen(array[i]);
		nf[begin] ='/';
		begin++;
		int l = strlen(args[1]);
		int j=0;
		for(j=0;j<=l;j++)
			nf[begin+j] = args[1][j];
		//printf("%s\n",nf);
		i++;
		if( access( nf, F_OK ) != -1 )
			printf("%s\n",nf);
	}

}

static void myPwd(){
	int PATH_MAX = 100;
	char* cwd;
    char buff[PATH_MAX + 1];

    cwd = getcwd( buff, PATH_MAX + 1 );
    if( cwd != NULL ) {
        printf( "%s\n", cwd );
    }
}

char* concatString(char* a, char* b){
	char * result = (char*)malloc(sizeof(a)+sizeof(b)+sizeof(char));
	strcat(result,a);
	strcat(result,"=");
	if(b != NULL)
		strcat(result,b);
	return result;

}

static void mySetenv(char ** args){

	if(args[1] == NULL){
		char **env;
		for (env = environ; *env; ++env)
        	printf("%s\n", *env);
	}
	else if(args[2] == NULL){
		char* newEnv = concatString(args[1],"\0");
		putenv(newEnv);
	}else if(args[3] == NULL){
		char* newEnv = concatString(args[1],args[2]);
		putenv(newEnv);
	}else{
		if(strcmp(args[3],"0") !=0){
			char* newEnv = concatString(args[1],args[2]);
			putenv(newEnv);
		}
	}

}

static void myUnsetenv(char ** args){

	if(args[1] == NULL)
		return;

	unsetenv(args[1]);
	
}


static int processLastCmd(Cmd c){

	if(c->out != Tnil){

		FILE* f;
		int f2;
		switch ( c->out ) {
		  	case Tout:
				if ((f = fopen(c->outfile, "w")) == NULL){
					if ((f2 = creat(c->outfile,PERMS)) == -1){
						fprintf(stderr,"%s\n",strerror(errno));			
						exit(0);
					}
				}else{
					f2 = fileno(f);
				}
			break;
			case Tapp:
				if ((f = fopen(c->outfile, "a")) == NULL){
				if ((f2 = creat(c->outfile,PERMS)) == -1){
					fprintf(stderr,"%s\n",strerror(errno));				
					exit(0);
				}
				}else{
					f2 = fileno(f);
				}
			break;
			case ToutErr:
				if ((f = fopen(c->outfile, "w")) == NULL){
				if ((f2 = creat(c->outfile,PERMS)) == -1){
					fprintf(stderr,"%s\n",strerror(errno));			
					exit(0);
				}
				}else{
					f2 = fileno(f);
				}
				dup2(f2,2);				
			break;
			case TappErr:
				if ((f = fopen(c->outfile, "a")) == NULL){
				if ((f2 = creat(c->outfile,PERMS)) == -1){
					fprintf(stderr,"%s\n",strerror(errno));				
					exit(0);
				}
				}else{
					f2 = fileno(f);
				}
				dup2(f2,2);
			break;
			default:
			fprintf(stderr,"%s\n",strerror(errno));	
			exit(-1);
      }
		
		dup2(f2,1);
		close(f2);
	}

}


static int validateCmd(Cmd c){

	if( access( c->args[0], F_OK ) != -1 ) {
    	if(access(c->args[0], X_OK) == -1 || isDir(c->args[0]))
			return -1;
	}
	return 0;
}

static int validateFile(char* fname){

	if( access( fname, F_OK ) == -1 ) {

		fprintf(stderr,"No such file or directory\n");
		return -1;
	}
	if( isDir(fname)){
		fprintf(stderr,"No such file or directory\n");
		return -1;
	}
	if(access(fname, R_OK) == -1 || access(fname,W_OK) == -1){
		fprintf(stderr,"Permission denied\n");
		return -1;
	} 
	
	 
	return 0;
}

static int runBuiltIn(Cmd c){

	char* fname = c->args[0];
	if(strcmp(fname,"echo") == 0){
			myEcho(c->args);
				
	}
	else if(strcmp(fname,"cd") == 0){
		myCd(c->args);
	}
	else if(strcmp(fname,"pwd") == 0){
		myPwd();
	}
	else if(strcmp(fname,"setenv")==0){
		mySetenv(c->args);
	}
	else if(strcmp(fname,"unsetenv")==0){
		myUnsetenv(c->args);
	}
	else if(strcmp(fname,"where") == 0){
		myWhere(c->args);
	}
	else if(strcmp(fname, "nice") == 0){
		int rev= myNice(c->args);
		if(rev != 0){
			c->args = &(c->args[rev]);
			return 1;
		}
			
	}
	else if(strcmp(fname,"logout") == 0){
		exit(0);
	}
	else
		return -1;
	
	return 0;
}

static void prPipe(Pipe p)
{
    int i = 0;
	int oldstdin = dup(0);
	int oldstdout = dup(1);
	int oldstderr = dup(2);
    Cmd c;
	Cmd first = p->head;
	int parray[1000];
	int pcounter=0;

  for ( c = p->head; c->next != NULL; c = c->next ) {
	
	int pd[2];

	if(pipe(pd) == -1){
		fprintf(stderr,"%s\n",strerror(errno));	
		return ;
	}

	int perm = validateCmd(c);
	if(perm == -1){
		fprintf(stderr,"permission denied\n");
		return;
	}

	if(c == first){
		if(c->in == Tin){
			FILE* f;
			int f2;
			int valFile = validateFile(c->infile);
			if(valFile == -1)
				return;
			if ((f = fopen(c->infile, "r")) == NULL){
				fprintf(stderr,"No such file or directory\n");
				return;
			}
			f2 = fileno(f);
			dup2(f2,0);
		}
	}	

	int pid = fork();
	
	if(pid == -1){
		fprintf(stderr,"Error forking child\n");
		return ;
	}

	if(pid == 0){		

		signal(SIGINT,SIG_DFL);
			signal(SIGHUP,SIG_DFL);
			signal(SIGKILL,SIG_DFL);
			signal(SIGTERM,SIG_DFL);
			signal(SIGSTOP,SIG_DFL);

		dup2(pd[1],1);		
		close(pd[0]);
		close(pd[1]);

		if(runBuiltIn(c) == 0){
			exit(0);
		}
		else{
			if ( execvp(c->args[0],c->args)) 
			{
				if(errno == ENOENT){
					fprintf(stderr,"command not found\n");	
				}	
				 exit(0);
			}
		}
	}
	
    dup2(pd[0],0);
	close(pd[1]);
	close(pd[0]);
  }

	//execute last command here and display the result on terminal or redirect.
	int perm = validateCmd(c);
	if(perm == -1){
		fprintf(stderr,"permission denied\n");
		return;
	}

	if(c == first){
		if(c->in == Tin){
			FILE* f;
			int f2;
			int valFile = validateFile(c->infile);
			if(valFile == -1)
				exit(0);
			if ((f = fopen(c->infile, "r")) == NULL){
				if(errno == ENOENT)
					fprintf(stderr,"No such file or directory\n");
				return;
			}
			f2 = fileno(f);
			dup2(f2,0);
		}
	}	
		
	if(processLastCmd(c) == -1)
		return;

	if(runBuiltIn(c) != 0){			
		
		int lchild = fork();
		if(lchild == 0){	

			signal(SIGINT,SIG_DFL);
			signal(SIGHUP,SIG_DFL);
			signal(SIGKILL,SIG_DFL);
			signal(SIGTERM,SIG_DFL);
			signal(SIGSTOP,SIG_DFL);		
		
			if ( execvp(c->args[0],c->args) == -1) 
			{			
				if(errno == ENOENT)
					fprintf(stderr,"command not found\n");
				else
					fprintf(stderr,"%s",strerror(errno));
			}
			return;
		}
	}
	while (wait(NULL) > 0);	
	dup2(oldstdin,0);
	dup2(oldstdout,1);
	dup2(oldstderr,2);
	close(oldstdin);	
	close(oldstdout);
	close(oldstderr);
	
}

static void executeUshrc()
{
	Pipe p;
	fpos_t pos;
	char* home = getenv("HOME");
	char* homecpy = (char*)malloc(sizeof(home) + 8*sizeof(char));
	strcpy(homecpy, home);

	strcat(homecpy, "/.ushrc");
	if(access(homecpy, R_OK ) == -1) 
	{
		fprintf(stderr,"%s\n",strerror(errno));
		return ;
	}
    fflush(stdin);
    fgetpos(stdin, &pos);
    int oldstdin = dup(0);
    freopen(homecpy, "r", stdin);

	while ( 1 ) {
    p = parse();
	if(p == NULL)
		break;
	if(p!= NULL && strcmp((p->head)->args[0],"logout")==0)
		break;
	while(p != NULL){
		prPipe(p);
		p= p->next;
	}  
    freePipe(p);	
  }

    fflush(stdin);
	fflush(stdout);
    dup2(oldstdin, 0);
    close(oldstdin);
    fsetpos(stdin, &pos); 	
}


int main(int argc, char *argv[])
{
    Pipe p;
	char host[256];
	gethostname(host, 255);

	signal(SIGINT,SIG_IGN);
	signal(SIGHUP,SIG_IGN);
	signal(SIGKILL,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGSTOP,SIG_IGN);

	executeUshrc();
	
  while ( 1 ) {
	printf("%s%% ", host);	
	fflush(stdout);
    p = parse();
	if(p!= NULL && strcmp((p->head)->args[0],"logout")==0)
		break;
	
	while(p != NULL){
		prPipe(p);
		p= p->next;
	}  
    freePipe(p);	
  }
}

/*........................ end of main.c ....................................*/
