#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sqlite3.h>
#include "SQLCommands.h"

#define DBNAME "SPST.db"
#define PORT 2024
#define MAXMSGLEN 512

int currentUser = -1;
char serverType;

struct User
{
    int ID;
    char username[64];
    char password[64];
};

void sendResponse(int client, char *response)
{
    printf("[Broker#%d] Trimit raspuns:\"%s\"\n", getpid(), response);
    if (write(client, response, MAXMSGLEN) <= 0)
        printf("[Broker#%d] Eroare la write() catre client.\n", getpid());
    else
        printf("[Broker#%d] Raspuns trimis.\n", getpid());
}

void retrieveUser(struct User *dest, const char *username)
{
    char command[256];
    sprintf(command, selectUser, serverType == 's' ? "Subscribers" : "Publishers", username);
    sqlite3 *database;
    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        sqlite3_close(database);
        dest->ID = -2;
        printf("[Broker#%d] Eroare la deschiderea bazei de date: %s\nServer inchis.\n", getpid(), sqlite3_errmsg(database));
        exit(1);
    }
    sqlite3_stmt *statement;
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_ROW)
        dest->ID = -1;
    else
    {
        dest->ID = sqlite3_column_int(statement, 0);
        strcpy(dest->username, username);
        strcpy(dest->password, sqlite3_column_text(statement, 2));
    }
    sqlite3_finalize(statement);
    sqlite3_close(database);
}

void attemptLogin(const char *username, const char *password, char *response)
{
    struct User usr;
    retrieveUser(&usr, username);
    if (usr.ID == -1)
    {
        sprintf(response, ".Utilizatorul \"%s\" nu exista.", username);
        return;
    }
    else if (usr.ID == -2)
    {
        strcpy(response, ".Serverul a intampinat o eroare la deschiderea bazei de date.");
        printf("[Broker#%d] Eroare la deschiderea bazei de date.\nServer inchis.\n", getpid());
    }

    if (strcmp(usr.password, password) == 0)
    {
        currentUser = usr.ID;
        sprintf(response, ".Conectat cu succes ca: %s.", usr.username);
    }
    else
        strcpy(response, ".Parola incorecta.");
}

void handleLogin(char *request, char *response)
{
    char username[64], password[64], *p;
    int nArgs = 0;
    p = strtok(request, " ");
    while (p != NULL)
    {
        nArgs++;
        if (nArgs == 2)
            strcpy(username, p);
        if (nArgs == 3)
            strcpy(password, p);
        p = strtok(NULL, " ");
    }

    if (nArgs != 3)
        strcpy(response, ".Numar gresit de argumente.");
    else if (currentUser != -1)
        strcpy(response, ".Deja esti conectat. Deconecteaza-te mai intai.");
    else
        attemptLogin(username, password, response);
}

void attemptRegister(const char *username, const char *password, char *response)
{
    struct User usr;
    retrieveUser(&usr, username);
    if (usr.ID != -1)
    {
        sprintf(response, ".Exista deja un cont de %s cu numele \"%s\".", serverType == 's' ? "Subscriber" : "Publisher", username);
        return;
    }

    char command[256];
    sprintf(command, insertUser, serverType == 's' ? "Subscribers" : "Publishers", username, password);
    sqlite3 *database;
    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        strcpy(response, ".*Serverul a intampinat o eroare fatala. Reporniti clientul.*");
        printf("[Broker#%d] Eroare la deschiderea bazei de date: %s\nServer inchis.\n", getpid(), sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(1);
    }
    sqlite3_stmt *statement;
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_DONE)
        sprintf(response, ".A aparut o problema la inregistrarea in baza de date.");
    else
        sprintf(response, ".Contul de %s cu numele \"%s\" a fost creat cu succes.", serverType == 's' ? "Subscriber" : "Publisher", username);
    sqlite3_finalize(statement);
    sqlite3_close(database);
}

void handleRegister(char *request, char *response)
{
    char username[64], password[64], *p;
    int nArgs = 0;
    p = strtok(request, " ");
    while (p != NULL)
    {
        nArgs++;
        if (nArgs == 2)
            strcpy(username, p);
        if (nArgs == 3)
            strcpy(password, p);
        p = strtok(NULL, " ");
    }
    if (nArgs != 3)
        strcpy(response, ".Numar gresit de argumente.");
    else
        attemptRegister(username, password, response);
}

