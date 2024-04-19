#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h> // termios, TCSANOW, ECHO, ICANON
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <limits.h>

// Digit representations for digital_clock function
#include "large_digits.h"

const char *sysname = "Shellect";

#define SIZE        40000
#define LINE_SIZE   2000

// Global variable for tracking status of kernel module
int kernel_module_loaded = -1;

enum return_codes {
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};

struct command_t {
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3]; // in/out redirection

	// New struct variables for fixing white space issue in redirections
	char *redirect_files[3];
	int redirect_idxs[3];

	struct command_t *next; // for piping
};

/* simple replace function that replaces orig in str by rep, where orig starts at loc*/
char* replace(char* str, char* orig, char* rep, char* loc) {
    static char buffer[4096];
    static char buffer2[4096];
    strncpy(buffer, str, loc-str); 
    buffer[loc-str] = '\0';
    strcpy(buffer2, loc+strlen(orig));
    sprintf(buffer+(loc-str), "%s%s", rep, buffer2);
    return buffer;
}

/* a function that returns the text in filename, to be used in multiple parts */
char* read_from_file(char* filename) {
    /* reads the text in the given filename and returns */
    char* line = malloc(sizeof(char)*LINE_SIZE);
    char* str = malloc(sizeof(char)*SIZE);
    FILE* ptr; 
    
    ptr = fopen(filename, "r");     
   if (ptr == NULL) { 
        return NULL;
    } 
        
    while (fgets(line, LINE_SIZE, ptr) != NULL) {
        strcat(str, line);
    }
    fclose(ptr);
    return str;
}

/* checking for aliases that are in terminal input, and replacing them*/
char* check_for_aliases(char* buf) {
	char* alias_file;
	// return buf if there is not a predefined alias
	if ((alias_file = read_from_file("alias.txt")) == NULL) {
		return buf;
	}
	// in alias.txt, there is an alias defined in each new line
	const char s[2] = "\n";
   	char* end;
	char alias[100], comm[400];
	int index;
	// get the first alias&command pair
	char* token = strtok(alias_file, s);
	// operate on temp_buf rather and original buf
	char* temp_buf = malloc(4096 * sizeof(char));
	strcpy(temp_buf, buf);
	strcat(temp_buf, " ");
	// walk through other aliases
	while( token != NULL ) {
		// alias and command are separated with ";" 
		end = strchr(token, ';');
		index = (int)(end - token);
		// alias is before ;
		strncpy(alias, token, index);
		// command is after ; and adding 1 space so that it doesn't cause any issue while replacing
		strcpy(comm, token + index + 1);
		strcat(comm, " ");
		char temp[index];
		// temp stores the beginning of the terminal prompt for the length of alias
		strncpy(temp, temp_buf, index);
		// adding a space to alias so that we don't replace something that is inside a word
		strcat(alias, " ");
		char *loc;
		// location of the alias in terminal prompt
		loc = strstr(temp_buf, alias);
		if (loc != NULL) { // alias is in prompt
		    if ((int)(loc - temp_buf) == 0) { // alias is at the beginning of the prompt
		        // alias in buf
				temp_buf = replace(temp_buf, alias, comm, loc);
		    }
            else {
                if((loc-1)[0] == 32) { // alias is not at the beginning of the prompt, so checking whether the character one before alias in the prompt is space
                    // alias in buf
					temp_buf = replace(temp_buf, alias, comm, loc);
                }
            }
		} 
		token = strtok(NULL, s);
	}
	return temp_buf;
}

// Helper function for xdd
void print_xdd(char* input, int group_size) {
	for(int i=0; (unsigned)i<strlen(input)/16+1; i++) {
        printf("%08x: ",i*16); // print the current number of bytes printed in hexadecimal
        for(int j=0; j<16; j++){ // printing 16 chars in each row
			if(input[i*16+j] == '\0') break; // stop printing at the end of string
            printf("%02x", input[i*16+j]);
            if((j+1) % group_size == 0){ // printing a space after printing GROUP_SIZE bytes, this organizes the column structure
                printf(" ");
            }
        }
        printf("\n");
    }
}

