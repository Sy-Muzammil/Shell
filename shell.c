#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

char *argv[1000];
char *command[1000];
int p_count;
int new_Pipe[2],old_Pipe[2];
int redr_read;
int redr_write;
char *ncommand[1000];
char *hist[1000];
int h=0;
int c_count;


void delete_space()
{
int n,i;
n=strlen(command[1]);
	for(i=0 ; i<n ; i++)
	{
		command[1][i]=command[1][i+1];
	}
	
}
void builtin_commands()
{
if(strncmp(command[0],"cd",2)==0 && c_count >1)
	{
	if(chdir(command[1])!=0);
			perror("chdir()");
	}
	else if(strncmp(command[0],"cd",2)==0 && c_count ==1)
	{
		if(chdir(getenv("HOME"))!=0);
			perror("chdir()");
	}
	else if(strncmp(command[0],"pwd",3)==0 && c_count == 1)
	{
		char path[PATH_MAX+1];
		char *pp;
		pp=getcwd(path,PATH_MAX);
		fprintf(stdout,"%s\n",pp);
	}
	else if(strncmp(command[0],"history",7)==0 && c_count == 1)
	{
		int b;
		for(b=0;b<h;b++)
		printf("%d: %s\n",b+1,hist[b]);
	}
}

			/** Execute the command here **/

int execute(int i)
{
int status,pid;

	if((strncmp(command[0],"cd",2)==0)
	||(strncmp(command[0],"pwd",3)==0 && p_count == 1)
	||(strncmp(command[0],"export",6)==0)
	||(strncmp(command[0],"history",7)==0))
	{
		builtin_commands();
	}

	else
	{

		if(i < p_count-1) 
			pipe(new_Pipe); /* create a pipe */
		pid = fork();

		if(i>0 && i<p_count-1)	/** Middle commands only reads outside writes inside fork() **/
		{
			dup2(old_Pipe[0], 0); 
			close(old_Pipe[1]);
			close(old_Pipe[0]);

		}
		else if(i==p_count-1)	/** for last command read happens from pipe[0]**/
		{
			dup2(old_Pipe[0], 0); /** last command will not write on pipe it will by default write on console()**/
			close(old_Pipe[1]);
			close(old_Pipe[0]);

		}
			
		if(pid == 0)  /* Child */
		{
			if(i==0)
			{
				dup2(new_Pipe[1], 1); /** First command will do only stdout goes into new_Pipe[1]**/
				close(new_Pipe[0]);
				close(new_Pipe[1]);

			}

			if(i>0 && i<p_count-1)/** Middle commands will do stdout on new_Pipe[1] **/
			{
			dup2(new_Pipe[1], 1); 
			close(new_Pipe[0]);
			close(new_Pipe[1]);
			}

			if(redr_read == 1)
			{
				int ex;
				if((ex=execvp(command[0],ncommand))==-1)
				{
					printf("Error. Command not found: %s\n", command[0]);
					exit(1);	
				}
			}

			if(redr_write == 1)
			{
				int ex;
				if((ex=execvp(command[0],ncommand))==-1)
				{
					printf("Error. Command not found: %s\n", command[0]);
					exit(1);	
				}
			}

			if(redr_write == 0 && redr_read ==0)
			{	
				int ex;
				if((ex=execvp(command[0],command))==-1)
				{
					printf("Error. Command not found: %s\n", command[0]);
					exit(1);	
				}
			}	
		} 
		/*** parent process waits for each command to execute ***/
		else 
		waitpid(pid, &status, 0);			

			
			/* Contents of newpipe is copied into old pipe for the reusablity of newpipe .*/

			if(i < p_count-1) 
			{
			old_Pipe[0] = new_Pipe[0];
			old_Pipe[1] = new_Pipe[1];
			}
	}
}


	/** Seperate each Command wrt spaces AND then send them one by one to execute command**/