void attemptDeliverAll(char *response, int client)
{
    strcpy(response, "!Toate articolele scrise vreodata pe topicurile tale de interes:");
    sendResponse(client, response);
    char command[300];
    int artCount = 0;

    sprintf(command, selectAllArticles, currentUser);
    sqlite3 *database;
    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        strcpy(response, ".*Serverul a intampinat o eroare fatala. Reporniti clientul.*");
        sendResponse(client, response);
        printf("[Broker#%d] Eroare la deschiderea bazei de date: %s\nServer inchis.\n", getpid(), sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(1);
    }

    sqlite3_stmt *statement;
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        sprintf(response, "!\t\"%s\" - de '%s'\n  %s", sqlite3_column_text(statement, 0), sqlite3_column_text(statement, 1), sqlite3_column_text(statement, 2));
        sendResponse(client, response);
        artCount++;
    }
    sqlite3_finalize(statement);
    sqlite3_close(database);
    sprintf(response, ".Total: %d articol(e).", artCount);
    sendResponse(client, response);
}

void attemptDeliverNew(char *response, int client)
{
    strcpy(response, "!Articole noi de interes de la ultima verificare:");
    sendResponse(client, response);
    char command[300], lastUpdateTimestamp[32], currentTimestamp[32];
    int artCount = 0;
    sprintf(command, selectTimestampPair, currentUser);

    sqlite3 *database;
    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        strcpy(response, ".*Serverul a intampinat o eroare fatala. Reporniti clientul.*");
        sendResponse(client, response);
        printf("[Broker#%d] Eroare la deschiderea bazei de date: %s\nServer inchis.\n", getpid(), sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(1);
    }

    sqlite3_stmt *statement;
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_ROW)
    {
        strcpy(response, ".Nu aveti niciun topic preferat setat.");
        sendResponse(client, response);
        sqlite3_finalize(statement);
        sqlite3_close(database);
        return;
    }
    strcpy(lastUpdateTimestamp, sqlite3_column_text(statement, 0));
    strcpy(currentTimestamp, sqlite3_column_text(statement, 1));
    sqlite3_finalize(statement);

    sprintf(command, selectNewArticles, lastUpdateTimestamp, currentUser);
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        sprintf(response, "!\t\"%s\" - de '%s'\n  %s", sqlite3_column_text(statement, 0), sqlite3_column_text(statement, 1), sqlite3_column_text(statement, 2));
        sendResponse(client, response);
        artCount++;
    }
    sqlite3_finalize(statement);
    sprintf(response, ".Total: %d articol(e).", artCount);
    sendResponse(client, response);

    sprintf(command, updateTimestamps, currentTimestamp, currentUser);
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_DONE)
        printf("[Broker#%d] Eroare la actualizat data ultimului update: %s.\n", getpid(), sqlite3_errmsg(database));
    sqlite3_finalize(statement);
    sqlite3_close(database);
}

void handleDeliver(char *request, char *response, int client)
{
    char option[8], *p;
    int nArgs = 0;
    p = strtok(request, " ");
    while (p != NULL)
    {
        nArgs++;
        if (nArgs == 2)
            strcpy(option, p);
        p = strtok(NULL, " ");
    }
    if (nArgs != 2)
    {
        strcpy(response, ".Numar gresit de argumente.");
        sendResponse(client, response);
        return;
    }
    if (strncmp(option, "toate", 5) == 0)
        attemptDeliverAll(response, client);
    else if (strncmp(option, "noi", 3) == 0)
        attemptDeliverNew(response, client);
    else
    {
        strcpy(response, ".Optiune invalida.");
        sendResponse(client, response);
    }
}