// Helper function for good_morning
void good_morning(char* minute_as_text, char* music_file_path) {
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) == NULL) {
		perror("getcwd() error");
	}
	int minutes = atoi(minute_as_text);

	if (minutes <= 0) {
		printf("Please provide a positive number of minutes.\n");
	}

	// Opening the file that uses sox and be called on cronjobs
	FILE *file_alarm = fopen("alarm.sh", "w");
	if (file_alarm == NULL) {
		perror("Error for opening alarm.sh");
	}

	freopen(NULL,"w+",file_alarm);
	
	fprintf(file_alarm,"#!/bin/bash\n");
	fprintf(file_alarm,"PATH=$PATH:/bin\n");
	fprintf(file_alarm,"export DISPLAY=:0.0\n");
	fprintf(file_alarm, "play ");
	fprintf(file_alarm, "%s",music_file_path); 

	fclose(file_alarm);

	// Get the current time
	time_t now;
	time(&now);

	// Calculate the future time by adding minutes
	time_t futureTime = now + (minutes * 60);

	// Convert the future time to a tm struct
	struct tm *futureTimeInfo = localtime(&futureTime);

	FILE *file = fopen("cron_creator.sh", "w");
	if (file == NULL) {
		perror("Error creating text");
	}

	fprintf(file,"(crontab -l 2>/dev/null || true; echo \"");
	fprintf(file,"%d %d %d %d %d ",
		futureTimeInfo->tm_min,
		futureTimeInfo->tm_hour,
		futureTimeInfo->tm_mday,
		futureTimeInfo->tm_mon + 1,
		futureTimeInfo->tm_wday); 

	fprintf(file, "%s/alarm.sh ",cwd);
	//fprintf(file, "> /home/barisyakar/Desktop/log.txt 2>&1 ");
	//fprintf(file, "> /home/kida/Desktop/log.txt 2>&1 ");
	fprintf(file,"\") | crontab - \n");
	
	fclose(file);

	// Change file permissions (e.g., make it executable)
	int result = chmod("cron_creator.sh", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	if (result != 0) {
		perror("Error changing file permissions");
	}

	strcat(cwd,"/cron_creator.sh");

	// giving system command and storing return value
	int returnCode = system(cwd);
	
	// checking if the command was executed successfully
	if (returnCode != 0) {
		printf("Command execution failed or returned "
			"non-zero: %d \n", returnCode);
	}
}

// Helper function for digital_clock
void display_time(char *timezone) {
    time_t currentTime;
    struct tm *timeInfo;

    if (timezone != NULL) {
        setenv("TZ", timezone, 1);
        tzset();
    }

    time(&currentTime);
    timeInfo = localtime(&currentTime);

    int hours = timeInfo->tm_hour;
    int minutes = timeInfo->tm_min;
    int seconds = timeInfo->tm_sec;

	
    if (timezone != NULL) {
            printf("     Clock  for %s timezone \n",timezone);

    } else {
    	printf("     Clock  for local time   \n");
	}	
    printf("======================================\n");
    for (int row = 0; row < DIGIT_HEIGHT; row++) {
    
        printf("%s  %s  ", largeDigits[hours / 10][row], largeDigits[hours % 10][row]);

        if (row == 2 || row == 3){
            printf(": ");
        } else{
            printf("  ");
        }

        printf("%s  %s  ", largeDigits[minutes / 10][row], largeDigits[minutes % 10][row]);
        
        if (row == 2 || row == 3){
            printf(": ");
        } else{
            printf("  ");
        }

        printf("%s  %s ", largeDigits[seconds / 10][row], largeDigits[seconds % 10][row]);

        printf("\n");
    }
    printf("======================================\n");
    printf("Press e or E to exit\n");

}

/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t *command) {
	int i = 0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background ? "yes" : "no");
	printf("\tNeeds Auto-complete: %s\n",
		   command->auto_complete ? "yes" : "no");
	printf("\tRedirects:\n");

	for (i = 0; i < 3; i++) {
		printf("\t\t%d: %s\n", i,
			   command->redirects[i] ? command->redirects[i] : "N/A");
	}

	printf("\tArguments (%d):\n", command->arg_count);

	for (i = 0; i < command->arg_count; ++i) {
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	}

	if (command->next) {
		printf("\tPiped to:\n");
		print_command(command->next);
	}
}

