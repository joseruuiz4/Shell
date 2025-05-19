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


void manejador(int signal){
	block_SIGCHLD(); // bloqueamos la señal SIGCHLD
	job * item;
	int status, info;
	int pid_wait;

	enum status status_res;

	for(int i = 1; i <= list_size(jobs); i++){
		item = get_item_bypos(jobs, i); // obtenemos el job
		pid_wait = waitpid(item->pgid, &status, WUNTRACED | WNOHANG | WCONTINUED); // esperamos a que termine

		if(pid_wait == item->pgid){
			status_res = analyze_status(status, &info); // analizamos el estado

			if(status_res == SUSPENDED){
				printf("Process suspended, command: %s, %s, info: %d\n", item->command, status_strings[status_res], info);
				item->state = STOPPED; // cambiamos el estado a STOPPED
				

			}else if(status_res == CONTINUED){
				printf("Process continued, command: %s, %s, info: %d\n", item->command, status_strings[status_res], info);
				item->state = BACKGROUND; // cambiamos el estado a BACKGROUND

			}else if(status_res == EXITED){
				printf("Process exited, command: %s, %s, info: %d\n", item->command, status_strings[status_res], info);
				delete_job(jobs, item); // eliminamos el job de la lista

			}
		}

		unblock_SIGCHLD(); // desbloqueamos la señal SIGCHLD
	}

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
			block_SIGCHLD(); // bloqueamos la señal SIGCHLD

			print_job_list(jobs); // print job list
			
			unblock_SIGCHLD(); // desbloqueamos la señal SIGCHLD
			continue; // vuelve a pedir otro comando
		}

		if(strcmp(args[0], "fg") == 0){ // if command is fg
			block_SIGCHLD();

			int pos = 1;
			if(args[1] != NULL){
				pos = atoi(args[1]); // convert string to int
			}

			if(pos > list_size(jobs) || pos < 1){ // si la posicion es incorrecta
				fprintf(stderr, "Error, invalid position: %d\n", pos);
				fflush(stderr);
				continue; // vuelve a pedir otro comando
			}else{
				job * item = get_item_bypos(jobs, pos); // obtenemos el job
				new_process_group(item->pgid); // creamos un nuevo grupo de procesos
				set_terminal(item->pgid); // seteamos el terminal para el proceso en primer plano

				if(item->state == STOPPED){
					killpg(item->pgid, SIGCONT); // enviamos la señal SIGCONT al grupo de procesos
				}
				pid_wait = waitpid(item->pgid, &status, WUNTRACED); // esperamos a que termine

				set_terminal(getpid()); // devolvemos el terminal al padre

				status_res = analyze_status(status, &info); // analizamos el estado

				if(status_res == EXITED || status_res == SIGNALED){ // si el proceso ha terminado
					delete_job(jobs, item); // eliminamos el job de la lista

				}else if(status_res == SUSPENDED){ // si el proceso ha sido suspendido
					item->state = STOPPED; // cambiamos el estado a STOPPED
				}

			}	
			unblock_SIGCHLD(); // desbloqueamos la señal SIGCHLD
			continue; // vuelve a pedir otro comando
		}

		if(strcmp(args[0], "bg") == 0){ // if command is bg
			block_SIGCHLD(); // bloqueamos la señal SIGCHLD
			
			int pos = 1;
			if(args[1] != NULL){
				pos = atoi(args[1]); // convert string to int
			}

			if(pos > list_size(jobs) || pos < 1){ // si la posicion es incorrecta
				fprintf(stderr, "Error, invalid position: %d\n", pos);
				fflush(stderr);
				continue; // vuelve a pedir otro comando
			}else{
				job * item = get_item_bypos(jobs, pos); // obtenemos el job

				if(item->state == STOPPED){
					item->state = BACKGROUND; // cambiamos el estado a BACKGROUND
					killpg(item->pgid, SIGCONT); // enviamos la señal SIGCONT al grupo de procesos
				}
				
			}						
			unblock_SIGCHLD(); // desbloqueamos la señal SIGCHLD
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
					block_SIGCHLD(); // bloqueamos la señal SIGCHLD

					job * newjob = new_job(pid_fork, args[0], STOPPED);
					add_job(jobs, newjob); // add job to list

					printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_wait, args[0], status_strings[status_res], info);
					fflush(stdout);

					unblock_SIGCHLD(); // desbloqueamos la señal SIGCHLD
				}else if(status_res == EXITED || status_res == SIGNALED){ // Si el proceso ha terminado
					if(info != 255){	// Si no da error
						printf("Foreground pid: %d, command: %s, %s, info: %d\n", pid_wait, args[0], status_strings[status_res], info);
						fflush(stdout);
					}
				}
			}else{
				block_SIGCHLD(); // bloqueamos la señal SIGCHLD

				job * newjob = new_job(pid_fork, args[0], BACKGROUND);
				add_job(jobs, newjob); // add job to list

				printf("Background job running... pid: %d, command: %s\n", pid_fork, args[0]);
				fflush(stdout);
				
				unblock_SIGCHLD(); // desbloqueamos la señal SIGCHLD
			}
		}


	} 
}