void attemptAddLikedTopic(const char *topic, char *response, int client)
{
    char command[256];
    int topicID;
    sqlite3 *database;

    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        strcpy(response, ".*Serverul a intampinat o eroare fatala. Reporniti clientul.*");
        sendResponse(client, response);
        printf("[Broker#%d] Eroare la deschiderea bazei de date: %s\nServer inchis.\n", getpid(), sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(1);
    }
    sprintf(command, selectSTPair, topic, currentUser);
    sqlite3_stmt *statement;
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    sqlite3_step(statement);
    if (sqlite3_column_int(statement, 0) == 1)
    {
        sprintf(response, "!\"%s\" - deja existent.", topic);
        sendResponse(client, response);
        sqlite3_finalize(statement);
        return;
    }
    sqlite3_finalize(statement);
    sprintf(command, selectTopic, topic);
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    if (sqlite3_step(statement) == SQLITE_ROW)
        topicID = sqlite3_column_int(statement, 0);
    else
    {
        sqlite3_finalize(statement);
        sprintf(command, insertTopic, topic);
        sqlite3_prepare_v2(database, command, -1, &statement, NULL);
        if (sqlite3_step(statement) != SQLITE_DONE)
        {
            sprintf(response, "!\"%s\" - nu l-am putut crea.", topic);
            sendResponse(client, response);
            sqlite3_finalize(statement);
            return;
        }
        topicID = sqlite3_last_insert_rowid(database);
    }
    sqlite3_finalize(statement);
    sprintf(command, insertSTPair, currentUser, topicID);
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    if (sqlite3_step(statement) == SQLITE_DONE)
        sprintf(response, "!\"%s\" - adaugat.", topic);
    else
        sprintf(response, "!\"%s\" - nu l-am putut adauga.", topic);
    sendResponse(client, response);
    sqlite3_finalize(statement);
    sqlite3_close(database);
}

void attemptDelLikedTopic(const char *topic, char *response, int client)
{
    char command[128];
    sprintf(command, deleteSTPair, currentUser, topic);
    sqlite3 *database;
    sqlite3_stmt *statement;

    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        strcpy(response, ".*Serverul a intampinat o eroare fatala. Reporniti clientul.*");
        sendResponse(client, response);
        printf("[Broker#%d] Eroare la deschiderea bazei de date: %s\nServer inchis.\n", getpid(), sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(1);
    }
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_DONE)
    {
        sprintf(response, "!\"%s\" - nu l-am putut sterge.", topic);
        sendResponse(client, response);
        sqlite3_finalize(statement);
        sqlite3_close(database);
        return;
    }
    if (sqlite3_changes(database) == 0)
        sprintf(response, "!\"%s\" - nu exista.", topic);
    else
        sprintf(response, "!\"%s\" - sters.", topic);
    sendResponse(client, response);
}

void attemptShowTopics(char *response, int client)
{
    strcpy(response, "!Topicurile tale:");
    sendResponse(client, response);
    char command[128];
    int tCount = 0;
    sqlite3 *database;

    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        strcat(response, ".*Serverul a intampinat o eroare fatala. Reporniti clientul.*");
        sendResponse(client, response);
        printf("[Broker#%d] Eroare la deschiderea bazei de date: %s\nServer inchis.\n", getpid(), sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(1);
    }
    sprintf(command, selectTopics, currentUser);
    sqlite3_stmt *statement;
    sqlite3_prepare_v2(database, command, -1, &statement, NULL);
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        sprintf(response, "!%s", sqlite3_column_text(statement, 0));
        sendResponse(client, response);
        tCount++;
    }
    sqlite3_finalize(statement);
    sqlite3_close(database);
    sprintf(response, ".Total: %d topic(uri) preferat(e).", tCount);
    sendResponse(client, response);
}

void handleUpdateTopics(char *request, char *response, int client)
{
    char *p;
    int nArgs = 1;
    p = strtok(request, " ");
    p = strtok(NULL, " ");
    while (p != NULL)
    {
        bzero(response, MAXMSGLEN);
        nArgs++;
        if (p[0] == '+')
            attemptAddLikedTopic(p + 1, response, client);
        else if (p[0] == '-')
            attemptDelLikedTopic(p + 1, response, client);
        else
        {
            sprintf(response, "!\"%s\" - argument invalid.", p);
            sendResponse(client, response);
            nArgs--;
        }
        p = strtok(NULL, " ");
    }
    if (nArgs <= 1)
        strcpy(response, ".Numar invalid de argumente (corecte).");
    else
        sprintf(response, ".Total: %d topic(uri) actualizat(e).", nArgs - 1);
    sendResponse(client, response);
}

