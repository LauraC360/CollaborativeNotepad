/* servTCPConcTh.c - Exemplu de server TCP concurent care deserveste clientii
   prin crearea unui thread pentru fiecare client.
   Asteapta un command de la clienti si intoarce clientilor mesajul "Hello command".

   Autor: Lenuta Alboaie  <adria@info.uaic.ro> (c)
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdint.h>

//librarii pentru xml
#include <libxml/parser.h>
#include <libxml/tree.h>

//libraria pentru sqlite
#include <sqlite3.h>

/* portul folosit */
#define PORT 2024

/* block size-ul pt sha256 (algoritmul de hash al parolei) */
#define SHA256_BLOCK_SIZE 64

// pairing clienti
#define MAX_CLIENTS 500

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData {
    int idThread; //id-ul thread-ului tinut in evidenta de acest program
    int cl;       //descriptorul intors de accept
    int clientID;
} thData;


// Structura pentru a stoca informații despre o pereche de clienți
typedef struct {
    int client1ID;
    int client2ID;
} ClientPair;

/**
struct ClientInfo {
    int idThread;
    int clientID;
    char numeUtilizator[30];
    bool Editing;
    bool Connected;
    //alte info despre client
};**/

thData clients[MAX_CLIENTS];
int numClients = 0;

// Definirea mutex-ului global
pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;

///// FUNCTIILE PT PAIRING INTRE CLIENTI

/**
// Functie pentru a sterge un client din array
void removeClient(int idThread) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].idThread == idThread) {
            // Marchează slot-ul ca fiind disponibil
            clients[i].idThread = 0;
            clients[i].cl = 0;
            break; // Ieși din buclă după ce ai găsit și marcat slot-ul
        }
    }
}*/

void removeClient(int idThread) {
    pthread_mutex_lock(&clientsMutex);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].idThread == idThread) {
            // Marchează slot-ul ca fiind disponibil
            clients[i].idThread = 0;
            clients[i].cl = 0;
            break; // Ieși din buclă după ce ai găsit și marcat slot-ul
        }
    }

    pthread_mutex_unlock(&clientsMutex);
}

int findClientById(int idThread) {
    for(int i = 0; i < numClients; ++i) {
        if(clients[i].idThread == idThread) {
            return i;  // Returnează indicele găsit
        }
    }
    return -1;  // Returnează -1 dacă nu a fost găsit
}

// Funcție pentru a găsi o structură thData după idThread
struct thData* findThreadById(pthread_t idThread) {
    for(int i = 0; i < numClients; ++i) {
        if(clients[i].idThread == idThread) {
            return &clients[i];
        }
    }
    return NULL;  // Nu s-a găsit thread-ul
}

void sendMessageToClient(int destClientID, const char *message) {
    pthread_mutex_lock(&clientsMutex);

    for(int i = 0; i < MAX_CLIENTS; ++i) {
        if(clients[i].idThread == destClientID) {
            printf("Sending message to client %d: %s\n", destClientID, message); // Debug message
            write(clients[i].cl, message, strlen(message));
            break;
        }
    }

    pthread_mutex_unlock(&clientsMutex);
}

void* handleClientCommunication(void* arg) {
    ClientPair* clientPair = (ClientPair*)arg;

    int client1ID = clientPair->client1ID;
    int client2ID = clientPair->client2ID;

    // Logica de comunicare
    while (1) {
        char message[100];
        int bytesRead = read(client1ID, message, sizeof(message));

        if (bytesRead <= 0) {
            // Ceva nu a mers bine sau clientul 1 a închis conexiunea
            break;
        }

        // Trimite mesajul de la client 1 către client 2
        write(client2ID, message, bytesRead);

        // Așteaptă răspunsul de la client 2
        bytesRead = read(client2ID, message, sizeof(message));

        if(bytesRead <= 0) {
            // Ceva nu a mers bine sau clientul 2 a închis conexiunea
            break;
        }

        // Trimite răspunsul de la client 2 către client 1
        write(client1ID, message, bytesRead);
    }

    // Închide conexiunile și eliberează memoria alocată pentru ClientPair
    close(client1ID);
    close(client2ID);
    free(clientPair);

    return NULL;
}

/**
// fct pt a trimite mesajul unui client cu un anumit idThread
void sendMessageToClient(int dest_clientID, const char *message) {
    // Găsește thread-ul corespunzător clientului destinatar
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].idThread == dest_clientID) {
            // Trimite mesajul la clientul destinatar
            if (write(clients[i].cl, message, strlen(message) + 1) <= 0) {
                perror("Error at writing to destination thread.\n");
            }
            return; // iesi din bucla daca ai gasit clientul
        }
    }
    // clientul cu dest_clientID nu a fost găsit
    printf("Error: Client with ID %d not found!\n", dest_clientID);
}**/

/**
void adaugaPerecheClienti(struct ClientInfo client1, struct ClientInfo client2) {
    if(nrPerechi < MAX_PERECHI_CLIENTI) {
        client1.Paired = 1;
        client2.Paired = 1;

        perechiClienti[nrPerechi++] = client1;
        perechiClienti[nrPerechi++] = client2;

        printf("Pereche de clienți adăugată cu succes.\n");
    } else {
        printf("Numărul maxim de perechi de clienți a fost atins. Imposibil de adăugat perechea.\n");
    }
}

void eliminaClient(int idThread) {
    for (int i = 0; i < nrPerechi; i++) {
        if (perechiClienti[i].idThread == idThread) {
            // Resetare indicator de pereche pentru utilizator
            perechiClienti[i].Paired = false;

            // Elimină clientul din listă
            for (int j = i; j < nrPerechi - 1; j++) {
                perechiClienti[j] = perechiClienti[j + 1];
            }

            nrPerechi--;

            printf("Client eliminat cu succes.\n");
            return;
        }
    }

    printf("Clientul cu idThread %d nu a fost găsit.\n", idThread);
}
**/


static void *treat(void *); /* functia executata de fiecare thread ce realizeaza comunicarea cu clientii */

void raspunde(void *);

void XMLFileParser(const char *xmlFilePath) {
    xmlDocPtr doc;
    xmlNodePtr root, userNode, documentNode, propertyNode;

    // Parsare XML
    doc = xmlReadFile(xmlFilePath, NULL, 0);

    if (doc == NULL) {
        fprintf(stderr, "Eroare la parsarea documentului.\n");
        return;
    }

    // Obținem nodul rădăcină
    root = xmlDocGetRootElement(doc);

    // Verificăm dacă nodul rădăcină este "documentsList"
    if (xmlStrcmp(root->name, BAD_CAST "documentsList")) {
        fprintf(stderr, "Eroare: Nodul rădăcină nu este \"documentsList\".\n");
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return;
    }

    // Iterăm prin nodurile copil ale nodului rădăcină
    for (userNode = root->children; userNode; userNode = userNode->next) {
        // Verificăm dacă nodul este de tip element și are numele "user"
        if (userNode->type == XML_ELEMENT_NODE && xmlStrEqual(userNode->name, BAD_CAST "user")) {
            // Afișăm numele utilizatorului
            printf("Utilizator: %s\n", xmlGetProp(userNode, BAD_CAST "name"));

            // iterare prin noduri
            for (documentNode = userNode->children; documentNode; documentNode = documentNode->next) {
                // Verificăm dacă nodul este de tip element și are numele "document"
                if (documentNode->type == XML_ELEMENT_NODE && xmlStrEqual(documentNode->name, BAD_CAST "document")) {
                    printf("Document data: \n");

                    // Iterăm prin nodurile copil ale nodului "document"
                    for (propertyNode = documentNode->children; propertyNode; propertyNode = propertyNode->next) {
                        // Verificăm dacă nodul este de tip element
                        if (propertyNode->type == XML_ELEMENT_NODE) {
                            // Afișăm numele elementului și conținutul
                            printf("    %s: %s\n", propertyNode->name, xmlNodeGetContent(propertyNode));
                        }
                    }
                }
            }
        }
    }

    // eliberare resurse
    xmlFreeDoc(doc);
    xmlCleanupParser();
}

