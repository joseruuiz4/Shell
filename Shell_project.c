/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c 
#include <string.h>       // para comparar cadenas

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job * jobs; // job list






// -----------------------------------------------------------------------
//                            MANEJADOR          
// -----------------------------------------------------------------------


void manejador(){


	
}





// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */



	ignore_terminal_signals(); // ignore terminal signals
	new_process_group(getpid()); // create new process group
	set_terminal(getpid()); // set terminal for foreground process

	jobs = new_list("Lista de jobs"); // create new job list

	signal(SIGCHLD, manejador); // vincular la señal SIGCHLD al manejador

	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		printf("COMMAND->");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command

		if(strcmp(args[0], "cd") == 0){ // if command is cd
			if(args[1] == NULL){
				chdir(getenv("HOME")); // cambiamos a directorio home
			}else{
			if(chdir(args[1]) != 0){
				perror("Error changing directory");
				}
			}
			continue; // vuelve a pedir otro comando
		}

		if(strcmp(args[0], "jobs") == 0){ // if command is jobs
			print_job_list(jobs); // print job list
			continue; // vuelve a pedir otro comando
		}



		pid_fork = fork(); // create a child process

		if(pid_fork == 0){ // Proceso Hijo
			new_process_group(getpid()); 
			
			if(background == 0){
				set_terminal(getpid()); // set terminal for foreground process

			}

			restore_terminal_signals(); // restore terminal signals

			


    		execvp(args[0], args); // ejecuta el comando
    		fprintf(stderr, "Error, command not found: %s\n", args[0]);
			fflush(stderr);
			exit(-1); // exit child process
		}else if(pid_fork > 0){ // Proceso Padre
			if(background == 0){
				
				pid_wait = waitpid(pid_fork, &status, WUNTRACED); // wait for child process
				set_terminal(getpid()); // set terminal for foreground process

				status_res = analyze_status(status, &info); // analyze status
				
				if(status_res == SUSPENDED){ // Si el proceso se ha suspendido
					job * newjob = new_job(pid_fork, args[0], STOPPED);
				add_job(jobs, newjob); // add job to list
					printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_wait, args[0], status_strings[status_res], info);
					fflush(stdout);
				}else if(status_res == EXITED || status_res == SIGNALED){ // Si el proceso ha terminado
					if(info != 255){	// Si no da error
						printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_wait, args[0], status_strings[status_res], info);
						fflush(stdout);
					}
				}
			}else{

				job * newjob = new_job(pid_fork, args[0], BACKGROUND);
				add_job(jobs, newjob); // add job to list

				printf("Background job running... pid: %d, command: %s\n", pid_fork, args[0]);
				fflush(stdout);
				
			}
		}


	} 
}