void attemptPublish(const char *title, const char *text, char *topics, char *response, int client)
{
    char publishCommand[(int)1.5 * MAXMSGLEN];
    sprintf(publishCommand, insertArticle, currentUser, title, text);

    sqlite3 *database;
    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        strcat(response, ".*Serverul a intampinat o eroare fatala. Reporniti clientul.*");
        sendResponse(client, response);
        printf("[Broker#%d] Eroare la deschiderea bazei de date: %s\nServer inchis.\n", getpid(), sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(1);
    }
    sqlite3_stmt *statement;
    sqlite3_prepare_v2(database, publishCommand, -1, &statement, NULL);
    if (sqlite3_step(statement) != SQLITE_DONE)
    {
        sprintf(response, ".A aparut o eroare la publicarea articolului.");
        sendResponse(client, response);
        sqlite3_finalize(statement);
        sqlite3_close(database);
        return;
    }
    sprintf(response, "!Articolul \"%s\" a fost publicat. ", title);
    sendResponse(client, response);

    sqlite3_finalize(statement);
    int articleID = sqlite3_last_insert_rowid(database), topicID, tCount = 0;
    char command[64], *p;
    p = strtok(topics, ",");
    while (p != NULL)
    {
        tCount++;
        bzero(response, MAXMSGLEN);
        sprintf(command, selectTopic, p);
        sqlite3_prepare_v2(database, command, -1, &statement, NULL);
        if (sqlite3_step(statement) == SQLITE_ROW)
            topicID = sqlite3_column_int(statement, 0);
        else
        {
            sqlite3_finalize(statement);
            sprintf(command, insertTopic, p);
            sqlite3_prepare_v2(database, command, -1, &statement, NULL);
            if (sqlite3_step(statement) != SQLITE_DONE)
            {
                sqlite3_finalize(statement);
                sprintf(response, "!Nu am putut crea topicul \"%s\".", p);
                sendResponse(client, response);
                p = strtok(NULL, ",");
                continue;
            }
            topicID = sqlite3_last_insert_rowid(database);
        }

        sqlite3_finalize(statement);
        sprintf(command, insertATPair, articleID, topicID);
        sqlite3_prepare_v2(database, command, -1, &statement, NULL);
        if (sqlite3_step(statement) == SQLITE_DONE)
            sprintf(response, "!Am legat articolul de topicul \"%s\".", p);
        else
            sprintf(response, "!Nu am putut lega articolul de topicul \"%s\".", p);
        sqlite3_finalize(statement);

        sendResponse(client, response);
        p = strtok(NULL, ",");
    }

    sqlite3_close(database);
    sprintf(response, ".Total: %d topic(uri) asociat(e).", tCount);
    sendResponse(client, response);
}

void handlePublish(char *request, char *response, int client)
{
    char title[64], text[400], topics[128], *p;
    int nArgs = 0;
    p = strtok(request, "#");
    while (p != NULL)
    {
        nArgs++;
        if (nArgs == 2)
            strcpy(title, p);
        else if (nArgs == 3)
            strcpy(text, p);
        else if (nArgs == 4)
            strcpy(topics, p);
        p = strtok(NULL, "#");
    }
    if (nArgs != 4)
    {
        strcpy(response, ".Numar gresit de argumente");
        sendResponse(client, response);
        return;
    }

    attemptPublish(title, text, topics, response, client);
}

void serveSubscriber(int client)
{
    char request[MAXMSGLEN];
    char response[MAXMSGLEN];

    while (1)
    {
        bzero(request, MAXMSGLEN);
        printf("[Broker#%d] Astept o cerere.\n", getpid());
        fflush(stdout);

        if (read(client, request, MAXMSGLEN) <= 0)
        {
            printf("[Broker#%d] Eroare la read() de la client.\n", getpid());
            close(client);
            continue;
        }

        printf("[Broker#%d] Cerere primita: %s\n", getpid(), request);

        bzero(response, MAXMSGLEN);
        switch (request[0])
        {
        case '1':
            handleLogin(request, response);
            sendResponse(client, response);
            break;
        case '2':
            handleRegister(request, response);
            sendResponse(client, response);
            break;
        case '3':
            handleDeliver(request, response, client);
            break;
        case '4':
            attemptShowTopics(response, client);
            break;
        case '5':
            handleUpdateTopics(request, response, client);
            break;
        case '6':
            currentUser = -1;
            strcat(response, ".Deconectat.");
            sendResponse(client, response);
            break;
        case '0':
            strcat(response, ".Inchidere.");
            sendResponse(client, response);
            break;

        default:
            strcpy(response, ".Comanda necunoscuta.");
            sendResponse(client, response);
            break;
        }

        if (request[0] == '0')
            break;
    }
    close(client);
    exit(0);
}