int AfisareInterogari(void *data, int argc, char **argv, char **colNames) {
    int i;
    printf("%s: ", (const char*)data);

    for (i = 0; i < argc; i++) {
        printf("%s = %s, ", colNames[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

/**
int addDocument(sqlite3 *database, int idUtilizator, const char *continut, int *idDocument) {
    char sql_query[2000];

    // Adaugă documentul în baza de date
    snprintf(sql_query, sizeof(sql_query),
             "INSERT INTO Documente (ID_Utilizator, Continut) "
             "VALUES (%d, '%s');",
             idUtilizator, continut);

    if(sqlite3_exec(database, sql_query, 0, 0, 0) != SQLITE_OK) {
        fprintf(stderr, "Eroare la adăugarea documentului în baza de date.\n");
        return -1;
    }

    // Obține ID-ul documentului inserat
    *idDocument = sqlite3_last_insert_rowid(database);

    return 0;
}**/

int addDocument(sqlite3 *database, int idUtilizator, const char *continut, const char *permisiuniEditare, const char *denumire, int *idDocument) {
    char sql_query[2000];

    snprintf(sql_query, sizeof(sql_query),
             "INSERT INTO Documente (ID_Utilizator, Continut, PermisiuniEditare, Denumire) "
             "VALUES (%d, '%s', '%s', '%s');",
             idUtilizator, continut, permisiuniEditare, denumire);

    if(sqlite3_exec(database, sql_query, 0, 0, 0) != SQLITE_OK) {
        fprintf(stderr, "Eroare la adăugarea documentului în baza de date.\n");
        return -1;
    }

    *idDocument = sqlite3_last_insert_rowid(database);

    return 0;
}

void addUserToDoc(const char *documentName, const char *username, sqlite3 *db) {
    char query[512];

    // Select la permisiunile actuale ale documentului
    snprintf(query, sizeof(query), "SELECT PermisiuniEditare FROM Documente WHERE NumeDocument='%s';", documentName);

    sqlite3_stmt *selectStatement;
    if (sqlite3_prepare_v2(db, query, -1, &selectStatement, NULL) == SQLITE_OK) {
        if (sqlite3_step(selectStatement) == SQLITE_ROW) {
            const char *currentPermissions = (const char *)sqlite3_column_text(selectStatement, 0);

            // Construire sir
            char newPermissions[1024];

            if (currentPermissions != NULL) {
                snprintf(newPermissions, sizeof(newPermissions), "%s; %s", currentPermissions, username);
            } else {
                snprintf(newPermissions, sizeof(newPermissions), "%s", username);
            }

            snprintf(query, sizeof(query), "UPDATE Documente SET PermisiuniEditare='%s' WHERE NumeDocument='%s';", newPermissions, documentName);

            sqlite3_stmt *updateStatement;
            if (sqlite3_prepare_v2(db, query, -1, &updateStatement, NULL) == SQLITE_OK) {
                if (sqlite3_step(updateStatement) == SQLITE_DONE) {
                    printf("[server] Utilizatorul %s a fost adăugat la permisiunile de editare pentru documentul %s.\n", username, documentName);
                } else {
                    fprintf(stderr, "[server] Eroare la actualizarea permisiunilor.\n");
                }
                sqlite3_finalize(updateStatement);
            } else {
                fprintf(stderr, "[server] Eroare la pregătirea interogării de actualizare.\n");
            }
        }
        sqlite3_finalize(selectStatement);
    } else {
        fprintf(stderr, "[server] Eroare la pregătirea interogării de selectare.\n");
    }
}

/// FUNCTII PT EDITARE DE DOCUMENTE
void addClientID(const char *documentName, const char *username, sqlite3 *db) {
    char query[512];

    // Cauta ID
    int userID = -1;
    snprintf(query, sizeof(query), "SELECT ID FROM Utilizatori WHERE Nume='%s';", username);

    sqlite3_stmt *selectStatement;
    if (sqlite3_prepare_v2(db, query, -1, &selectStatement, NULL) == SQLITE_OK) {
        if (sqlite3_step(selectStatement) == SQLITE_ROW) {
            userID = sqlite3_column_int(selectStatement, 0);
        }
        sqlite3_finalize(selectStatement);
    } else {
        fprintf(stderr, "[server] Eroare la pregătirea interogării de selectare pentru ID-ul utilizatorului.\n");
        return;
    }

    if (userID == -1) {
        fprintf(stderr, "[server] Utilizatorul %s nu există în baza de date.\n", username);
        return;
    }

    // Verificare stare document
    int existingClient1ID = -1;
    int existingClient2ID = -1;

    snprintf(query, sizeof(query), "SELECT Client1ID, Client2ID FROM Documente WHERE NumeDocument='%s';", documentName);

    if (sqlite3_prepare_v2(db, query, -1, &selectStatement, NULL) == SQLITE_OK) {
        if (sqlite3_step(selectStatement) == SQLITE_ROW) {
            existingClient1ID = sqlite3_column_int(selectStatement, 0);
            existingClient2ID = sqlite3_column_int(selectStatement, 1);
        }
        sqlite3_finalize(selectStatement);
    } else {
        fprintf(stderr, "[server] Eroare la pregătirea interogării de selectare pentru Client1ID și Client2ID.\n");
        return;
    }

    // Actualizare campuri
    if (existingClient1ID == -1) {
        snprintf(query, sizeof(query), "UPDATE Documente SET Client1ID=%d WHERE NumeDocument='%s';", userID, documentName);
    } else if (existingClient2ID == -1) {
        snprintf(query, sizeof(query), "UPDATE Documente SET Client2ID=%d WHERE NumeDocument='%s';", userID, documentName);
    } else {
        fprintf(stderr, "[server] Ambii clienți sunt deja prezenți pentru documentul %s.\n", documentName);
        return;
    }

    // Efectuare actualizare
    sqlite3_stmt *updateStatement;
    if (sqlite3_prepare_v2(db, query, -1, &updateStatement, NULL) == SQLITE_OK) {
        if (sqlite3_step(updateStatement) != SQLITE_DONE) {
            fprintf(stderr, "[server] Eroare la actualizarea Client1ID sau Client2ID.\n");
        }
        sqlite3_finalize(updateStatement);
    } else {
        fprintf(stderr, "[server] Eroare la pregătirea interogării de actualizare pentru Client1ID sau Client2ID.\n");
    }

    printf("[server] Client1ID sau Client2ID a fost actualizat pentru documentul %s.\n", documentName);
}

// void Edit();

/// FUNCTII PT DOWNLOAD DOCUMENT

int getDocumentContent(const char *documentName, char *contentBuffer, size_t bufferSize, sqlite3 *db) {
    char query[256];  // Ajustați dimensiunea în funcție de nevoi
    snprintf(query, sizeof(query), "SELECT Continut FROM Documente WHERE NumeDocument='%s';", documentName);

    sqlite3_stmt *statement;
    if (sqlite3_prepare_v2(db, query, -1, &statement, NULL) == SQLITE_OK) {
        if (sqlite3_step(statement) == SQLITE_ROW) {
            const char *content = (const char *)sqlite3_column_text(statement, 0);

            if (content != NULL) {
                // Verificăm dimensiunea bufferului pentru a evita buffer overflow
                size_t contentLength = sqlite3_column_bytes(statement, 0);
                if (contentLength < bufferSize) {
                    snprintf(contentBuffer, bufferSize, "%s", content);
                } else {
                    fprintf(stderr, "[server] Dimensiunea bufferului nu este suficientă pentru conținutul documentului.\n");
                    return 0;
                }
            } else {
                fprintf(stderr, "[server] Conținutul documentului este NULL.\n");
            }
        } else {
            fprintf(stderr, "[server] Nu s-a găsit documentul cu numele: %s\n", documentName);
            return 0;
        }
        sqlite3_finalize(statement);
    } else {
        fprintf(stderr, "[server] Eroare la pregătirea interogării.\n");
        return 0;
    }

    return 1;
}

void downloadDocument(int clientID, const char *documentName) {

    sqlite3 *db;
    if(sqlite3_open("server_data.db", &db) != SQLITE_OK) {
        fprintf(stderr, "[server] Nu s-a putut deschide baza de date!\n");
        return;
    }

    char currentContent[1024];

    if(getDocumentContent(documentName, currentContent, sizeof(currentContent), db)) {
        // Trimite continutul documentului inapoi la client
        if(write(clientID, currentContent, sizeof(currentContent)) <= 0) {
                fprintf(stderr, "[server] Eroare la trimiterea conținutului documentului la client.\n");
        }
        else {
            fprintf(stderr, "[server] Document not found: %s\n", documentName);
        }
    }

    sqlite3_close(db);
}


/**
void adaugaDocumente(sqlite3 *database) {
    // Adaugă documente pentru primul utilizator (ID_Utilizator = 1)
    addDocument(db, 1, "Acesta e un text scris de utilizatori in document.");
    addDocument(db, 1, "Acesta e al doilea text pentru testare.");

    // Adaugă document pentru al doilea utilizator (ID_Utilizator = 2)
    addDocument(db, 2, "Text adaugat de un alt user, eventual impreuna cu un alt user.");
}*/
/*
int stergeDocumente(sqlite3 *db) {
    char sql_query[] = "DELETE FROM Documente;";
    char reset_sequence_query[] = "DELETE FROM sqlite_sequence WHERE name='Documente';";

    if (sqlite3_exec(db, sql_query, 0, 0, 0) != SQLITE_OK) {
        fprintf(stderr, "Eroare la ștergerea documentelor din baza de date.\n");
        return -1;
    }

    if (sqlite3_exec(db, reset_sequence_query, 0, 0, 0) != SQLITE_OK) {
        fprintf(stderr, "Eroare la resetarea secvenței ID-urilor.\n");
        return -1;
    }

    return 0;
}
*/
/////

void addDocToXML(const char *username, int documentID) {
    // Deschide fișierul XML în modul de citire
    FILE *file = fopen("documente.xml", "r");
    if (file == NULL) {
        fprintf(stderr, "Nu am putut deschide fisierul XML.\n");
        return;
    }

    // Citeste continutul fisierului in buffer
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(file_size + 1);
    fread(buffer, 1, file_size, file);
    fclose(file);

    // Adauga noul document in buffer
    time_t current_time;
    struct tm *local_time;

    time(&current_time);
    local_time = localtime(&current_time);

    char created[20], last_modified[20];
    strftime(created, sizeof(created), "%d.%m.%Y %H:%M", local_time);
    strcpy(last_modified, created);

    char documentString[200];
    snprintf(documentString, sizeof(documentString),
             "        <document>\n"
             "            <title>Untitled%d</title>\n"
             "            <file_id>%d</file_id>\n"
             "            <location>home_dir</location>\n"
             "            <created>%s</created>\n"
             "            <last_modified>%s</last_modified>\n"
             "        </document>\n",
             documentID, documentID, created, last_modified);

    // Cauta pozitia unde trebuie adaugat noul document
    char *position = strstr(buffer, username);
    if (position == NULL) {
        fprintf(stderr, "Nu s-a gasit utilizatorul in fisierul XML.\n");
        free(buffer);
        return;
    }

    // Aduaga documentString in buffer
    size_t offset = position - buffer;
    size_t newSize = file_size + strlen(documentString);
    buffer = realloc(buffer, newSize + 1);

    memmove(buffer + offset + strlen(documentString), buffer + offset, file_size - offset);
    memcpy(buffer + offset, documentString, strlen(documentString));

    // Deschide fisierul XML in modul de scriere si rescrie continutul
    file = fopen("documente.xml", "w");
    if (file == NULL) {
        fprintf(stderr, "Nu am putut deschide fisierul XML pentru scriere.\n");
        free(buffer);
        return;
    }

    fwrite(buffer, 1, newSize, file);
    fclose(file);

    free(buffer);
}

int resetareIdUtilizatori(sqlite3 *db) {
    char *error = 0;
    int rc;

    // Șterge toate înregistrările din tabela "Utilizatori"
    const char *deleteAllUsersSQL = "DELETE FROM Utilizatori;";
    rc = sqlite3_exec(db, deleteAllUsersSQL, 0, 0, &error);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Eroare la ștergerea înregistrărilor din tabela Utilizatori! %s\n", error);
        sqlite3_free(error);
        return rc;
    }

    // Resetează secvența pentru coloana ID
    const char *resetSequenceSQL = "UPDATE sqlite_sequence SET seq = 0 WHERE name = 'Utilizatori';";
    rc = sqlite3_exec(db, resetSequenceSQL, 0, 0, &error);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Eroare la resetarea secvenței pentru tabela Utilizatori! %s\n", error);
        sqlite3_free(error);
        return rc;
    }

    printf("Toate înregistrările din tabela Utilizatori au fost șterse și id-urile au fost resetate.\n");
    return SQLITE_OK;
}
/////

int SqlDatabase() {
    sqlite3 *db;
    char *error = 0;

    int rc = sqlite3_open("server_data.db", &db);
    if(rc) {
        fprintf(stderr, "Nu s-a putut deschide baza de date! %s\n", sqlite3_errmsg(db));
        return rc;
    }

    // Crearea tabelului "Utilizatori" dacă nu există deja
    const char *createUsersTableSQL =
        "CREATE TABLE IF NOT EXISTS Utilizatori ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "Nume TEXT NOT NULL, "
        "Parola TEXT NOT NULL, "
        "Status INTEGER NOT NULL, "
        "ClientID INTEGER DEFAULT -1);";

    rc = sqlite3_exec(db, createUsersTableSQL, 0, 0, &error);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "Eroare la crearea tabelului Utilizatori! %s\n", error);
        sqlite3_free(error);
        sqlite3_close(db);
        return rc;
    }

    // Crearea tabelului "Documente" dacă nu există deja
    const char *createDocumentsTableSQL =
        "CREATE TABLE IF NOT EXISTS Documente ("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
        "ID_Utilizator INTEGER NOT NULL, "
        "Continut TEXT, "
        "FOREIGN KEY (ID_Utilizator) REFERENCES Utilizatori(ID));";

    rc = sqlite3_exec(db, createDocumentsTableSQL, 0, 0, &error);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "Eroare la crearea tabelului Documente: %s\n", error);
        sqlite3_free(error);
        sqlite3_close(db);
        return rc;
    }

     // Interogare pentru a obține toate rândurile din tabelul "Utilizatori"
    const char *selectUsersSQL = "SELECT * FROM Utilizatori;";
    rc = sqlite3_exec(db, selectUsersSQL, AfisareInterogari, (void*)"Utilizatori", &error);

    if(rc != SQLITE_OK) {
        fprintf(stderr, "Eroare la interogarea Utilizatori: %s\n", error);
        sqlite3_free(error);
        sqlite3_close(db);
        return rc;
    }

    rc = resetareIdUtilizatori(db);
    //if(rc == 0)

    // stergeDocumente(db);
    // adaugaDocumente(db);

    // Închiderea bazei de date SQLite
    sqlite3_close(db);
    return 0;
}



