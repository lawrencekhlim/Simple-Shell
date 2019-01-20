// Dsed sample codes from October 12th discussion section.
// Lawrence Lim
// Perm: 4560892

/* A simple server in the internet domain using TCP
 The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <ctype.h>


void error(const char *msg) {
    perror(msg);
    exit(1);
}



int check_processes (int* pids, int num_alive) {
    for (int i = num_alive-1; i >= 0; i--) {
        int status;
        //printf ("i = %d\n", i);
        //fflush (stdout);
        pid_t result = waitpid (pids[i], &status, WNOHANG);
        if (result > 0) {
            //printf ("End for i = %d\n", i);
            //fflush (stdout);
            pids [i] = pids [num_alive-1];
            pids [num_alive-1] = 0;
            num_alive --;
        }
        
        
        //printf ("End for i = %d\n", i);
        //fflush (stdout);
    }
    return num_alive;
}


int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buf[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    
    //*
    // READ FROM hangman_words.txt
    FILE *hangman_file  = fopen("hangman_words.txt", "r"); // read only
    if (hangman_file == NULL) {
        fprintf(stderr, "ERROR, could not open hangman_words.txt\n");
        exit (1);
    }
    char buf_all_words[30][15];
    char buffer[30];
    bzero (buffer, 30);
    
    for (int i = 0; i < 15; i++) {
        bzero (buf_all_words[i], 30);
    }
    
    int total_words = 0;
    while ( fscanf(hangman_file, "%s", &buffer ) == 1 && total_words < 15) {
        for(int i = 0; buffer[i]; i++){
            buffer[i] = tolower(buffer[i]);
        }
        strcpy (buf_all_words[total_words], buffer);
        total_words++;
        bzero (buffer, 30);
    }
    
    if (total_words == 0) {
        fprintf(stderr, "ERROR, no words in hangman_words.txt\n");
        exit (1);
    }
    // */
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int on = 0;
    setsockopt(sockfd, SOCK_RDM, TCP_NODELAY, (const char *)&on, sizeof(int));
    if (sockfd < 0)
        error("ERROR opening socket");
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr,  sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    
    
    listen(sockfd,5);
    clilen = sizeof(cli_addr);
    
    int pids [4];
    pids [0] = 0;
    pids [1] = 0;
    pids [2] = 0;
    pids [3] = 0;
    
    int num_connections = 0;
    while (1) {
        // Check for closed connections here
        //
        
        //printf ("Before check_processes, parent");
        fflush (stdout);
        num_connections = check_processes (pids, num_connections);
        
        //printf ("After check_processes, parent");
        fflush (stdout);
        if (num_connections < 4) {
            // Wait and listen for incoming connections
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            
            // AND Check for closed connections here
            num_connections = check_processes (pids, num_connections);
            
            int pid = fork();
            //printf ("pid=%d", pid);
            fflush (stdout);
            if (pid == 0 && num_connections < 3) {
                // If there are still connections possible, I am the child process
                int on = 0;
                setsockopt(newsockfd, SOL_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
                
                //printf ("After new socket");
                
                if (newsockfd < 0)
                    error("ERROR on accept");
                
                // Wait for ready to
                char buffer[255];
                bzero(buffer,255);
                
                
                //printf ("Before read");
                
                int n = read(newsockfd,buffer,255);
                
                //printf ("After read");
                
                //printf ("before if n=%d\n", n);
                if (n >= 0 && buffer[0] == '\0') {
                    //printf ("after if n=%d\n", n);
                    
                    int random = rand() % total_words;
                    //printf ("-1\n");
                    char word_buffer [30];
                    bzero (word_buffer, 30);
                    //printf ("0Attempting to print word_buffer\n");
                    strcpy (word_buffer, buf_all_words[random]);
                    
                    //printf (word_buffer);
                    //printf ("1\n");
                    int guesses [26] = { 0 };
                    int num_wrong_guesses = 0;
                    int word_length = strlen (word_buffer);
                    int send_length = word_length +3;
                    int incomplete_word = 1;
                    char wrong_buffer [10];
                    bzero (wrong_buffer, 10);
                    
                    //printf ("2\n");
                    while (num_wrong_guesses < 6 && incomplete_word == 1) {
                        char guessed_buffer [30];
                        bzero (guessed_buffer, 30);
                        incomplete_word = 0;
                        
                        //printf ("3\n");
                        for (int i = 0; i < word_length; i++) {
                            if (guesses [word_buffer [i] - 'a'] == 1)
                                guessed_buffer[i] = word_buffer[i];
                            else {
                                incomplete_word = 1;
                                guessed_buffer[i] = '_';
                            }
                        }
                        //printf("%d\n", incomplete_word);
                        
                        //printf ("Finished constructing string");
                        char send_buffer [40];
                        send_buffer [0] = 0;
                        send_buffer [1] = word_length;
                        send_buffer [2] = num_wrong_guesses;
                        //  printf ("len guessed = %d", strlen (guessed_buffer));
                        
                        // printf ("len wrong = %d", strlen (wrong_buffer));
                        for (int i = 0; i < strlen (guessed_buffer); i++) {
                            send_buffer[i+3] = guessed_buffer[i];
                        }
                        
                        //strcat (send_buffer, guessed_buffer);
                        if (num_wrong_guesses > 0) {
                            for (int i = 0; i < strlen (wrong_buffer); i++) {
                                send_buffer[i+3+word_length] = wrong_buffer[i];
                            }
                        }
                        
                        
                        n = write(newsockfd, send_buffer, send_length+num_wrong_guesses);
                        
                        
                        
                        if (n < 0)
                            error ("SEND To Client Failure");
                        
                        if(incomplete_word == 1 && num_wrong_guesses < 6){
                            bzero(buffer,255);
                            n = read(newsockfd,buffer,255);
                            if (n < 0)
                                error ("READ From Client Failure");
                            
                            
                            if (!strchr (word_buffer, buffer [1]) || guesses [buffer[1] - 'a']) {
                                wrong_buffer [num_wrong_guesses] = buffer [1];
                                num_wrong_guesses += 1;
                            }
                            guesses [buffer[1] - 'a'] = 1;
                        }
                        
                    }
                    
                    char return_word [20];
                    //printf("finished\n");
                    if (! incomplete_word) {
                        strcpy (return_word, "You Win!");
                    }
                    else {
                        strcpy (return_word, "You Lose :(");
                    }
                    char send_buffer [40];
                    
                    send_buffer [0] = strlen(return_word);
                    for (int i = 0 ; i < strlen (return_word); i++) {
                        send_buffer[i+1] = return_word [i];
                    }
                    
                    n = write(newsockfd, send_buffer, strlen(return_word)+1);
                    if (n < 0)
                        error ("SEND To Client Failure");
                    
                }
                
                
                close(newsockfd);
                
                return 0;
            }
            else if (pid == 0 && num_connections >= 3) {
                // If no connections are available and I am a child process
                
                int on = 0;
                setsockopt(newsockfd, SOL_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));
                
                
                //printf ("After new socket, child, wrong location");
                
                if (newsockfd < 0)
                    error("ERROR on accept");
                char buffer [18];
                buffer [0] = '\17';
                strcat (buffer,  "server-overloaded");
                n=write(newsockfd, buffer, strlen (buffer));
                if (n < 0) error("Sendto");
                
                
                close(newsockfd);
                
                return 0;
            }
            else {
                // If I am the parent process
                
                //printf ("After new socket, parent num_con=%d", num_connections);
                //fflush (stdout);
                pids[num_connections] = pid;
                num_connections++;
                
                
                //printf ("Update PIDS, parent");
                //fflush (stdout);
            }
        }
        
    }
    close(sockfd);
    return 0;
}
