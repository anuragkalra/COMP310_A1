#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>

#define MaxHist 10 

struct job{
	int id_number;
	char* line;
};

int children[10];
int childcounter = 0;

//IMPLEMEMENTED METHOD TO CHECK STRING AND VERIFY IF IT DOES NOT CONTAIN ANY NON-DIGITS
//A NUMERICAL VALUE TELLS US TO GO TO OUR HISTORY IMPLEMENTATION

int isnumeric(char *tmp) {
	int j, isDigit;
	j=0;
	while(j<strlen(tmp)-1){
		isDigit = isdigit(tmp[j]);
		/* printf("\nchar=%c isDigit=%d", tmp[j], isDigit); */
		if (isDigit == 0) return(0);
		j++;
	}
	return(1);
}


int getcmd(char *prompt, char *args[], int *background){
    int length;
	char *line;
    size_t linecap = 0;
	int parm_cnt;
	char *line_bkup; //WE KEEP A STRING CALLED LINE_BKUP TO KEEP A SAVED COPY OF THE LINE

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

	// printf("\nline=%s", line); 

	line_bkup = malloc(sizeof(char) * strlen(line)); //WE ALLOCATE FOR SIZEOF CHAR * LENGTH OF LINE
	strcpy(line_bkup, line);

    if (length <= 0) {
        exit(-1);
    }

	parm_cnt = parse_cmd(line, args, background);
	args[parm_cnt + 1] = line_bkup;
	// printf("\nline_bkup=%s", line_bkup); 

	return (parm_cnt);

}