int main()
{
    struct sockaddr_in server; // structura folosita de server
    struct sockaddr_in from;
    int sd;                   //descriptorul de socket
    pthread_t th[100];        //Identificatorii thread-urilor care se vor crea
    int i = 0;

    /* crearea unui socket */
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[server]Eroare la socket().\n");
        return errno;
    }
    /* utilizarea optiunii SO_REUSEADDR */
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    /* pregatirea structurilor de date */
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

    /* atasam socketul */
    if(bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if(listen(sd, 2) == -1)
    {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    /* servim in mod concurent clientii...folosind thread-uri */
    while(1)
    {
        int client;
        thData *td; //parametru functia executata de thread
        int length = sizeof(from);

        printf("[server]Asteptam la portul %d...\n", PORT);
        fflush(stdout);

        /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[server]Eroare la accept().\n");
            continue;
        }

        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &treat, td);
    } //while
}

static void *treat(void *arg) {
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[Thread %d] Asteptam comanda...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());

    // Asociere clientID pentru noul client
    tdL.clientID = numClients + 1;

    // Adăugare informații despre noul client în array
    pthread_mutex_lock(&clientsMutex);
    if (numClients < MAX_CLIENTS) {
        clients[numClients].idThread = tdL.idThread;
        clients[numClients].cl = tdL.cl;
        clients[numClients].clientID = tdL.clientID;
        numClients++;
    }
    pthread_mutex_unlock(&clientsMutex);

    raspunde((struct thData *)&tdL);

    // Am terminat cu acest client, inchidem
    close(((intptr_t)arg));
    removeClient(tdL.idThread);

    return NULL;
}

