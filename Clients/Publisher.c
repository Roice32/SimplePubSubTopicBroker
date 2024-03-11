#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>

#define MAXUSRNAMELEN 64
#define MAXMSGLEN 512

char currentUser[MAXUSRNAMELEN];

void recieveResponse(int server, char *response)
{
    bzero(response, MAXMSGLEN);
    if (currentUser[0] == '\0')
        printf("\n[Publisher Client] Rezultat:\n");
    else
        printf("\n[Publisher \"%s\"] Rezultat:\n", currentUser);
    do
    {
        if (read(server, response, MAXMSGLEN) < 0)
        {
            printf("[Publisher Client] Eroare la read() de la Broker.\n");
            break;
        }
        printf("%s\n", response + 1);
    } while (response[0] == '!');
    printf("\n");
}

void displayMenu()
{
    if (currentUser[0] == '\0')
        printf("\t[Publisher Client] Comenzi:\n");
    else
        printf("\t[Publisher \"%s\"] Comenzi:\n", currentUser);
    if (currentUser[0] == 0)
    {
        printf("> Conectare: 1 <Nume Utilizator> <Parola>\n");
        printf("> Inregistrare: 2 <Nume Utilizator> <Parola>\n");
    }
    else
    {
        printf("> Publicare articol (intreaga comanda sa se incadreze in 512 caractere): 3#<Titlu>#<Text>#<Topic1>,{Topic2},...,{Topicn}\n");
        printf("> Deconectare: 4\n");
    }
    printf("> Inchidere: 0\n");
    printf("Comanda dvs: ");
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    bzero(currentUser, MAXUSRNAMELEN);
    int port;
    int socketDescriptor;
    struct sockaddr_in server;
    char message[MAXMSGLEN];
    int controlCode = 2;

    if (argc != 3)
    {
        printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[2]);

    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("[Publisher Client] Eroare la socket().\n");
        return -2;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_port = htons(port);

    if (connect(socketDescriptor, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        printf("[Publisher Client] Eroare la connect().\n");
        return -3;
    }
    if (write(socketDescriptor, &controlCode, sizeof(int)) <= 0)
    {
        printf("[Publisher Client] Eroare la write() spre server.\n");
        return -4;
    }

    while (1)
    {
        bzero(message, MAXMSGLEN);
        displayMenu();
        read(0, message, MAXMSGLEN);

        for (int i = strlen(message) - 1; message[i] == '\n' && i >= 0; i--)
            message[i] = '\0';

        if (write(socketDescriptor, message, MAXMSGLEN) <= 0)
        {
            printf("[Publisher Client] Eroare la write() spre server.\n");
            return -4;
        }

        recieveResponse(socketDescriptor, message);

        if (strncmp(message + 1, "Conect", 6) == 0)
        {
            strcpy(currentUser, strstr(message, ":") + 2);
            currentUser[strlen(currentUser) - 1] = '\0';
        }
        if (strncmp(message + 1, "Decon", 5) == 0)
            bzero(currentUser, MAXUSRNAMELEN);
        if (strncmp(message + 1, "Inchid", 5) == 0)
            break;
    }
    close(socketDescriptor);
}