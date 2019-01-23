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
#include <fcntl.h>  

#define STD_INPUT 0
#define STD_OUTPUT 1
#define STD_ERROR 2

using namespace std;

int error_fd;

void trim(string& s) {
    size_t p = s.find_first_not_of(" \t\n");
    s.erase(0, p);
    
    p = s.find_last_not_of(" \t\n");
    if (string::npos != p)
        s.erase(p+1);
}

void signal_handler (int signal) {
    int status;
    wait (&status);
    int i = WEXITSTATUS (status);
    //cout << "HERE2 \n" << i  << "\n";
    if (WIFEXITED (status) && WEXITSTATUS (status) != 0) {
        //fprintf (stderr, "ERROR2: %d", WEXITSTATUS (status));
        char buf [512];
        ssize_t i = read (::error_fd, buf, 512);
        //fprintf (stderr, "%ld", i);
        if (i > 0) {
            
            string msg (buf);
            msg = msg.substr (0, i);
            replace (msg.begin(), msg.end(), '\n', ' ');
            fprintf (stderr, "ERROR: %s\n", msg.c_str());
            fflush (stderr);
        }
    }
}

void signal_handler2 (int signal) {
    int status;
    wait (&status);
    int i = WEXITSTATUS (status);
    //cout << "HERE3 \n" << i  << "\n";
    exit (i);
}

int main(int argc, char *argv[]) {
    bool dont_show_shell = 0;
    string flag ("-n");
    if (argc >= 2 && flag.compare (argv [1]) == 0) {
        dont_show_shell = 1;
    }
    
    int fd2 [2];
    pipe (&fd2[0]);
    ::error_fd = fd2[0];
    signal(SIGCHLD, signal_handler);
    
    if (!dont_show_shell) {
        cout << "shell: ";
    }
    char in [513];
    while (std::cin.getline (in, 512)) {
        string input (in);
        trim (input);
        bool background_process = (input.length() > 0 && input[input.length()-1] == '&');
        
        if (background_process) {
            input.erase(input.length()-1);
            trim (input);
        }
        if (input.length() > 0) {
            size_t n2 = std::count(input.begin(), input.end(), ' ');
            char *args2[n2+2];
            
            char * inputarr2 = new char [input.length()+1];
            std::strcpy (inputarr2, input.c_str());
            
            char *pch2 = strtok (inputarr2, " ");
            int count2 = 0;
            args2[count2] = pch2;
            count2 ++;
            while (pch2 != NULL) {
                pch2 = strtok (NULL, " ");
                args2[count2] = pch2;
                count2 ++;
            }
            args2 [count2] = 0;
            
            string changedir2 ("cd");
            if (changedir2.compare (args2[0]) == 0) {
                int i = chdir (args2 [1]);
                if (i == -1) {
                    fprintf (stderr, "ERROR: No such directory\n");
                }
            }
            else if (fork() == 0) {
                signal (SIGCHLD, signal_handler2);
                close(STD_ERROR);
                close (fd2[0]);
                dup   (fd2[1]);
                close (fd2[1]);
                
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
                            
                            int status;
                            wait (&status);
                            
                            //cout << "HERE2\n";
                            if (WIFEXITED (status) && WEXITSTATUS (status) != 0) {
                                return 1;
                            }
                        }
                        trim (input);
                    }
                } while (found != string::npos);
                
                // Then, handle redirections and files
                //
                std::size_t found2;
                
                found = input.find (">");
                if (found != string::npos) {
                    if (is_piped == 0) {
                        fprintf(stderr, "Has both output redirect and pipe");
                        fflush (stderr);
                        return 1;
                    }
                    
                    string output = input.substr (found+1);
                    
                    found2 = output.find (">");
                    if (found2 != string::npos) {
                        fprintf (stderr, "Has multiple output redirect");
                        fflush (stderr);
                        return 1;
                    }
                    
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
                        fprintf (stderr, "Has both input redirect and pipe");
                        fflush (stderr);
                        return 1;
                    }
                    found = input.find ("<");

                    string input_file_name = input.substr (found+1);
                    
                    found2 = input_file_name.find ("<");
                    if (found2 != string::npos) {
                        fprintf (stderr, "Has multiple input redirect");
                        fflush (stderr);
                        return 1;
                    }
                    
                    trim (input_file_name);
                    
                    int file_descripter = open (input_file_name.c_str(), O_RDONLY);
                    //perror ("ERROR");
                    close (STD_INPUT);
                    dup (file_descripter);
                    close (file_descripter);
                    
                    input = input.substr (0, found);
                    trim (input);
                }
                
                
                // Finally execute the command
                size_t n = std::count(input.begin(), input.end(), ' ');
                char *args[n+2];
                
                char * inputarr = new char [input.length()+1];
                std::strcpy (inputarr, input.c_str());
                
                char *pch = strtok (inputarr, " ");
                int count = 0;
                args[count] = pch;
                count ++;
                while (pch != NULL) {
                    pch = strtok (NULL, " ");
                    args[count] = pch;
                    count ++;
                }
                args [count] = 0;
                
                execvp (args[0], args);
                perror ("");
                exit (errno);
                //perror ("ERROR");
                return 0;
            }
            else {
                // I am the parent
                if (!background_process) {
                    int status;
                    wait (&status);
                    int i = WEXITSTATUS (status);
                    //cout << "HERE1 \n";
                    //cout << i  << "\n";
                    if (WIFEXITED (status) && WEXITSTATUS (status) != 0) {
                        //fprintf (stderr, "ERROR1: %d", WEXITSTATUS (status));
                        char buf [512];
                        ssize_t i = read (::error_fd, buf, 512);
                        if (i > 0) {
                            string msg (buf);
                            replace (msg.begin(), msg.end(), '\n', ' ');
                            fprintf (stderr, "ERROR: %s\n", msg.c_str());
                        }
                    }
                }
            }
        }
        
        if (!dont_show_shell) {
            cout << "shell: ";
        }
    }
    return 0;
}