/**
static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[Thread %d] Asteptam commanda...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());

    // adaugare informatii despre noul client in array
    for(int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].idThread == 0) { // cauta slot liber in array
            clients[i].idThread = tdL.idThread;
            clients[i].cl = tdL.cl;
            break; // iesi din bucla dupa ce ai gasit slot
        }
    }


    raspunde((struct thData *)arg);

    // am terminat cu acest client, inchidem conexiunea
    close(((intptr_t)arg));
    removeClient(tdL.idThread);
    return (NULL);
}**/

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


char mesaj[100];

///


// FUNCTIILE PT HASHING-UL PAROLEI
// fct de rotatie spre dreapta
uint32_t rightRotate(uint32_t value, int shift) {
    return (value >> shift) | (value << (32 - shift));
}

// fct de transformare - pt fctia ce descrie alg. sha256
void sha256Transform(uint32_t state[8], const unsigned char block[SHA256_BLOCK_SIZE]) {

    // SHA-256 constante
    static const uint32_t k[] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    };

    uint32_t W[64]; //W - blocul pt mesaj

    for(int t = 0; t < 16; t++) {
        W[t] = (block[t * 4] << 24) | (block[t * 4 + 1] << 16) | (block[t * 4 + 2] << 8) | block[t * 4 + 3];
    } // copiere in blocul W

    for(int t = 16; t < 64; t++) {
        uint32_t s0 = rightRotate(W[t - 15], 7) ^ rightRotate(W[t - 15], 18) ^ (W[t - 15] >> 3);
        uint32_t s1 = rightRotate(W[t - 2], 17) ^ rightRotate(W[t - 2], 19) ^ (W[t - 2] >> 10);
        W[t] = W[t - 16] + s0 + W[t - 7] + s1;
    } // extinderea dimensiunii blocului

    // initializare cu valorile de hash
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    // compresie
    for(int t = 0; t < 64; t++) {
        uint32_t S1 = rightRotate(e, 6) ^ rightRotate(e, 11) ^ rightRotate(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + k[t] + W[t];
        uint32_t S0 = rightRotate(a, 2) ^ rightRotate(a, 13) ^ rightRotate(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    // update-ul valorilor de hash
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// Algoritmul sha256 - functia de transformare a parolei
void sha256(const unsigned char *message, size_t length, unsigned char hash[32]) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    }; //valori initiale de hash

    for(size_t i = 0; i < length / SHA256_BLOCK_SIZE; i++) {
        sha256Transform(state, message + i * SHA256_BLOCK_SIZE);
    } ///transformarea fiecaruki bloc


    unsigned char block[SHA256_BLOCK_SIZE];
    size_t lastBlock = length % SHA256_BLOCK_SIZE;
    size_t bitLength = length * 8;

    memcpy(block, message + length - lastBlock, lastBlock); // copierea mesajului

    block[lastBlock++] = 0x80; //add unui bit de 1

    if(lastBlock > SHA256_BLOCK_SIZE - 8) {
        memset(block + lastBlock, 0, SHA256_BLOCK_SIZE - lastBlock);
        sha256Transform(state, block);
        memset(block, 0, SHA256_BLOCK_SIZE - 8);
    } else {
        memset(block + lastBlock, 0, SHA256_BLOCK_SIZE - 8 - lastBlock);
    } // umplere cu 0 pana la utimiii 8 bytes

    for(int i = 0; i < 8; i++) {
        block[SHA256_BLOCK_SIZE - 8 + i] = (bitLength >> ((7 - i) * 8)) & 0xFF;
    }

    sha256Transform(state, block); // transformarea finala

    // copierea hash-ului final in output
    for(int i = 0; i < 32; i++) {
        hash[i] = (state[i / 4] >> ((3 - i % 4) * 8)) & 0xFF;
    }
}

// FUNCTII DE VERIFICARE

// Funcție pentru a converti un șir de octeți într-un șir de caractere hexazecimale
void bytesToHex(const unsigned char *bytes, int length, char *hex) {
    for (int i = 0; i < length; i++) {
        sprintf(hex + 2 * i, "%02x", bytes[i]);
    }
}

int verifyPassword(const char username[], const char enteredPassword[], unsigned char storedHashedPasswordHex[], sqlite3 *database) {

    sqlite3_stmt *stmt;
    const char *sql_query = "SELECT Parola FROM Utilizatori WHERE Nume = ?;";

    // pregatirea interogarii
    if(sqlite3_prepare_v2(database, sql_query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error at preparing the query.\n");
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC); // parametrul pt interogare

    int query_rs = sqlite3_step(stmt);

    //sqlite3_finalize(stmt); //finalizare interogare

    // verificarea rezultatului si obtinerea hash-ului stocat
    if(query_rs == SQLITE_ROW) {
        const unsigned char *dbHash = sqlite3_column_text(stmt, 0);

        // Compararea sirurilor de octeti
        if(memcmp(dbHash, storedHashedPasswordHex, strlen(storedHashedPasswordHex)) == 0) {
            // Parolele coincid
            ////
            printf("Parola stocata: %s\n", dbHash);
            printf("Parola introdusa: %s\n", storedHashedPasswordHex);
            /////
            sqlite3_finalize(stmt);
            return 1;
        } else {
            // Parolele nu coincid
            sqlite3_finalize(stmt);
            return 0;
        }
    } else if(query_rs != SQLITE_DONE) {
        fprintf(stderr, "Error at executing the query.\n");
        sqlite3_finalize(stmt);
        return -1;
    } else {
        // Utilizatorul nu a fost gasit
        sqlite3_finalize(stmt);
        return 0;
    }
    sqlite3_finalize(stmt); //finalizare interogare
    // inchidem baza de date
    ///sqlite3_close(db);
}

int verifyUserStatus(const char *userName, int *clientid, sqlite3 *database) {
    int clientId = -1;

    sqlite3_stmt *stmt;

    // construirea interogarii
    char sql_query[100];

    if (userName != NULL) {
        snprintf(sql_query, sizeof(sql_query), "SELECT Status FROM Utilizatori WHERE Nume = '%s';", userName);
    } else {
        fprintf(stderr, "[server]Erorr: Please provide a username.\n");
        return -1;
    }


    // pregateste si executa interogarea
    if (sqlite3_prepare_v2(database, sql_query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[server]Eroare la pregătirea interogării.\n");
        sqlite3_finalize(stmt);
        return -1;
    }

    int status = -1; // val de inceput :  utilizatorul nu a fost gasit

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // obtine valoarea coloanei status din tabel
        status = sqlite3_column_int(stmt, 0);
    }

    // finalizarea interogarii
    sqlite3_finalize(stmt);
    /// inchidem baza de date
    //sqlite3_close(db);

    return status;
}

int verifyUsername(char user[], sqlite3 *database) {
    sqlite3_stmt *stmt;
    const char *sql_query = "SELECT * FROM Utilizatori WHERE Nume = ?;";

    // pregateste interogarea
    if (sqlite3_prepare_v2(database, sql_query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error ar preparing the query.\n");
        return -1;
    }

    // Parametru pentru interogare
    sqlite3_bind_text(stmt, 1, user, -1, SQLITE_STATIC);

    // Executa interogarea
    int result = sqlite3_step(stmt);

    // Finalizeaza interogarea
    sqlite3_finalize(stmt);

    if (result == SQLITE_ROW) {
        // Utilizatorul a fost găsit
        return 1;
    } else if (result == SQLITE_DONE) {
        // Utilizatorul nu a fost găsit
        return 0;
    } else {
        // Eroare la executia interogarii
        fprintf(stderr, "Error at executing the query.\n");
        return -1;
    }
}

int Signup(char comm[], sqlite3 *database) {

    char username[30]; char parola[30];

    // PRELUARE DATE INTRODUSE
    const char *delim = " ";
    char *token;

    token = strstr(comm, delim); //prima aparitie a spatiului

    if(token != NULL) {
        // avanseaza peste spatii
        token += strlen(delim);
        // gaseste urmatoarea aparitie a spatiului
        char *usernameEnd = strstr(token, delim);

        if(usernameEnd != NULL) {
            // copiere username in sir
            strncpy(username, token, usernameEnd - token);
            username[usernameEnd - token] = '\0';

            // avanseaza peste spatii pana la parola
            token = usernameEnd + strlen(delim);
            strcpy(parola,token);
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    // VERIFICARE VALIDITATE DATE
    // VALIDITATE USERNAME
    if(!username || username[0] == '\0' || strlen(username)<2 || strlen(username)>30) {
        return -1;
    } else {
        for(int i = 0; username[i] != '\0'; i++) {
            if(!isalnum(username[i])) {
                return -1;
            }
        }
    }

    // VALIDITATE PAROLA
    if(!parola || parola[0] == '\0' || strlen(parola)<5 || strlen(parola)>30) {
        return -1;
    } else {
        for(int i = 0; parola[i]!='\0'; i++) {
            if(parola[i] == ' ') {
                return -1;
            }
        }
    }

    // DESCHIDERE BAZA DE DATE PT VERIFICARE
    if(sqlite3_open("server_data.db", &database) != SQLITE_OK) {
        fprintf(stderr, "Nu s-a putut deschide baza de date!\n");
        return -1;
    }


    // VERIFICARE DACA EXISTA DEJA UTILIZATOR CU ACELASI USERNAME - n-ar trebui sa se poata adauga
    int usernameExists = verifyUsername(username, database);
    if(usernameExists == 1) return 0;

    // HASHING PAROLA
    printf("Parola este: %s\n", parola);
    unsigned char hashedPassword[32] = "";
    sha256((const unsigned char *)parola, strlen(parola), hashedPassword);

    unsigned char parolaHashed[64] = "";
    snprintf(parolaHashed, sizeof(parolaHashed), "%s", hashedPassword);
    bytesToHex(hashedPassword, sizeof(hashedPassword), parolaHashed);
    printf("Hashed Password: %s\n", parolaHashed);

    // acum avem username[30] si parola parolaHashed[64] - trebuie doar sa le introducem in tabel impreuna

    // inserarea
    const char *insertUserQuery = "INSERT INTO Utilizatori (Nume, Parola, Status) VALUES (?, ?, ?);";

    sqlite3_stmt *stmt; // pentru instructiunea preparata

    // pregatirea instructiunii sql
    int rc = sqlite3_prepare_v2(database, insertUserQuery, -1, &stmt, 0);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "Error at preparing the SQL instruction! %s\n", sqlite3_errmsg(database));
        //sqlite3_close(database);
        sqlite3_finalize(stmt);
        return -1;
    }

    // legarea valorilor la parametri
    sqlite3_bind_text(stmt, 1, username, strlen(username), SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, parolaHashed, sizeof(parolaHashed), SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, 0);  // campul Status

    rc = sqlite3_step(stmt);
    if(rc == SQLITE_DONE) {
        printf("Utilizatorul a fost înregistrat cu succes!\n");
        // s-a facut adaugarea cu succes => return 1
        sqlite3_finalize(stmt);
        return 1;
    } else if(rc == SQLITE_CONSTRAINT) {
        printf("Utilizatorul există deja!\n");
        // return 0 - daca exista - deci nu e eroare la signup, doar exista utilizatorul
        sqlite3_finalize(stmt);
        return 0;
    } else {
        fprintf(stderr, "Eroare la execuția instrucțiunii SQL! %s\n", sqlite3_errmsg(database));
        sqlite3_finalize(stmt);
        return -1;
    }

    // finalizarea instructiunii
    sqlite3_finalize(stmt);
}

int searchUserByClientID(int clientid, char *foundUsername, size_t bufferSize, sqlite3 *database) {
    //interogarea
    char query[100];
    snprintf(query, sizeof(query), "SELECT Nume FROM Utilizatori WHERE Status = 1 AND ClientID = %d;", clientid);

    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(database, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error at preparing the query.\n");
        return -1;
    }

    int rs = sqlite3_step(stmt);

    if(rs == SQLITE_ROW) {
        // preluare nume din interogare
        const char *username = sqlite3_column_text(stmt, 0);
        strcpy(foundUsername, "");
        strncpy(foundUsername, username, bufferSize - 1);
        foundUsername[bufferSize - 1] = '\0';

        printf("[server]Utilizatorul conectat cu ClientID-ul %d este: %s\n", clientid, username);
        sqlite3_finalize(stmt);
        return 1;
    } else {
        printf("[server]Nu există utilizator conectat cu ClientID-ul %d.\n", clientid);
        sqlite3_finalize(stmt);
        return 0;
    }
}

int searchClientIDbyUsername(const char *username, sqlite3 *database) {
    char sql_query[2000];
    int clientID = -1;  // Valoare implicită dacă utilizatorul nu este găsit

    // Construiește interogarea SQL pentru a obține ClientID-ul utilizatorului
    snprintf(sql_query, sizeof(sql_query),
             "SELECT ClientID FROM Utilizatori WHERE Nume = '%s';", username);

    // Execută interogarea și obține rezultatele
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(database, sql_query, -1, &stmt, 0) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            // Obține ClientID-ul din rezultatele interogării
            clientID = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);  // Eliberează resursele interogării
    }

    return clientID;
}

/**
int searchUserByClientID(int clientID, char *foundUsername, size_t bufferSize) {
    for(int i = 0; i < numClients; ++i) {
        if(clients[i].clientID == clientID) {
            // Am gasit utilizatorul
            strncpy(foundUsername, clients[i].username, bufferSize - 1);
            foundUsername[bufferSize - 1] = '\0'; // asigura terminarea sirului
            printf("[server]Utilizatorul conectat cu ClientID-ul %d este: %s\n", clientID, foundUsername);
            return 1;
        }
    }
    // Utilizatorul nu a fost gasit
    return 0;
} */

/*
int searchClientIDbyUsername(const char *username) {
    for(int i = 0; i < numClients; ++i) {
        if(strcmp(clients[i].username, username) == 0) {
            int id = (int)clients[i].clientID;
            return id;
        }
    }
    return -1;  // dc nu a fost gasit niciun client cu acel nume de utilizator
}**/

/**
int searchClientIDbyUsername(const char *username, sqlite3 *database) {

    // Interogare pentru a găsi ClientID-ul asociat cu un username
    char query[100];
    snprintf(query, sizeof(query), "SELECT ClientID FROM Utilizatori WHERE Nume = ?;");

    sqlite3_stmt *stmt;
    if(sqlite3_prepare_v2(database, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparing the query.\n");
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    // Executia
    int rs = sqlite3_step(stmt);

    // Finalizare inerogare
    sqlite3_finalize(stmt);

    if(rs == SQLITE_ROW) {
        // ClientID găsit
        return sqlite3_column_int(stmt, 0);
    } else {
        // Utilizatorul nu a fost gasit..
        return -1;
    }
}**/

int getUserIdByName(const char *username, sqlite3 *database) {
    char query[100];
    snprintf(query, sizeof(query), "SELECT ID FROM Utilizatori WHERE Nume = ?;");

    sqlite3_stmt *stmt;

    // TODO : solve error

    // Pregateste interogarea
    if (sqlite3_prepare_v2(database, query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparing the query.\n");
        return -1;
    }

    // Legare parametru
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    // Executie
    int result = sqlite3_step(stmt);

    // variabila pt stocare id
    int user_id = -1;
    if (result == SQLITE_ROW) {
        // Obține ID-ul utilizatorului din coloana corespunzătoare
        user_id = sqlite3_column_int(stmt, 0);
    }

    // finalizare query
    sqlite3_finalize(stmt);

    return user_id;
}


int Login(char comm[], int clientid, sqlite3 *database) {

    char username[30]; char parola[30];

    // PRELUARE DATE INTRODUSE
    const char *delim = " ";
    char *token;

    token = strstr(comm, delim); //prima aparitie a spatiului

    if(token != NULL) {

        // avanseaza peste spatii
        token += strlen(delim);
        // gaseste urmatoarea aparitie a spatiului
        char *usernameEnd = strstr(token, delim);

        if(usernameEnd != NULL) {
            // copiere username in sir
            strncpy(username, token, usernameEnd - token);
            username[usernameEnd - token] = '\0';

            // avanseaza peste spatii pana la parola
            token = usernameEnd + strlen(delim);
            strcpy(parola,token);
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    // VERIFICARE VALIDITATE DATE
    // VALIDITATE USERNAME
    if (!username || username[0] == '\0' || strlen(username)<2 || strlen(username)>30) {
        return -1;
    } else {
        for(int i = 0; username[i] != '\0'; i++) {
            if(!isalnum(username[i])) {
                return -1;
            }
        }
    }
    // VALIDITATE PAROLA
    if (!parola || parola[0] == '\0' || strlen(parola)<5 || strlen(parola)>30) {
        return -1;
    } else {
        for(int i = 0; parola[i]!='\0'; i++) {
            if(parola[i] == ' ') {
                return -1;
            }
        }
    }

    // INCEPERE PROCES PROPRIU-ZIS DE LOGIN
    bool valid_username = 0;
    bool valid_password = 0;


    // DESCHIDERE BAZA DE DATE PT VERIFICARE
    if(sqlite3_open("server_data.db", &database) != SQLITE_OK) {
        fprintf(stderr, "Nu s-a putut deschide baza de date!\n");
        return -1;
    }

    // VERIFICAREA USERNAME-ULUI SI A PAROLEI + HASH !!
    int usernameExists = verifyUsername(username, database);
    if(usernameExists == 1) {
        valid_username = 1;
        printf("Utilizatorul cu numele %s exista.\n", username);
    } else return -1;

    // HASHING parola pentru verificare
    unsigned char hashedPwd[32] = "";

    printf("Parola este: %s\n", parola);
    sha256((const unsigned char *)parola, strlen(parola), hashedPwd);

    char parolaHashed[64] = "";
    bytesToHex(hashedPwd, sizeof(hashedPwd), parolaHashed);
    printf("Hashed Password: %s\n", parolaHashed);

    // VERIFICARE PAROLA
    int pwdMatches = verifyPassword(username, parola, parolaHashed, database);
    if(pwdMatches == 1) {
        valid_password = 1;
        printf("Parola este corecta.\n");
    } else {
        printf("Parola este incorecta.\n");
        return -1;
    }

    // VERIFICARE + SETARE STATUS - daca e deja 1 - EROARE, daca e 0 il setam la 1
    int clientID;
    int current_status = verifyUserStatus(username, &clientID, database);
    if(current_status == 1) {
        return 0;
    }
    sqlite3_stmt *stmt;
    const char *sql_query = "UPDATE Utilizatori SET Status = 1, ClientID = ? WHERE Nume = ?;";

    // pregateste interogarea
    if (sqlite3_prepare_v2(database, sql_query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error at preparing the query.\n");
        return -1;
    }

    // parametrul pt query
    sqlite3_bind_int(stmt, 1, clientid);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_STATIC);

    int query_rs = sqlite3_step(stmt);
    //sqlite3_finalize(stmt); //finalizarea interogarii

    if(query_rs == SQLITE_DONE) {
        // actualizarea s-a realizat cu succes
        printf("Status for user %s was updated to 1 (logged in).\n", username);
        sqlite3_finalize(stmt); //finalizarea interogarii

        //// test
        int id_Client = searchClientIDbyUsername(username, database);
        printf("Valoare ID Client noua : %d\n", id_Client);

        /////
        return 1;
    } else {
        // eroare la executarea interogarii
        fprintf(stderr, "Error at executing the query.\n");
        sqlite3_finalize(stmt); //finalizarea interogarii
        return -1;
    }
    sqlite3_finalize(stmt); //finalizarea interogarii
}

int Logout(const char *username, sqlite3 *database) {

    // Actualizare Status și ClientID pentru utilizatorul deconectat
    const char *updateQuery = "UPDATE Utilizatori SET Status = 0, ClientID = -1 WHERE Nume = ?;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(database, updateQuery, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error preparing the update query.\n");
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    int rs = sqlite3_step(stmt);

    sqlite3_finalize(stmt);

    if(rs == SQLITE_DONE) {
        printf("[server]Utilizatorul %s a fost deconectat cu succes.\n", username);
        return 1;
    } else {
        fprintf(stderr, "[server]Eroare la actualizarea datelor utilizatorului %s. %s\n", username, sqlite3_errmsg(database));
        return -1;
    }
}

/// verificare denumire pentru create_document
bool VerificareDenumireInvalida(const char* Nume) {

    if(strlen(Nume) > 30 || strlen(Nume) < 1) return 0;
    while(*Nume) {
        if(!(isalnum(*Nume) || *Nume == '-' || *Nume == '_')) {
            return 0;
        }
        Nume++;
    }
    return 1;
}

struct modify_text {
    int user_id;
    int operation_type;
    int cursor_position;
    int length;
    char data[500];
};


////////
int findUser(sqlite3 *database, int idUtilizator, char *username) {
    char sql_query[200];
    sqlite3_stmt *stmt;

    // Crează și pregătește interogarea SQL
    snprintf(sql_query, sizeof(sql_query),
             "SELECT Nume FROM Utilizatori WHERE ID = %d;", idUtilizator);

    if (sqlite3_prepare_v2(database, sql_query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Eroare la pregătirea interogării.\n");
        sqlite3_finalize(stmt);
        return -1;
    }

    // Execută interogarea și atribuie numele utilizatorului
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *nume = (const char *)sqlite3_column_text(stmt, 0);
        strncpy(username, nume, 30);
        username[29] = '\0'; // Asigură că șirul este null-terminated
        sqlite3_finalize(stmt);
        return 0; // Succes
    } else {
        printf("Utilizatorul cu ID-ul %d nu a fost găsit.\n", idUtilizator);
        sqlite3_finalize(stmt);
        return -1; // Eșec
    }
}


//// FUNCTII PENTRU GASIRE DOCUMENT

int findDocument(sqlite3 *database, int idUtilizator, const char *denumireDocument) {
    char sql_query[2000];

    // Interogare pentru a găsi documentul
    snprintf(sql_query, sizeof(sql_query),
             "SELECT COUNT(*) FROM Documente WHERE ID_Utilizator = %d AND Denumire = '%s';",
             idUtilizator, denumireDocument);

    sqlite3_stmt *stmt;

    // Pregătește interogarea
    if (sqlite3_prepare_v2(database, sql_query, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Eroare la pregătirea interogării.\n");
        return 0;
    }

    // Execută interogarea
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // Obține rezultatul COUNT(*)
        int count = sqlite3_column_int(stmt, 0);

        sqlite3_finalize(stmt);  // Finalizează interogarea

        // Returnează 1 dacă există cel puțin un document, altfel 0
        return (count > 0) ? 1 : 0;
    }

    // Eroare în timpul execuției interogării
    sqlite3_finalize(stmt);  // Finalizează interogarea
    return 0;
}

/// FUNCTIILE PT LUCRUL IN SESIUNE


/// FUNCTIILE PT EDITARE DOCUMENT



/// FUNCTIILE PT DOWNLOAD


void procesareComenzi(const char *cmd, int client_id) {

    /*    */
    /* TODO : PROCESARE PROVIZORIE PENTRU TOATE COMENZILE */
    /*    */

    char trimmedCommand[100];
    strcpy(trimmedCommand, cmd);
    strtrim(trimmedCommand);

    sqlite3 *db;
    /// DESCHIDERE BAZA DE DATE
    if(sqlite3_open("server_data.db", &db) != SQLITE_OK) {
        fprintf(stderr, "[server]Nu s-a putut deschide baza de date!\n");
        return;
    } else {
        printf("Am deschis baza de date in procesare comenzi.\n");
    }

    if(strncmp(trimmedCommand, "login", 5) == 0) {
        printf("[server]ID-ul clientului care face cererea curenta: %d\n", client_id);

        int logged_in = Login(trimmedCommand, client_id, db);
        if(logged_in == 0) {
            strcpy(mesaj, "");
            strcpy(mesaj, "User already logged in!");
            mesaj[strlen(mesaj)] = '\0';
        }
        else if(logged_in == 1) {
                strcpy(mesaj, "");
                strcpy(mesaj, "Login succesful!");
                mesaj[strlen(mesaj)] = '\0';
        } else {
                strcpy(mesaj, "");
                strcpy(mesaj, "Login unsuccesful! Try again!(format: login [username] [password])");
                mesaj[strlen(mesaj)] = '\0';
        }
    }
    else if(strncmp(trimmedCommand, "logout", 6) == 0) {

        printf("A intrat in else if de la logout\n");
        printf("ID-ul clientului care face cererea curenta: %d\n", client_id);

        /// functia de logout

        char userOnClient[30] = "";
        if(searchUserByClientID(client_id, userOnClient, sizeof(userOnClient), db)) {

            printf("[logout]User on client %d is %s.\n", client_id, userOnClient);

            // utilizatorul a fost gasit deci poate continua procesul de logout

            int logged_out = Logout(userOnClient, db);
            if(logged_out == 1) {
                strcpy(mesaj, "");
                strcpy(mesaj, "You're logged out!");
                mesaj[strlen(mesaj)] = '\0';
            } else {
                    strcpy(mesaj, "");
                    strcpy(mesaj, "Logout unsuccesful! Try again!");
                    mesaj[strlen(mesaj)] = '\0';
                }
            }
            else {
                // nu e niciun utilizator care trebuie deconectat de la server
                strcpy(mesaj, "");
                strcpy(mesaj, "Error at logout! No user is logged in on this client!");
                mesaj[strlen(mesaj)] = '\0';
            }
    }
    else if(strncmp(trimmedCommand, "signup", 6) == 0) {
        int signup_status = Signup(trimmedCommand, db);
        if(signup_status == 1) {
            strcpy(mesaj, "");
            strcpy(mesaj, "Signup succesful!");
            mesaj[strlen(mesaj)] = '\0';
        } else if(signup_status == 0) {
            strcpy(mesaj, "");
            strcpy(mesaj, "Error at signup: Username already exists!");
            mesaj[strlen(mesaj)] = '\0';
        } else if(signup_status == -1){
            strcpy(mesaj, "");
            strcpy(mesaj, "Signup unsuccesful! Try again!(format: signup [username] [password])");
            mesaj[strlen(mesaj)] = '\0';
        }
    }
    else if(strncmp(trimmedCommand, "invite", 6) == 0) {
        char username[30]; int idUtilizator;
        strcpy(username, trimmedCommand+7); //memorare utilizator

        char msg[50];

        int clientID = searchClientIDbyUsername(username, db);
        snprintf(msg, sizeof(msg), "Invitation sent to %s(%d). Please wait...\n", username, clientID);

        strcpy(mesaj, "");
        strcpy(mesaj, msg);
        mesaj[strlen(mesaj)] = '\0';
    }
    /**
    else if(strncmp(trimmedCommand, "close_document", 14) == 0) {
        /// Editing = 0;

    }**/
    else if(strcmp(trimmedCommand, "close session") == 0) {
        strcpy(mesaj, "");
        strcpy(mesaj, "stop");
        mesaj[strlen(mesaj)] = '\0';
    }/**
    else if(strncmp(trimmedCommand, "modify_docdata", 14) == 0) {
        /// const char* xmlFilePath = "documente.xml";
        // XMLFileParser(xmlFilePath); //testare
    }**/
    else if(strncmp(cmd, "create_document", 15) == 0) {
        // preluare idUtilizator
        char nume[30] = ""; int idUtilizator;
        strcpy(nume, cmd+16);

        printf("Document name: %s.\n", nume);

        // preluare idUtilizator si verificare daca e logat cineva de pe thread-ul curent - daca da, id-ul acelui user va fi preluat ca idUtilizator
        int logged_in = 0; char logged_user[30] = "";

        logged_in = searchUserByClientID(client_id, logged_user, sizeof(logged_user), db);
        printf("Logged_user: %s\n", logged_user);

        idUtilizator = getUserIdByName(logged_user, db);
        printf("Exista utilizator conectat cu ID %d.\n", idUtilizator);

        if(logged_in == 1) {
            printf("[logged_in=1]Exista utilizator conectat cu ID %d.\n", idUtilizator);
        }

        if(logged_in == 1) {

            /** if(VerificareDenumireInvalida(nume) != 1) {
                strcpy(mesaj, "");
                strcpy(mesaj, "Invalid name! Please try again!");
                mesaj[strlen(mesaj)] = '\0';**/
            /** } else if(VerificareDenumireInvalida(nume) == 1) { **/
                printf("Denumire valida\n");

                // adaugare document
                int idDoc;
                char result[40];
                snprintf(result, sizeof(result), "%s;", logged_user);

                int doc_added = addDocument(db, idUtilizator, "", result, nume, &idDoc);

                if(doc_added == -1) {
                    strcpy(mesaj, "");
                    strcpy(mesaj, "No new document created!");
                    mesaj[strlen(mesaj)] = '\0';
                } else {
                    strcpy(mesaj, "");
                    strcpy(mesaj, "New document created!");
                    mesaj[strlen(mesaj)] = '\0';
                }
            }
        //}
        else {
            strcpy(mesaj, "");
            strcpy(mesaj, "Error at document creation: No user logged in!");
            mesaj[strlen(mesaj)] = '\0';
        }
    }
    else if(strncmp(cmd, "edit", 4) == 0) {

        /// preluare nume din comanda
        char nume[256];
        strcpy(nume, cmd+5);

        /// gasire username utilizator conectat
        char Username[30];
        searchUserByClientID(client_id, Username, sizeof(Username), db);

        /// incepere editare client
        // addClientID(const char *documentName, username,db);
        bool Editing = 1;

        /// incepere editare
        /// compunem mesaj special pentru client
        /// in client vom face schimbarile necesare ca sa preia toate modificarile de la tastatura
        strcpy(mesaj, "");
        strcpy(mesaj, "Editing document...\n");
        mesaj[strlen(mesaj)] = '\0';


        /**
        sqlite3 *db;
        /// DESCHIDERE BAZA DE DATE
        if(sqlite3_open("server_data.db", &db) != SQLITE_OK) {
            fprintf(stderr, "[server]Nu s-a putut deschide baza de date!\n");
            return;
        }

        resetareIdUtilizatori(db);


        sqlite3 *db;
        /// DESCHIDERE BAZA DE DATE
        if(sqlite3_open("server_data.db", &db) != SQLITE_OK) {
            fprintf(stderr, "[server]Nu s-a putut deschide baza de date!\n");
            return;
        }

        resetareIdUtilizatori(db); */
        /* int result = SqlDatabase();
        if(result == 0 && modif == 0) {
            strcpy(mesaj, "");
            strcpy(mesaj, "Operation completed! New database just dropped!");
            mesaj[strlen(mesaj)] = '\0';
        } else {
            strcpy(mesaj, "");
            strcpy(mesaj, "Error! The new database didn't drop!");
            mesaj[strlen(mesaj)] = '\0';
        } */

    }
    else if(strncmp(cmd, "download_document", 17) == 0)
    {
        /// Verificarea daca vreun user e logat
        int logged_in = 0; char logged_user[30] = "";

        logged_in = searchUserByClientID(client_id, logged_user, sizeof(logged_user), db);
        printf("Logged_user: %s\n", logged_user);

        int idUtilizator = getUserIdByName(logged_user, db);
        printf("Exista utilizator conectat cu ID %d.\n", idUtilizator);

        if(logged_in == 0) {
            /// INCHIDERE BAZA DE DATE
            sqlite3_close(db);
            printf("Test: Am inchis baza de date in procesare comenzi.\n");

            return;
        }

        /// Preluare nume
        char nume[30] = "";
        strcpy(nume, trimmedCommand+18);

        printf("Document name: %s.\n", nume);

        // Preluare username
        char username[30];


        /// gasire user in baza de date
        if(findUser(db, idUtilizator, username) == 0) {
            printf("Numele utilizatorului cu ID-ul %d este: %s\n", idUtilizator, username);
        }


        /// GASIRE DOCUMENT TINTA
        int doc_found = findDocument(db, idUtilizator, nume);
        if(doc_found == 1) {

            /// DACA A FOST GASIT - INCEPEM PROCESUL DE DOWNLOAD
            downloadDocument(client_id, nume);

        } else {
            strcpy(mesaj, "");
            strcpy(mesaj, "Download failed! Document was not found. (usage: download_document [name])");
            mesaj[strlen(mesaj)] = '\0';
        }
    }
    else if(strncmp(cmd, "find_document", 13) == 0)
    {
        /// verificarea daca vreun user e logat

        /// Verificarea daca vreun user e logat
        int logged_in = 0; char logged_user[30] = "";

        logged_in = searchUserByClientID(client_id, logged_user, sizeof(logged_user), db);
        printf("Logged_user: %s\n", logged_user);

        int idUtilizator = getUserIdByName(logged_user, db);
        printf("Exista utilizator conectat cu ID %d.\n", idUtilizator);

        if(logged_in == 0) {
            /// INCHIDERE BAZA DE DATE
            sqlite3_close(db);
            printf("Test: Am inchis baza de date in procesare comenzi.\n");

            return;
        }


        char nume[30] = "";
        // preluare nume
        strcpy(nume, trimmedCommand+14);

        printf("Document name: %s", nume);

        /// GASIRE DOCUMENT IN TABELA DOCUMENTE
        int found = findDocument(db, idUtilizator, nume);
        if(found == 0) {
            strcpy(mesaj, "");
            strcpy(mesaj, "Error! Document not found!");
            mesaj[strlen(mesaj)] = '\0';
        } else {
            if (findUser(db, idUtilizator, logged_user) == 0) {
                printf("Numele utilizatorului cu ID-ul %d este: %s\n", idUtilizator, logged_user);
            }
        }
    }
    /**
    else if(strcmp(cmd, "delete_document") == 0) {
        //stergereDocument();
    }**/

    /// INCHIDERE BAZA DE DATE
    sqlite3_close(db);
    printf("Test: Am inchis baza de date in procesare comenzi.\n");
}


void raspunde(void *arg)
{
    char command[100];
    struct thData tdL;
    tdL = *((struct thData *)arg);

    /* citim comenzile de la client */
    while(1) {

        if(read(tdL.cl, command, sizeof(command)) <= 0)
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Error at reading from client.\n");
            break;
        }

        // Procesare comanda
        printf("[Thread %d] Commmand was received...%s\n", tdL.idThread, command);

        /* pregatim mesajul de raspuns */

        //char mesaj[100];
        ////
        snprintf(mesaj, sizeof(mesaj), "Got the command: %s", command);
        //////

        printf("[Thread %d] Processing command...%s\n", tdL.idThread, mesaj);

        int clientID = (int)tdL.idThread;
        printf("ID client that did the request: %d\n", clientID);

        procesareComenzi(command, clientID);
        // printf("Mesaj dupa procesare: %s\n", mesaj);

        int result_search;
        /// Cazul pentru comenzi cu redirectionarea thread-ului
        if(strncmp(mesaj, "Invitation sent to", 18) == 0) {
            char user[30];
            char admin_user[30];
            int dest_clientID = -1;


            bool ok = 1;
           // Extragem informatiile direct in main
            const char *start = strstr(mesaj, "Invitation sent to ");
            if (start != NULL) {
                start += strlen("Invitation sent to ");
                const char *end = strchr(start, '(');
                if (end != NULL) {
                    size_t usernameLength = end - start;
                    strncpy(user, start, usernameLength);
                    user[usernameLength] = '\0';

                    if (sscanf(end + 1, "%d", &dest_clientID) == 1) {
                        // Extragere reusita
                        printf("Destination data: %s on %d.\n", user, dest_clientID);
                    } else {
                        printf("Eroare la citirea clientID-ului.\n");
                        ok = 0;
                    }
                } else {
                    printf("Nu s-a găsit deschiderea parantezei.\n");
                    ok = 0;
                }
            } else {
                printf("Mesajul nu conține secvența așteptată.\n");
                ok = 0;
            }

            sqlite3 *db;
            /// DESCHIDERE BAZA DE DATE
            if(sqlite3_open("server_data.db", &db) != SQLITE_OK) {
                fprintf(stderr, "[server]Nu s-a putut deschide baza de date!\n");
                return;
            }

            if(dest_clientID != -1) {
                    // construire mesaj
                    char inviteMsg[100];

                    searchUserByClientID(clientID, admin_user, sizeof(admin_user), db);

                    snprintf(inviteMsg, sizeof(inviteMsg), "User %s invited you to edit a document. Accept or decline?", admin_user);

                    /// trimite mesaj la client
                    sendMessageToClient(dest_clientID, inviteMsg);


                    /////
                    // Creează un thread pentru gestionarea comunicării între cei doi clienți
                    ClientPair* clientPair = (ClientPair*)malloc(sizeof(ClientPair));
                    clientPair->client1ID = tdL.cl;
                    clientPair->client2ID = dest_clientID;

                    pthread_t communicationThread;
                    pthread_create(&communicationThread, NULL, handleClientCommunication, (void*)clientPair);


                    // Trimite un mesaj de acceptare la clientul invitat
                    char acceptMsg[100];
                    snprintf(acceptMsg, sizeof(acceptMsg), "Invitation accepted. You are now connected with %s.", user);
                    sendMessageToClient(dest_clientID, acceptMsg);
                    ////

                } else {
                    // clientID-ul nu a fost gasit in array
                    strcpy(mesaj, "");
                    strcpy(mesaj, "Error at invite! The invited user was not found!\n");
                    mesaj[strlen(mesaj)] = '\0';
                }
            sqlite3_close(db);
        }

        /// remove client - dupa logout cu succes
        if(strcmp(mesaj, "You're logged out!") == 0) {
            removeClient(tdL.idThread);
        }

        // Trimite mesajul clientului curent // ex. "Invitation sent to user [username]!"
        if (write(tdL.cl, mesaj, sizeof(mesaj)) <= 0)
        {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Error at writing to client.\n");
            //break;
        }
        else {
            printf("[Thread %d] Message was succesfully received.\n", tdL.idThread);
            ////
            printf("Sent message: %s\n", mesaj);
            //////
        }

    }
    // Inchide conexiunea dupa ce bucla se termina
    close(tdL.cl);
}


