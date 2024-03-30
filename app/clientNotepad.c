/* cliTCP.c - Exemplu de client TCP
   Trimite un nume la server; primeste de la server "Hello nume".

   Autor: Lenuta Alboaie  <adria@info.uaic.ro> (c)

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

/* structura modify text */
struct modify_text {
    int user_id;
    int operation_type;
    int cursor_position;
    int length;
    char data[500];
};

void strtrim(char *str) {
    int start = 0, end = strlen(str) - 1;

    // gaseste prima pozitie non-spațiu la inceput
    while (isspace((unsigned char)str[start]))
        start++;

    // gaseste prima pozitie non-spațiu la sfarsit
    while ((end >= start) && isspace((unsigned char)str[end]))
        end--;

    // sterge spatiile albe de la început la sfarsit
    for (int i = start; i <= end; i++)
        str[i - start] = str[i];

    // adauga \0 la final de sir
    str[end - start + 1] = '\0';
}

bool isCommandValid(const char* command) {

    char trimmedCommand[100];
    strcpy(trimmedCommand, command);
    strtrim(trimmedCommand);

    //strcmp(trimmedCommand, "login") == 0
    if (strncmp(command,"login", 5) == 0 || strncmp(trimmedCommand, "invite", 6) == 0 || strncmp(trimmedCommand, "logout",6) == 0 || strncmp(trimmedCommand, "signup", 6) == 0 || strncmp(trimmedCommand, "accept", 6) == 0  || strncmp(trimmedCommand, "deny", 4) == 0 || strcmp(trimmedCommand, "close session") == 0 || strncmp(command, "create_document", 15) == 0 || strncmp(command, "edit", 4) == 0 || strncmp(command, "download_document", 17) == 0 || strncmp(command, "find_document", 13) == 0)
        return 0;

    return 1;
}

void* receiveMessages(void* arg) {
    int clientSocket = *((int*)arg);
    char message[500];

    while (1) {
        int bytesRead = read(clientSocket, message, sizeof(message));

        if (bytesRead <= 0) {
            break;
        }

        if (strncmp(message, "Editing document...", 19) == 0) {
            printf("%s", message);

            bzero(message, sizeof(message));
            if (read(clientSocket, message, sizeof(message)) <= 0) {
                perror("[client] Error at read() while receiving document content.\n");
                break;
            }

            printf("Document content:\n%s\n", message);

            printf("Enter your edits (press Enter to finish):\n");
            char edits[500];
            bzero(edits, sizeof(edits));
            fgets(edits, sizeof(edits), stdin);
            strtrim(edits);

            // Trimite modificarile la server
            if(write(clientSocket, edits, sizeof(edits)) <= 0) {
                perror("[client] Error at write() while sending edits.\n");
                break;
            }

            bzero(message, sizeof(message));
            if(read(clientSocket, message, sizeof(message)) <= 0) {
                perror("[client] Error at read() while receiving confirmation.\n");
                break;
            }

            printf("[client] Server response: %s\n", message);
        } else {
            printf("[client] Received message: %s\n", message);
        }
    }

    return NULL;
}


int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare
  char msg[100];		// mesajul trimis

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("[client] Synthax: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[client] Error at socket().\n");
      return errno;
    }


  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);

  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Error at connect().\n");
      return errno;
    }

    // Creeaza un fir de executie pentru primirea mesajelor de la celalalt client
    pthread_t receiveThread;
    pthread_create(&receiveThread, NULL, receiveMessages, (void*)&sd);

  while(1) {
    /* citirea comenzii de la tastatura */
    bzero (msg, 100);
    printf ("[client] Enter command: ");
    fflush (stdout);
    read (0, msg, 100);
    bool comandaInvalida = 0;

    /* verificare validitate comanda inainte de trimiterea la server */
    if(isCommandValid(msg) == 1) {
        comandaInvalida = 1;
        printf("[client] Command is not valid. Try entering a valid command.\n");
    }

    //printf("comandaInvalida: %d\n", (int)comandaInvalida);

    if(comandaInvalida == 0) {
        /* trimiterea comenzii la server */
    if (write(sd, msg, 100) <= 0)
        {
            perror("[client] Error at write() to the server.\n");
            break;
        }

     /* citirea raspunsului dat de server
     (apel blocant pina cind serverul raspunde) */
     if (read (sd, msg, 100) < 0)
     {
        perror ("[client]Error at read() from the server.\n");
        break;
     }
        if(strcmp(msg, "stop") == 0) {
          printf("[client]Closing the session...\n");
          break;
        } else if (strncmp(msg, "Editing document...", 19) == 0) {
                // logica pentru editare
                pthread_t receiveThread;
                pthread_create(&receiveThread, NULL, receiveMessages, (void*)&sd);
                pthread_join(receiveThread, NULL);
            } else {
                printf("[client] %s\n", msg);
            }
     }
  }

  // Așteaptă să se încheie firul de primire înainte de a închide conexiunea
  pthread_join(receiveThread, NULL);

  /* inchidem conexiunea, am terminat */
  close(sd);
  return 0;
}
