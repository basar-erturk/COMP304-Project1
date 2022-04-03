#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h> //termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_BLACK   "\x1b[30m"
#define ANSI_COLOR_WHITE   "\x1b[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"


const char *sysname = "shellfyre";

enum return_codes
{
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};

struct command_t
{
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3];		// in/out redirection
	struct command_t *next; // for piping
};

/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t *command)
{
	int i = 0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background ? "yes" : "no");
	printf("\tNeeds Auto-complete: %s\n", command->auto_complete ? "yes" : "no");
	printf("\tRedirects:\n");
	for (i = 0; i < 3; i++)
		printf("\t\t%d: %s\n", i, command->redirects[i] ? command->redirects[i] : "N/A");
	printf("\tArguments (%d):\n", command->arg_count);
	for (i = 0; i < command->arg_count; ++i)
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	if (command->next)
	{
		printf("\tPiped to:\n");
		print_command(command->next);
	}
}

/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command)
{
	if (command->arg_count)
	{
		for (int i = 0; i < command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}
	for (int i = 0; i < 3; ++i)
		if (command->redirects[i])
			free(command->redirects[i]);
	if (command->next)
	{
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
int show_prompt()
{
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
int parse_command(char *buf, struct command_t *command)
{
	const char *splitters = " \t"; // split at whitespace
	int index, len;
	len = strlen(buf);
	while (len > 0 && strchr(splitters, buf[0]) != NULL) // trim left whitespace
	{
		buf++;
		len--;
	}
	while (len > 0 && strchr(splitters, buf[len - 1]) != NULL)
		buf[--len] = 0; // trim right whitespace

	if (len > 0 && buf[len - 1] == '?') // auto-complete
		command->auto_complete = true;
	if (len > 0 && buf[len - 1] == '&') // background
		command->background = true;

	char *pch = strtok(buf, splitters);
	command->name = (char *)malloc(strlen(pch) + 1);
	if (pch == NULL)
		command->name[0] = 0;
	else
		strcpy(command->name, pch);

	command->args = (char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index = 0;
	char temp_buf[1024], *arg;

	while (1)
	{
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch)
			break;
		arg = temp_buf;
		strcpy(arg, pch);
		len = strlen(arg);

		if (len == 0)
			continue;										 // empty arg, go for next
		while (len > 0 && strchr(splitters, arg[0]) != NULL) // trim left whitespace
		{
			arg++;
			len--;
		}
		while (len > 0 && strchr(splitters, arg[len - 1]) != NULL)
			arg[--len] = 0; // trim right whitespace
		if (len == 0)
			continue; // empty arg, go for next

		// piping to another command
		if (strcmp(arg, "|") == 0)
		{
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
		if (strcmp(arg, "&") == 0)
			continue; // handled before

		// handle input redirection
		redirect_index = -1;
		if (arg[0] == '<')
			redirect_index = 0;
		if (arg[0] == '>')
		{
			if (len > 1 && arg[1] == '>')
			{
				redirect_index = 2;
				arg++;
				len--;
			}
			else
				redirect_index = 1;
		}
		if (redirect_index != -1)
		{
			command->redirects[redirect_index] = malloc(len);
			strcpy(command->redirects[redirect_index], arg + 1);
			continue;
		}

		// normal arguments
		if (len > 2 && ((arg[0] == '"' && arg[len - 1] == '"') || (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
		{
			arg[--len] = 0;
			arg++;
		}
		command->args = (char **)realloc(command->args, sizeof(char *) * (arg_index + 1));
		command->args[arg_index] = (char *)malloc(len + 1);
		strcpy(command->args[arg_index++], arg);
	}
	command->arg_count = arg_index;
	return 0;
}

void prompt_backspace()
{
	putchar(8);	  // go back 1
	putchar(' '); // write empty over
	putchar(8);	  // go back 1 again
}

/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command)
{
	int index = 0;
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
	new_termios.c_lflag &= ~(ICANON | ECHO); // Also disable automatic echo. We manually echo each char.
	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	// FIXME: backspace is applied before printing chars
	show_prompt();
	int multicode_state = 0;
	buf[0] = 0;

	while (1)
	{
		c = getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		if (c == 9) // handle tab
		{
			buf[index++] = '?'; // autocomplete
			break;
		}

		if (c == 127) // handle backspace
		{
			if (index > 0)
			{
				prompt_backspace();
				index--;
			}
			continue;
		}
		if (c == 27 && multicode_state == 0) // handle multi-code keys
		{
			multicode_state = 1;
			continue;
		}
		if (c == 91 && multicode_state == 1)
		{
			multicode_state = 2;
			continue;
		}
		if (c == 65 && multicode_state == 2) // up arrow
		{
			int i;
			while (index > 0)
			{
				prompt_backspace();
				index--;
			}
			for (i = 0; oldbuf[i]; ++i)
			{
				putchar(oldbuf[i]);
				buf[i] = oldbuf[i];
			}
			index = i;
			continue;
		}
		else
			multicode_state = 0;

		putchar(c); // echo the character
		buf[index++] = c;
		if (index >= sizeof(buf) - 1)
			break;
		if (c == '\n') // enter key
			break;
		if (c == 4) // Ctrl+D
			return EXIT;
	}
	if (index > 0 && buf[index - 1] == '\n') // trim newline from the end
		index--;
	buf[index++] = 0; // null terminate string

	strcpy(oldbuf, buf);

	parse_command(buf, command);

	// print_command(command); // DEBUG: uncomment for debugging

	// restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
	return SUCCESS;
}

int process_command(struct command_t *command);

int main()
{
	while (1)
	{
		struct command_t *command = malloc(sizeof(struct command_t));
		memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

		int code;
		code = prompt(command);
		if (code == EXIT)
			break;

		code = process_command(command);
		if (code == EXIT)
			break;

		free_command(command);
	}

	printf("\n");
	return 0;
}

int process_command(struct command_t *command)
{
	int r;
	char hist_file[150];

	int writeDirHist(char *hist_file){
		char cwd[150];
		if (strcmp(hist_file,"") == 0){

			getcwd(cwd,sizeof(cwd));
			strcat(hist_file,cwd);
			strcat(hist_file,"/dir_hist.txt");
			strcat(cwd,"\n");
		}
		else {
			getcwd(cwd,sizeof(cwd));
			strcat(cwd,"\n");
		}

		FILE *dirhist=fopen(hist_file,"a");
		fputs(cwd,dirhist);
		fclose(dirhist);
		return SUCCESS;
	}
	if (strcmp(command->name, "") == 0)
		return SUCCESS;

	if (strcmp(command->name, "exit") == 0)
		return EXIT;

	if (strcmp(command->name, "cd") == 0)
	{
		if (command->arg_count > 0)
		{
			writeDirHist(hist_file);
			r = chdir(command->args[0]);
			if (r == -1)
				printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
			return SUCCESS;
		}
	}


	// TODO: Implement your custom commands here


	int filesearch(char *option, char *search, char *ogPath) {
		struct dirent ** fileListTemp;
	
		if (search == NULL) {
			search = option;
		}
			
		//char *path1 = (char*)malloc(100*sizeof(char));
		char path1[300];

		getcwd(path1, sizeof(path1));
	
		int fileNum = scandir(path1, &fileListTemp, NULL, alphasort);

		char *newSearch = strtok(search, "\"");

		for (int i = 0; i < fileNum; i++) {
			if(strstr(fileListTemp[i]->d_name, newSearch) != NULL) { 
				char *token = strstr(path1,ogPath);
				printf(".%s/%s\n",token+strlen(ogPath), fileListTemp[i]->d_name);
			}
			
			char str[2] = "\0";
			str[0] = fileListTemp[i]->d_name[0];

			if(strcmp(option, "-r") == 0 && strcmp(str, ".") != 0) {
				DIR *direc;
				char newPath[300];
				
				getcwd(newPath, sizeof(newPath));
				strcat(newPath, "/");
				strcat(newPath, fileListTemp[i]->d_name);
				
				direc = opendir(newPath);

		
				if(direc != NULL) {
					chdir(newPath);
					if (filesearch(option, search, ogPath) == EXIT)
						return EXIT;
					chdir(path1);
				}
			}
			

		}
		//free(path1);
		return SUCCESS;
	}	

	int take(char *dirName) {
		char pathInfo[300];
		getcwd(pathInfo, sizeof(pathInfo));
		strcat(pathInfo, "/");
		strcat(pathInfo, dirName);
		mkdir(pathInfo, 0777);
		chdir(pathInfo);
		return SUCCESS;
	}


	int colortext( char *color) {
		struct dirent ** fileListTemp;

		char path[300];
		getcwd(path, sizeof(path));
		
		int fileNum = scandir(path, &fileListTemp, NULL, alphasort);
		
		char *newColor = strtok(color, "\"");
		char validColors[100];

		strcat(validColors, "Black ");
		strcat(validColors, "Red ");
		strcat(validColors, "Green ");
		strcat(validColors, "Yellow ");
		strcat(validColors, "Blue ");
		strcat(validColors, "Magenta ");
		strcat(validColors, "Cyan ");
		strcat(validColors, "White");
		strcat(validColors, "Rainbow");

		bool flag = false;


		for (int j = 0; validColors[j]; j++) {
			validColors[j] = tolower(validColors[j]);
		}

		for (int k = 0; newColor[k]; k++) {
			newColor[k] = tolower(newColor[k]);
		}

		if (strstr((char *)validColors, newColor) == NULL) {
			printf("Did not enter a valid color.\n");

			return SUCCESS;
		}

		for (int i = 0; i < fileNum; i++) {
			if(strcmp(newColor, "red") == 0) {
				printf(ANSI_COLOR_RED	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);
				
				if (flag) 
					newColor = "yellow";
					continue;
			}

			if(strcmp(newColor, "green") == 0) {
				printf(ANSI_COLOR_GREEN	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);

				if (flag)
					newColor = "cyan";
					continue;
			}

			if(strcmp(newColor, "yellow") == 0) {
				printf(ANSI_COLOR_YELLOW	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);

				if(flag)
					newColor = "green";
					continue;
			}

			if(strcmp(newColor, "blue") == 0) {
				printf(ANSI_COLOR_BLUE	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);

				if(flag) 
					newColor = "magenta";
					continue;
			}


			if(strcmp(newColor, "magenta") == 0) {
				printf(ANSI_COLOR_MAGENTA	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);
		
				if(flag)
					newColor = "red";
					continue;
			}

			if(strcmp(newColor, "cyan") == 0) {
				printf(ANSI_COLOR_CYAN	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);

				if(flag)
					newColor = "blue";
					continue;
			}

			if(strcmp(newColor, "black") == 0)
				printf(ANSI_COLOR_BLACK	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);

			if(strcmp(newColor, "white") == 0)
				printf(ANSI_COLOR_WHITE	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);

			if(strcmp(newColor, "rainbow") == 0) {
				printf(ANSI_COLOR_RED	"%s"	ANSI_COLOR_RESET	"\n", fileListTemp[i]->d_name);

				flag = true;
				newColor = "yellow";
			}

		}

		return SUCCESS;

	}

	if (strcmp(command->name, "cdh") == 0){

		char cwd[150];
		if (strcmp(hist_file,"") == 0){

			getcwd(cwd,sizeof(cwd));
			strcat(hist_file,cwd);
			strcat(hist_file,"/dir_hist.txt");
			strcat(cwd,"\n");
		}
		else {
			getcwd(cwd,sizeof(cwd));
			strcat(cwd,"\n");
		}

		char history[10][150];
		for (int i = 0; i < sizeof(history)/sizeof(history[0]);i++)
			history[i][0] = '\0';
		int count1 = 0;
		int count2;
		char letter = 'a';
		char buf[150];
	        int cha;	
		FILE *hist = fopen(hist_file, "r");
		fseek(hist, 0, SEEK_END);
		while (ftell(hist) > 1 && count1 < 10){
			fseek(hist, -2, SEEK_CUR);
			if (ftell(hist)<=2)
				break;
			
			cha = fgetc(hist);
			count2 = 0;

			while(cha != '\n'){
				buf[count2] = cha;
				count2++;
				if (ftell(hist) < 2)
					break;
				fseek(hist, -2, SEEK_CUR);
				cha = fgetc(hist);
			}
			printf("%d  %c   ",count1, letter);
			for (int i = count2-1; i >= 0 && count2 > 0; i--){
				printf("%c", buf[i]);
				strncat(history[count1], &buf[i], 1);
			}
			strncat(history[count1], "\0", 1);
			printf("\n");
			count1++;
			letter++;
		}
		fclose(hist);

		int fd[2];

		if (pipe(fd) == -1)
			fprintf(stderr,"Pipe failed");

		
		pid_t pid = fork();

		char chosenDir='f';

		if (pid == 0){
			close(fd[0]);
			printf("Select directory by letter or number: ");
			scanf("%c", &chosenDir);
			if (chosenDir > 60){
				chosenDir -= 97;
			}
			else {
				chosenDir -= 48;
			}


			char chosenDirstr[10];
			snprintf(chosenDirstr,9,"%d",chosenDir);
			write(fd[1], chosenDirstr, strlen(history[chosenDir]+1));
			exit(0);
		}
		else{
			wait(NULL);
			char dirstr[10]="";
			close(fd[1]);
			read(fd[0], dirstr, 10);
			chosenDir = atoi(dirstr);	
			writeDirHist(hist_file);
			chdir(history[chosenDir]);
			return SUCCESS;
		}
		
	}



	// Basar custom command
	if (strcmp(command->name, "wfile") == 0){
		char text[100]="";
		FILE *file = fopen(command->args[0],"w");
		int i = 1;
		while (command->args[i] != NULL){
			strcat(text,command->args[i]);
			strcat(text," ");
			i++;
		}
		strcat(text,"\n");
		fputs(text,file);
		fclose(file);
		return SUCCESS;
	}
	

	if (strcmp(command->name, "filesearch") == 0) {
		char ogPath[120];
		
		getcwd(ogPath, sizeof(ogPath));
		int file = filesearch(command->args[0], command->args[1], ogPath);
		if (file == EXIT)
			return EXIT;

		return SUCCESS;
	}

	if (strcmp(command->name, "take") == 0) {
		char *token = strtok(command->args[0], "/");
		/*if (strmp(cwd,"") == 0){

			getcwd(cwd,sizeof(cwd));
			strcat(hist_path, cwd);
			strcat(hist_path,"/dir_hist.txt");
			strcat(cwd, "\n");
		}
		else{
			getcwd(cwd,sizeof(cwd));
			strcat(cwd,"\n");
		}
		*/
		writeDirHist(hist_file);
		while (token != NULL) {
		int takeCom = take(token);

		token = strtok(NULL, "/");

		}
		return SUCCESS;	
	}

	if (strcmp(command->name, "joker") == 0) {
		char pathJok[150] = "";
                        getcwd(pathJok, sizeof(pathJok));

                        strcat(pathJok, "/");
                        strcat(pathJok, "joker.sh");

                        char commJok[150] = "";

                        strcat(commJok, "echo \"* * * * * ");
                        strcat(commJok, pathJok);
                        strcat(commJok, " \" | crontab -");


                 system(commJok);
		
		return SUCCESS;
	}


	if (strcmp(command->name, "colortext") == 0) {
		int colortxt = colortext(command->args[0]);

		return SUCCESS;
	}



	pid_t pid = fork();

	if (pid == 0) // child
	{
		// increase args size by 2
		command->args = (char **)realloc(
			command->args, sizeof(char *) * (command->arg_count += 2));

		// shift everything forward by 1
		for (int i = command->arg_count - 2; i > 0; --i)
			command->args[i] = command->args[i - 1];

		// set args[0] as a copy of name
		command->args[0] = strdup(command->name);
		// set args[arg_count-1] (last) to NULL
		command->args[command->arg_count - 1] = NULL;

		/// TODO: do your own exiec with path resolving using execv()
		char path[300]="";
		strcat(path, "/bin/");

		strcat(path, command->name);
		char *comm[] = {path, command->args[1], command->args[2], command->args[3], command->args[4], command->args[5], command->args[6], command->args[7], NULL};
		if (execv(path, comm) == -1){
			 printf("-%s: %s: command not found\n", sysname, command->name);
		}
		exit(0);
	}
	else
	{
		/// TODO: Wait for child to finish if command is not running in background
		
		wait(NULL);
		return SUCCESS;
	}

	printf("-%s: %s: command not found\n", sysname, command->name);
	return UNKNOWN;
}
