// Lawrence Lim
// Perm: 4560892

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <ctype.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <iostream>
#include <fcntl.h>    /* For O_RDWR */

#define STD_INPUT 0
#define STD_OUTPUT 1

using namespace std;


void trim(string& s)
{
   size_t p = s.find_first_not_of(" \t");
   s.erase(0, p);

   p = s.find_last_not_of(" \t");
   if (string::npos != p)
      s.erase(p+1);
}


int main(int argc, char *argv[]) {
    bool dont_show_shell = 0;
    //cout << argv[1];
    string flag ("-n");
    if (argc >= 2 && flag.compare (argv [1]) == 0) {
        dont_show_shell = 1;
    }
    
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        if (!dont_show_shell) {
            cout << "shell: ";
	}
	string input;
	cin >> input;
        trim (input);
        bool background_process = (input[input.length()-1] == '&');

	if (background_process) {
            input.erase(input.length()-1);
	    trim (input);
	}

        
        int pid = fork();
        
        if (pid == 0) {
            // I am the child
	    //
	    // First, handle pipes
	    int is_piped = -1;
            std::size_t found;
            do {
	        found = input.find_last_of ("|");
                if (found != string::npos) {
                    int fd [2];
		    pipe (&fd[0]);
		    
                    int pid2 = fork();
		    if (pid2 == 0) {
                        close (fd[0]);
			close (STD_OUTPUT);
			dup (fd[1]);
			close (fd[1]);
			input = input.substr(0, found);
			is_piped = 0;
		    }
		    else {
			close (fd[1]);
			close (STD_INPUT);
			dup (fd[0]);
			close (fd[0]);
                        input = input.substr(found+1);
			is_piped = 1;
		    }
		    trim (input);
		}
	    } while (found != string::npos);
            
	    // Then, handle redirections and files
	    //
            found = input.find (">");
	    if (found != string::npos) {
                if (is_piped == 0) {
                    fprintf(stderr, "ERROR: Has both output redirect and pipe");
		    return 0;
		}

                string output = input.substr (found+1);
		trim (output);
                
		int file_descripter = open (output.c_str(), O_WRONLY | O_CREAT);
                close (STD_OUTPUT);
		dup (file_descripter);
		close (file_descripter);

		input = input.substr (0, found);
	        trim (input);	
	    }
            

	    found = input.find ("<");
	    if (found != string::npos) {
                if (is_piped == 1) {
                    fprintf (stderr, "ERROR: Has both input redirect and pipe");
		    return 0;
		}

                string input_file_name = input.substr (found+1);
		trim (input_file_name);

		int file_descripter = open (input_file_name.c_str(), O_RDONLY);
		close (STD_INPUT);
		dup (file_descripter);
		close (file_descripter);

	        input = input.substr (0, found);
		trim (input);
	    }


	    // Finally execute the command
	    //fprintf (stdin, input);
            size_t n = std::count(input.begin(), input.end(), ' ');             
            char *args[n+2];
	    
            char * inputarr = new char [input.length()+1];
            std::strcpy (inputarr, input.c_str());

	    char *pch = strtok (inputarr, " ");
	    int count = 0;
	    args[count] = pch;
	    count ++;
	    fprintf (stdin, args [count]);
            while (pch != NULL) {
                pch = strtok (NULL, " ");
	        args[count] = pch;
		fprintf (stdin, args[count]);
	        count ++;	
	    }
	    args [count] = 0;

	    execvp (args[0], args);
	    
	    perror ("ERROR");
	    return 0; 
	}
	else {
            // I am the parent
	    if (!background_process) {
                wait (NULL);
	    }
	}
    }
    return 0;
}