/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command) {
	if (command->arg_count) {
		for (int i = 0; i < command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}

	for (int i = 0; i < 3; ++i) {
		if (command->redirects[i])
			free(command->redirects[i]);
	}

	if (command->next) {
		free_command(command->next);
		command->next = NULL;
	}

	free(command->name);
	free(command);
	return 0;
}

/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt() {
	char cwd[1024], hostname[1024];
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}

/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command) {
	const char *splitters = " \t"; // split at whitespace
	int index, len;
	len = strlen(buf);

	// trim left whitespace
	while (len > 0 && strchr(splitters, buf[0]) != NULL) {
		buf++;
		len--;
	}

	while (len > 0 && strchr(splitters, buf[len - 1]) != NULL) {
		// trim right whitespace
		buf[--len] = 0;
	}

	// auto-complete
	if (len > 0 && buf[len - 1] == '?') {
		command->auto_complete = true;
	}

	// background
	if (len > 0 && buf[len - 1] == '&') {
		command->background = true;
	}

	char *pch = strtok(buf, splitters);
	if (pch == NULL) {
		command->name = (char *)malloc(1);
		command->name[0] = 0;
	} else {
		command->name = (char *)malloc(strlen(pch) + 1);
		strcpy(command->name, pch);
	}

	command->args = (char **)malloc(sizeof(char *));

	int redirect_index = -1;
	int arg_index = 0;
	char temp_buf[1024], *arg;
	int idxs[3] = {-1, -1, -1};
	int redirection_order = 0;
	memcpy(command->redirect_idxs, idxs, sizeof(command->redirect_idxs));
	while(1) {
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (redirect_index != -1) {
			command->redirect_files[redirect_index] = malloc(strlen(pch));
			strcpy(command->redirect_files[redirect_index], pch);
			redirect_index = -1;
			continue;
		}
		if (!pch)
			break;
		arg = temp_buf;
		strcpy(arg, pch);
		len = strlen(arg);

		// empty arg, go for next
		if (len == 0) {
			continue;
		}

		// trim left whitespace
		while (len > 0 && strchr(splitters, arg[0]) != NULL) {
			arg++;
			len--;
		}

		// trim right whitespace
		while (len > 0 && strchr(splitters, arg[len - 1]) != NULL) {
			arg[--len] = 0;
		}

		// empty arg, go for next
		if (len == 0) {
			continue;
		}

		// piping to another command
		if (strcmp(arg, "|") == 0) {
			struct command_t *c = malloc(sizeof(struct command_t));
			int l = strlen(pch);
			pch[l] = splitters[0]; // restore strtok termination
			index = 1;
			while (pch[index] == ' ' || pch[index] == '\t')
				index++; // skip whitespaces

			parse_command(pch + index, c);
			pch[l] = 0; // put back strtok termination
			command->next = c;
			continue;
		}

		// background process
		if (strcmp(arg, "&") == 0) {
			// handled before
			continue;
		}

		// handle input redirection
		
		if (arg[0] == '<') {
			redirect_index = 0;
		}

		if (arg[0] == '>') {
			if (len > 1 && arg[1] == '>') {
				redirect_index = 2;
				len--;
			} else {
				redirect_index = 1;
			}
		}

		if (redirect_index != -1) {
			//printf("redirect idx: %d, len: %d, arg: %s ", redirect_index, len, arg+1);
			command->redirects[redirect_index] = malloc(len);
			command->redirect_idxs[redirection_order++] = redirect_index;
			strcpy(command->redirects[redirect_index], arg + 1);
			continue;
		}

		// normal arguments
		if (len > 2 &&
			((arg[0] == '"' && arg[len - 1] == '"') ||
			 (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
		{
			arg[--len] = 0;
			arg++;
		}

		command->args =
			(char **)realloc(command->args, sizeof(char *) * (arg_index + 1));

		command->args[arg_index] = (char *)malloc(len + 1);
		strcpy(command->args[arg_index++], arg);
		redirect_index = -1;
	}
	command->arg_count = arg_index;

	// increase args size by 2
	command->args = (char **)realloc(
		command->args, sizeof(char *) * (command->arg_count += 2));

	// shift everything forward by 1
	for (int i = command->arg_count - 2; i > 0; --i) {
		command->args[i] = command->args[i - 1];
	}

	// set args[0] as a copy of name
	command->args[0] = strdup(command->name);

	// set args[arg_count-1] (last) to NULL
	command->args[command->arg_count - 1] = NULL;
	command->arg_count -= 1;
	return 0;
}

void prompt_backspace() {
	putchar(8); // go back 1
	putchar(' '); // write empty over
	putchar(8); // go back 1 again
}

/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command) {
	size_t index = 0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;
	// ICANON normally takes care that one line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &=
		~(ICANON |
		  ECHO); // Also disable automatic echo. We manually echo each char.
	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	show_prompt();
	buf[0] = 0;

	while (1) {
		c = getchar();
		//printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		// handle tab
		if (c == 9) {
			buf[index++] = '?'; // autocomplete
			break;
		}

		// handle backspace
		if (c == 127) {
			if (index > 0) {
				prompt_backspace();
				index--;
			}
			continue;
		}

		if (c == 27 || c == 91 || c == 66 || c == 67 || c == 68) {
			continue;
		}

		// up arrow
		if (c == 65) {
			while (index > 0) {
				prompt_backspace();
				index--;
			}

			char tmpbuf[4096];
			printf("%s", oldbuf);
			strcpy(tmpbuf, buf);
			strcpy(buf, oldbuf);
			strcpy(oldbuf, tmpbuf);
			index += strlen(buf);
			continue;
		}

		putchar(c); // echo the character
		buf[index++] = c;
		if (index >= sizeof(buf) - 1)
			break;
		if (c == '\n') // enter key
			break;
		if (c == 4) // Ctrl+D
			return EXIT;
	}

	// trim newline from the end
	if (index > 0 && buf[index - 1] == '\n') {
		index--;
	}

	// null terminate string
	buf[index++] = '\0';

	strcpy(oldbuf, buf);

	/* before parse command, replacing aliases with the original commands */
	char* temp_buf;
	temp_buf = check_for_aliases(buf);

	parse_command(temp_buf, command);

	// print_command(command); // DEBUG: uncomment for debugging

	// restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
	return SUCCESS;
}

int process_command(struct command_t *command);

// Helper function for digital_clock to exit in Shellect
int kbhit(void) {
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF) {
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}
int main() {
	while (1) {
		struct command_t *command = malloc(sizeof(struct command_t));

		// set all bytes to 0
		memset(command, 0, sizeof(struct command_t));

		int code;
		code = prompt(command);

		if (code == EXIT) {
			break;
		}

		code = process_command(command);
		if (code == EXIT) {
			break;
		}

		free_command(command);
	}

	// If kernel module is loaded remove it
	if(kernel_module_loaded == 1) {
		system("sudo rmmod mymodule.ko");
	}
	printf("\n");
	return 0;
}

int process_command(struct command_t *command) {
	int r;

	if (strcmp(command->name, "") == 0) {
		return SUCCESS;
	}

	if (strcmp(command->name, "exit") == 0) {
		return EXIT;
	}

	if (strcmp(command->name, "repl") == 0) {
		if (command->arg_count == 5) {
			char* text = read_from_file(command->args[3]);
			char* orig = command->args[1];
			char* rep = command->args[2];
			char* loc = strstr(text,orig);
			while (loc != NULL) {
				text = replace(text, orig, rep, loc);
				loc = strstr(text,orig);
			}
			FILE* fp = fopen(command->args[4], "w");
			fprintf(fp, "%s", text);
			fclose(fp);
		} else {
			printf("-%s: %s: %s\n", sysname, command->name, "Wrong number of parameters!");
		}
		return SUCCESS;
	}
	
	
	//Barış's custom command
	if (strcmp(command->name, "digital_clock") == 0) {
		if (command->arg_count == 1) {
			while (1) {
				
				display_time(NULL); // Use local time if no timezone is specified

				if(kbhit()){
					char input = getchar();
					if (input == 'e' || input == 'E') {
						return SUCCESS; // Exit the program
					}
				}
				
				sleep(1);
				printf("\033[2J\033[1;1H"); // Clear the console (works on some systems)
			}
		} else if (command->arg_count == 2) {
			while (1) {
				display_time(command->args[1]);
		        
				if(kbhit()){
					char input = getchar();
					if (input == 'e' || input == 'E') {
						return SUCCESS; // Exit the program
					}
				}

				sleep(1);
				printf("\033[2J\033[1;1H"); // Clear the console (works on some systems)
			}

		}else {
			printf("-%s: %s: %s\n", sysname, command->name, "Wrong number of parameters!");
		}
		return SUCCESS;
	}
	if (strcmp(command->name, "cd") == 0) {
		if (command->arg_count > 0) {
			if (command->arg_count == 1) {
				chdir(getenv("HOME"));
			} else {
				r = chdir(command->args[1]);
				if (r == -1) {
					printf("-%s: %s: %s\n", sysname, command->name,
						strerror(errno));
				}
			}
			
			return SUCCESS;
		}
	}
	// Part3
	if (strcmp(command->name, "xdd") == 0) {
		if (command->arg_count == 1) { // only xdd is prompted
			char str[1024];
			printf("Enter a string to group:\n"); // get a string with stdin
			scanf("%s", str);
			print_xdd(str, 2); // default group_size is 2
		} else if (command->arg_count == 2) { // xdd and filename is prompted
			char* text = read_from_file(command->args[1]);
			print_xdd(text, 2); // default group_size is 2
		} else if (command->arg_count == 4) { // xdd -g group_size filename
			int group_size = atoi(command->args[2]);
			char* text = read_from_file(command->args[3]);
			print_xdd(text, group_size);
		} else {
			printf("-%s: %s: %s\n", sysname, command->name, "Wrong number of parameters!");
		}
		return SUCCESS;
	}

	if (strcmp(command->name, "alias") == 0) { // this part adds aliases and stores them in "alias.txt" so that they can be accessed in other shell runs
		FILE* fp = fopen("alias.txt", "a"); // appending because alias.txt could previously exist
		char comm[100];
		for(int i=2; i<command->arg_count; i++) { // concetanating the args from 2 until the end, this is what alias represents
			if(i==2) {
				strcpy(comm, command->args[i]);
				strcat(comm, " ");
			} else if (i != command->arg_count - 1) {
				strcat(comm, command->args[i]);
				strcat(comm, " ");
			} else {
				strcat(comm, command->args[i]);
			}
		}
		fprintf(fp,"%s;%s\n", command->args[1], comm); // write to file as {alias};{command}
		fclose(fp);
		return SUCCESS;
	}

	// Part 3-c
	if (strcmp(command->name, "good_morning") == 0) {
		if (command->arg_count != 3) {
			printf("-%s: %s: %s \n", sysname, command->name, "Wrong number of parameters!");		
		}else {
			good_morning(command->args[1],command->args[2]);
		}
		return SUCCESS;

	} 

	// Part 4
	if (strcmp(command->name, "psvis") == 0) {
		int pid = atoi(command->args[1]);
		char* filename = command->args[2];
		char buffer[1024];
		chdir("module");
		snprintf(buffer, sizeof(buffer), "sudo dmesg -C ; sudo insmod mymodule.ko my_pid=%d ; sudo dmesg > tree.txt", pid);
		system(buffer);
		kernel_module_loaded = 1;
		snprintf(buffer, sizeof(buffer), "python3 plot.py %s", filename);
		system(buffer);
		chdir("..");
	}


	pid_t pid = fork();
	// child
	if (pid == 0) {
		//Part 2
		for(int i=0; i<3; i++){
			if (command->redirect_idxs[i] != -1) {
				int rdrct = command->redirect_idxs[i];
				if (rdrct == 0) {
					int fd = open(command->redirect_files[0], O_RDONLY); // open the file for reading
					if (fd == -1) {
						printf("-%s: %s: %s \n", sysname, command->redirects[0], strerror(errno));
						exit(1);
					}	
					dup2(fd, STDIN_FILENO); // redirect stdin to the file
					close(fd); 
				} else if (rdrct == 1) {
					int fd = open(command->redirect_files[1], O_WRONLY | O_CREAT | O_TRUNC, 0644); // open the file or create it if it does not exist
					if (fd == -1) {
						printf("-%s: %s: %s \n", sysname, command->redirects[0], strerror(errno));
						exit(1);
					}
					dup2(fd, STDOUT_FILENO); // redirect stdout to the file
					close(fd); 
				} else if (rdrct == 2) {
					int fd = open(command->redirect_files[2], O_WRONLY | O_CREAT | O_APPEND, 0644); // open the file or create it if it does not exist
					if (fd == -1) {
						printf("-%s: %s: %s \n", sysname, command->redirects[0], strerror(errno));
						exit(1);
					}
					dup2(fd, STDOUT_FILENO); // redirect stdout to the file
					close(fd); 
				}					
			}
		}
		/// This shows how to do exec with environ (but is not available on MacOs)
		// extern char** environ; // environment variables
		// execvpe(command->name, command->args, environ); // exec+args+path+environ

		/// This shows how to do exec with auto-path resolve
		// add a NULL argument to the end of args, and the name to the beginning
		// as required by exec

		// TODO: do your own exec with path resolving using execv()
		// do so by replacing the execvp call below
		// execvp(command->name, command->args); // exec+args+path
		
		//Part 1
		char *path = getenv("PATH");
		char cwd[256];
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
			strcat(path, ":");
			strcat(path,cwd);
		}
		char *token = strtok(path, ":");
		
		while (token != NULL) {
			char *path_to_exec = malloc(strlen(token) + strlen(command->name) + 2);
			strcpy(path_to_exec, token); // copy token to path_to_exec
			strcat(path_to_exec, "/");
			strcat(path_to_exec, command->name);
			execv(path_to_exec, command->args);
			free(path_to_exec);
			token = strtok(NULL, ":"); // get next token
		}
		exit(0);
	} else {
		// TODO: implement background processes here
		// if not background process, wait for child process to finish
		if (!command->background)
			waitpid(pid, NULL, 0);
		else
			printf("-%s: %s: background process started\n", sysname,
				   command->name);
		return SUCCESS;
	}

	// TODO: your implementation here

	printf("-%s: %s: command not found\n", sysname, command->name);
	return UNKNOWN;
}