void servePublisher(int client)
{
    char request[MAXMSGLEN];
    char response[MAXMSGLEN];

    while (1)
    {
        bzero(request, MAXMSGLEN);
        printf("[Broker#%d] Astept o cerere.\n", getpid());
        fflush(stdout);

        if (read(client, request, MAXMSGLEN) <= 0)
        {
            printf("[Broker#%d] Eroare la read() de la client.\n", getpid());
            close(client);
            continue;
        }

        printf("[Broker#%d] Cerere primita: %s\n", getpid(), request);

        bzero(response, MAXMSGLEN);
        switch (request[0])
        {
        case '1':
            handleLogin(request, response);
            sendResponse(client, response);
            break;
        case '2':
            handleRegister(request, response);
            sendResponse(client, response);
            break;
        case '3':
            handlePublish(request, response, client);
            break;
        case '4':
            currentUser = -1;
            strcat(response, ".Deconectat.");
            sendResponse(client, response);
            break;
        case '0':
            strcat(response, ".Inchis.");
            sendResponse(client, response);
            break;

        default:
            strcpy(response, ".Comanda necunoscuta.");
            sendResponse(client, response);
            break;
        }

        if (request[0] == '0')
            break;
    }
    close(client);
    exit(0);
}

void initDB()
{
    const char *tableNames[] = {"Subscribers", "Publishers", "Articles", "Topics", "ATPair", "STPair"};
    const char *createStatements[] = {createSubscribers, createPublishers, createArticles, createTopics, createATPair, createSTPair};
    sqlite3 *database;
    if (sqlite3_open(DBNAME, &database) != SQLITE_OK)
    {
        printf("Eroare la deschiderea bazei de date: %s\nServer inchis.\n", sqlite3_errmsg(database));
        sqlite3_close(database);
        exit(1);
    }
    sqlite3_stmt *statement;
    for (int i = 0; i < 6; i++)
    {
        sqlite3_prepare_v2(database, createStatements[i], -1, &statement, NULL);
        if (sqlite3_step(statement) != SQLITE_DONE)
        {
            printf("A aparut o eroare in crearea tabelului \"%s\".\nServer inchis.\n", tableNames[i]);
            sqlite3_finalize(statement);
            sqlite3_close(database);
            exit(1);
        }
        sqlite3_finalize(statement);
    }
    sqlite3_close(database);
    printf("[MasterBroker] Baza de date functionala.\n");
}

int main()
{
    initDB();

    struct sockaddr_in masterSocket;
    struct sockaddr_in communicationSocket;
    int socketDescriptor, controlCode;

    if ((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[MasterBroker] Eroare la socket().\n");
        return errno;
    }

    bzero(&masterSocket, sizeof(masterSocket));
    bzero(&communicationSocket, sizeof(communicationSocket));

    masterSocket.sin_family = AF_INET;
    masterSocket.sin_addr.s_addr = htonl(INADDR_ANY);
    masterSocket.sin_port = htons(PORT);

    if (bind(socketDescriptor, (struct sockaddr *)&masterSocket, sizeof(struct sockaddr)) == -1)
    {
        perror("[MasterBroker] Eroare la bind().\n");
        return errno;
    }

    if (listen(socketDescriptor, 1) == -1)
    {
        perror("[MasterBroker] Eroare la listen().\n");
        return errno;
    }

    while (1)
    {
        int client;
        int length = sizeof(communicationSocket);

        printf("[MasterBroker] Astept la portul %d...\n", PORT);
        fflush(stdout);

        client = accept(socketDescriptor, (struct sockaddr *)&communicationSocket, &length);

        if (client < 0)
        {
            perror("[MasterBroker] Eroare la accept().\n");
            continue;
        }

        int pid;
        if ((pid = fork()) == -1)
        {
            printf("[MasterBroker] Eroare la fork().\n");
            close(client);
            continue;
        }
        else if (pid > 0)
        {
            close(client);
            while (waitpid(-1, NULL, WNOHANG));
            continue;
        }
        else if (pid == 0)
        {
            close(socketDescriptor);
            if (read(client, &controlCode, sizeof(int)) <= 0)
            {
                printf("[Broker#%d] Eroare la read() de la client.\n", getpid());
                close(client);
                continue;
            }
            switch (controlCode)
            {
            case 1:
                printf("[Broker#%d] Acum servesc un Subscriber.\n", getpid());
                serverType = 's';
                serveSubscriber(client);
                break;
            case 2:
                printf("[Broker#%d] Acum servesc un Publisher.\n", getpid());
                serverType = 'p';
                servePublisher(client);
                break;
            default:
                printf("[Broker#%d] Tip de client necunoscut.\n", getpid());
                break;
            }
            exit(0);
        }
    }
}