//I HAVE IMPLEMENTED A PARSE_CMD FUNCTION THAT BREAKS THE GET_CMD INTO A SIMPLER INTERFACE
int parse_cmd(char *line_orig, char *args[], int *background) {
	char *token, *loc;
	int i = 0;
	char *line;
	line = malloc(sizeof(char) * strlen(line_orig));
	strcpy(line, line_orig);


    // Check if background is specified..
	if ((loc = index(line, '&')) != NULL) {
    	*background = 1;
    	*loc = ' ';
    } else {
        *background = 0;
    }

    while ((token = strsep(&line, " \t\n")) != NULL) {
		// printf("token=%s.", token);
		int j;
        for (j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }

	args[i] = '\0';
	// printf("Number of param = %d",i);  
    return i;
}


int main() {

    char *args[20];
    int bg;
    int cnt;
	char *line_cmd[1];
	char *line;

	char *hist[MaxHist];
	int arr_start = 0;
	int hist_start = -1;
	int hist_end = -1;

	while(1) {
		bg = 0;
 		cnt = getcmd("\n>>  ", args, &bg);
		// printf("\nline in main=%s",args[cnt+1]); */
		line = args[cnt+1];
		// printf("\nline in main=%s", line); */


		/*
		printf("\ncnt=%d", cnt);
		int i;
		for(i=0; i < cnt; i++) {
			printf("\ni=%d",i);
			printf("\nArg[%d] = %s", i, args[i]);
			printf("\nending i=%d",i);
		}
		*/


		if (cnt == -1) {
			printf("\ngetcmd error. parent ending");
			exit(0);
		}
		if(cnt == 0) {
			printf("\nEmpty command, looping back");
		}
		if(cnt > 0) {
			
			/* if arg[0] is numeric
				if between hist_start and end then 
					translate argv[0] to array position in hist array tran_arr_pos	
					call the parse command with hist[tran_arr_pos] and get args and bg variable back
					follow the logi for fork (current logic)
				else
					history number out of range
					DO NOT fork, just loop back in while
			else 
				current logic
			*/


			int run_fork = 0; //RUN_FORK FLAG

			if(isnumeric(line)) {
				// run from history
				// printf("\nNumeric"); 
				
				int hist_num;
				hist_num = atoi(line);
				// printf("\nNumber is %d", hist_num); */
				if (hist_num >= hist_start & hist_num <= hist_end) {
					line = hist[hist_num - hist_start];
					// printf("\nin range %s", line);
					parse_cmd(line, args, &bg);
					run_fork = 1;
				}
				else {
					printf("\nno command found in history");
					run_fork = 0;
				}

			}
			else {
				// run internal command (cd or pwd) or do execvp
				// printf("\nNon Numeric");
				if (strcmp(args[0], "pwd") == 0) {
					run_fork = 0;
					char pwd_string[100];

					if (getcwd(pwd_string, 100) == NULL) {
						printf("\npwd returned larger than 100 bytes string");
					}
					else {
						printf("\npwd returned %s", pwd_string);
					}
				}

				else if(strcmp(args[0], "exit") == 0) {
					run_fork = 0;
					exit(1);					
				}
				else if(strncmp(args[0], "fg", 2) == 0) {
					run_fork = 0;
					char* number_maybe = &args[0] + 2;
					if(isnumeric(number_maybe)){
						printf("Job number %s moved to foreground", number_maybe);
					}
					else printf("Incorrectly formatted");
				}
				else if(strcmp(args[0], "jobs") == 0){
					int i;
					for(i = 0; i < childcounter; i++){
						printf("Job: %d 	processId: %d\n", i, children[i]);
					}
				}

				else {
					if (strcmp(args[0], "cd") == 0) {
						run_fork = 0;
						int chdir_retcode = chdir(args[1]);
						
						if (chdir_retcode == 0) {
							printf("\nchdir successful");
						}
						else {
							printf("\nchdir failed with return code =%d", chdir_retcode);
						}
					}
					else {
						run_fork = 1;
					}
				}	
			}



			if (run_fork) {

				pid_t childPid = fork();
				if (childPid == 0) {
					int red_flag = 0;
					int fd;
					if(cnt >= 3) {

						int red_cmp = strcmp(args[cnt-2], ">");
						// printf("\nred_cmp=%d", red_cmp);
						if (red_cmp == 0) {
							// printf("\nRedirection");
							// printf("\nasd");
							args[cnt-2] = '\0';
							fd = open(args[cnt-1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
							dup2(fd, 1);
							red_flag = 1;
						}	
					}

					int execCode;
					// printf("\nexecvp starting");
					execCode = execvp(args[0], &args[0]);
					// printf("\nexecvp ending");

					if (red_flag == 1) {
						close(fd);
					}
					exit(0);				
				}
				else {	
					if (bg == 0) {
						int status;
						// printf("\nwaitpid starting %d", childPid); */
						waitpid(childPid, &status, WUNTRACED | WCONTINUED);
						// printf("\nwaitpid ended"); */
					}
					if(bg == 1){
						children[childcounter] = childPid;
						childcounter++;
					}
					if (hist_start == -1) {
						hist_start = 1;
						hist_end = 1;
						hist[arr_start++] = line;
						/*printf("\nsetting first hist %s", line);
						printf("\nhist_arr=%s",hist[arr_start-1]); */
					}
					else {
						if (hist_end < MaxHist) {
							hist_end++;
							hist[arr_start++] = line;
							// printf("\nhist_arr=%s hist_end=%d", hist[arr_start-1], hist_end); */
						}
						else {
							hist_start++;
							hist_end++;
							int h_loop;
							for(h_loop=0; h_loop < MaxHist-1; h_loop++) {
								hist[h_loop] = hist[h_loop+1];
							}
							hist[MaxHist - 1] = line;
						}
					}
				/* printf("\narr_start = %d hist_start =%d hist_end=%d", arr_start, hist_start, hist_end); */
				// int h_prt;
				// for(h_prt=0; h_prt<arr_start; h_prt++) {
				//	printf("\narr=%d hist=%d arr=%s", h_prt, hist_start + h_prt, hist[h_prt]);
				// }


				}
		   }
		}
	}

	/*	int i;
	    for (i = 0; i < cnt; i++)
    	    printf("\nArg[%d] = %s", i, args[i]);

    	if (bg)
        	printf("\nBackground enabled..\n");
   	 else
    	    printf("\nBackground not enabled \n");

    	printf("\n\n");


		int execCode;
		execCode = execvp(args[0], &args[1]);
		printf("execvp rc=%d", execCode);
	*/

}