void seperate_command(char *argv[])
{
int i = 0, j, k = 0,c=0;
char *token;
int flag=1,pid,fd_in,fd_out;	
int status;
int in, out;				
	in=dup(STDIN_FILENO);	
	out=dup(STDOUT_FILENO);
	for(i = 0; i < p_count; i++) /* For each command */
	{
		 redr_read=0;
		 redr_write=0;
		 int nc=0;
		for(j=0;j<strlen(argv[i]);j++)
		{
			if(argv[i][j] == '<')
			{
				redr_read = 1;
				//printf("redr_read is 1\n");	
			}
			if(argv[i][j] == '>')
			{
				redr_write = 1;
				//printf("redr_write is 1\n");
			}

		}		
		c=0;	
			if(redr_read == 1)
			{
				token = strtok(argv[i], "<");
				while(token!=NULL)
				{
					token[strlen(token)]='\0';
					command[c]=token;
					printf("command[%d]: %s\n",c,command[c]);
					c++;
					token = strtok(NULL,"<");
				}
				delete_space(command[1]);
				int len1=strlen(command[1]);
				//printf("len1: %d\n",len1);
				//command[1][len1-1]='\0';
				//len2 = strlen(command[1]);
				//printf("len2: %d\n",len2);
				printf("command[1]: %s\n", command[1]);
				command[c]='\0';

				token = strtok(command[0]," ");
				while(token!=NULL)
				{
					token[strlen(token)]='\0';
					ncommand[nc]=token;
					//printf("ncammand[%d]: %s\n",nc,ncommand[nc]);
					nc++;
					token = strtok(NULL," ");
				}
				ncommand[nc]='\0';
				//printf("command[1]: %s\n", command[1]);
				if((fd_in = open(command[1],O_RDONLY))<0)
				{
					perror(command[1]);
					exit(1);
				}
				//printf("dup %d\n",fd_in);

				dup2(fd_in,STDIN_FILENO);
				//printf("i: %d\n",i);		

			}
			else if(redr_write == 1)
			{
				token = strtok(argv[i], ">");
				while(token!=NULL)
				{
					token[strlen(token)]='\0';
					command[c]=token;
					//printf("cammand[%d]: %s\n",c,command[c]);
					c++;
					token = strtok(NULL,">");
				}
				command[c]='\0';
				delete_space();
				//int len1=strlen(command[1]);
				//printf("len1: %d\n",len1);
				//command[1][len1-1]='\0';
				//nc=0;
				token = strtok(command[0]," ");
				while(token!=NULL)
				{
					token[strlen(token)]='\0';
					ncommand[nc]=token;
					//printf("ncammand[%d]: %s\n",nc,ncommand[nc]);
					nc++;
					token = strtok(NULL," ");
				}
				int len4 = strlen(ncommand[0]);
				//printf("len: %d\n",len4);
				ncommand[nc]='\0';
				if(fd_out = (open(command[1], O_RDWR | O_CREAT,0777)) < 0)
				{
					perror(command[1]);
					exit(1);
				}
				//printf("fd_out: %d\n",fd_out);	
				dup2(fd_out,STDOUT_FILENO);

			}
			else
			{
			token = strtok(argv[i]," ");
				while(token != NULL)
				{
				token[strlen(token)] = '\0';
				command[c] = token;
			//	printf("command[%d] = %s\n",c, command[c]);
				c++;
				token = strtok(NULL," ");
				}
			command[c++] = token;
			c_count = c-1;
			}
	execute(i);
	}
	
	

	close(old_Pipe[0]);
	close(new_Pipe[0]);
	close(old_Pipe[1]);
	close(new_Pipe[1]);
	dup2(in, 0);
	dup2(out, 1);
	close(in);
	close(out);
}
					
	//for(j = 0 ; j < i ; j++)
	//	printf("command[%d] = %s\n",j, command[j]);
	/*if((strncmp(command[0],"cd",2))!=0)
	execute();/** First execute for builtin commands**/
	
	/**execute if command not builtin**/
	//else	
	//builtin_commands();
	//}
//}

		/** Seperate Commands By pipe**/

void seperate_pipe_command(char *line, char *argv[])
{
int i=0,j;
//printf("line = %s\n", input);
char *token=strtok(line,"|");
	while(token!=NULL)
	{
		token[strlen(token)]='\0';
		argv[i]=token;
		i++;
		token=strtok(NULL,"|");
	}
	argv[i]='\0';
	int len=strlen(argv[i-1]);
	argv[i-1][len-1]='\0';

p_count=i;
//printf("pcount1 : %d\n",p_count);

//for(j=0;j<i;j++)
//	printf("argv[%d] = %s\n",j, argv[j]);
}
	/** Read the input command **/

char* read_command()
{
char *line = NULL;
size_t len = 0;
ssize_t read = 0;
read = getline(&line, &len, stdin);
//printf("%s\n", line);
//seperate_pipe_command(line,argv);
return line;
}

		/** main function **/
int main( )
{
char* cwd;
char *text;
int status;
//char **parse;
char buff[PATH_MAX + 1];
	do
	{
	cwd = getcwd( buff, PATH_MAX + 1 );
    		if( cwd != NULL )
        		printf( "MyShell~%s$ ", cwd );
    
	/**three steps "READ" then "SEPERATE" then "EXECUTE"  **/

	
	text = read_command();
		if(h==0)
		{
		hist[h]=text;
		h++;	
		}
		if(h>0)
		{
			if(hist[h-1]!=text)
				hist[h]=text;
		}
	seperate_pipe_command(text,argv);
	seperate_command(argv);
	if (strncmp(argv[0], "exit",4) == 0)  
  	             exit(1); 
	//status=execute_command();
	}while(1);
}
