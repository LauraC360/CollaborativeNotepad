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

/* codul de eroare returnat de anumite apeluri */
extern int errno;

typedef struct thData {
    int idThread; //id-ul thread-ului tinut in evidenta de acest program
    int cl;       //descriptorul intors de accept
} thData;

bool Editing = 0;

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

sqlite3 *db;

int addDocument(sqlite3 *database, int idUtilizator, const char *continut) {
    char sql_query[2000];

    // Adaugă documentul în baza de date
    snprintf(sql_query, sizeof(sql_query),
             "INSERT INTO Documente (ID_Utilizator, Continut) "
             "VALUES (%d, '%s');",
             idUtilizator, continut);

    if (sqlite3_exec(db, sql_query, 0, 0, 0) != SQLITE_OK) {
        fprintf(stderr, "Eroare la adăugarea documentului în baza de date.\n");
        return -1;
    }

    return 0;
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
    //sqlite3 *db;
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
        "Status INTEGER NOT NULL);";

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
    if(rc == 0)

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

static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[Thread %d] Asteptam commanda...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());

    raspunde((struct thData *)arg);

    /* am terminat cu acest client, inchidem conexiunea */
    close(((intptr_t)arg));
    return (NULL);
}

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

    // verificarea rezultatului si obtinerea hash-ului stocat
    if (query_rs == SQLITE_ROW) {
        const unsigned char *dbHash = sqlite3_column_text(stmt, 0);

        // Compararea sirurilor de octeti
        if(memcmp(dbHash, storedHashedPasswordHex, strlen(storedHashedPasswordHex)) == 0) {
            // Parolele coincid
            ////
            printf("Parola stocata: %s\n", dbHash);
            printf("Parola introdusa: %s\n", storedHashedPasswordHex);
            /////
            return 1;
        } else {
            // Parolele nu coincid
            return 0;
        }
    } else if (query_rs != SQLITE_DONE) {
        fprintf(stderr, "Error at executing the query.\n");
        return -1;
    } else {
        // Utilizatorul nu a fost gasit
        return 0;
    }
    sqlite3_finalize(stmt); //finalizare interogare
    // inchidem baza de date
    sqlite3_close(db);

}

int verifyUserStatus(const char *userName) {

    sqlite3_stmt *stmt;

    // construirea interogarii
    char sql_query[100];

    if (userName != NULL) {
        snprintf(sql_query, sizeof(sql_query), "SELECT Status FROM Utilizatori WHERE Nume = '%s';", userName);
    } else {
        fprintf(stderr, "[server]Erorr: Please provide a username.\n");
        return -1;
    }


    // deschidere baza de date
    sqlite3 *db;
    if (sqlite3_open("server_data.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Nu s-a putut deschide baza de date!\n");
        return -1;
    }

    // pregateste si executa interogarea
    if (sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "[server]Eroare la pregătirea interogării.\n");
        return -1;
    }

    int status = -1; // val de inceput :  utilizatorul nu a fost gasit

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // obtine valoarea coloanei status din tabel
        status = sqlite3_column_int(stmt, 0);
    }

    // finalizarea interogarii
    sqlite3_finalize(stmt);
    // inchidem baza de date
    sqlite3_close(db);

    return status;
}

int Signup(char comm[]) {

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

    // VERIFICARE DACA EXISTA DEJA UTILIZATOR CU ACELASI USERNAME - n-ar trebui sa se poata adauga
    sqlite3 *db;
    if (sqlite3_open("server_data.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Nu s-a putut deschide baza de date!\n");
        return -1;
    }
    int usernameExists = verifyUsername(username, db);
    sqlite3_close(db);
    if(usernameExists == 1) return 0;

    // HASHING PAROLA
    printf("Parola este: %s\n", parola);
    unsigned char hashedPassword[32] = "";
    sha256((const unsigned char *)parola, strlen(parola), hashedPassword);

    unsigned char parolaHashed[64] = "";
    snprintf(parolaHashed, sizeof(parolaHashed), "%s", hashedPassword);
    bytesToHex(hashedPassword, sizeof(hashedPassword), parolaHashed);
    printf("Hashed Password: %s\n", parolaHashed);

    // acum avem username[30] si parola hashedPassword[32] - trebuie doar sa le introducem in tabel impreuna

    // deschidere tabel
    if(sqlite3_open("server_data.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Couldn't open the database!\n");
        return -1;
    }

    // inserarea
    const char *insertUserQuery = "INSERT INTO Utilizatori (Nume, Parola, Status) VALUES (?, ?, ?);";

    sqlite3_stmt *stmt; // pentru instructiunea preparata

    // pregatirea instructiunii sql
    int rc = sqlite3_prepare_v2(db, insertUserQuery, -1, &stmt, 0);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "Error at preparing the SQL instruction! %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
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
        return 1;
    } else if(rc == SQLITE_CONSTRAINT) {
        printf("Utilizatorul există deja!\n");
        // return 0 - daca exista - deci nu e eroare la signup, doar exista utilizatorul
        return 0;
    } else {
        fprintf(stderr, "Eroare la execuția instrucțiunii SQL! %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // finalizarea instructiunii
    sqlite3_finalize(stmt);

    // inchidem baza de date
    sqlite3_close(db);
}


int Login(char comm[]) {

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
    sqlite3 *db;
    if(sqlite3_open("server_data.db", &db) != SQLITE_OK) {
        fprintf(stderr, "Nu s-a putut deschide baza de date!\n");
        return -1;
    }

    // VERIFICAREA USERNAME-ULUI SI A PAROLEI + HASH !!
    int usernameExists = verifyUsername(username, db);
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
    int pwdMatches = verifyPassword(username, parola, parolaHashed, db);
    if(pwdMatches == 1) {
        valid_password = 1;
        printf("Parola este corecta.\n");
    } else {
        printf("Parola este incorecta.\n");
        return -1;
    }

    // VERIFICARE + SETARE STATUS - daca e deja 1 - EROARE, daca e 0 il setam la 1
    int current_status = verifyUserStatus(username);
    if(current_status == 1) {
        return 0;
    }
    sqlite3_stmt *stmt;
    const char *sql_query = "UPDATE Utilizatori SET Status = 1 WHERE Nume = ?;";

    // pregateste interogarea
    if (sqlite3_prepare_v2(db, sql_query, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Error at preparing the query.\n");
        return -1;
    }

    // parametrul pt query
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    int query_rs = sqlite3_step(stmt);
    sqlite3_finalize(stmt); //finalizarea interogarii

    if (query_rs == SQLITE_DONE) {
        // actualizarea s-a realizat cu succes
        printf("Status for user %s was updated to 1 (logged in).\n", username);
        return 1;
    } else {
        // eroare la executarea interogarii
        fprintf(stderr, "Error at executing the query.\n");
        return -1;
    }

    // inchidere baza de date
    sqlite3_close(db);
}

bool VerificareDenumireInvalida(const char* Nume) {
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

void procesareComenzi(const char *cmd) {

    /*    */
    /* TODO : PROCESARE PROVIZORIE PENTRU TOATE COMENZILE */
    /*    */

    char trimmedCommand[100];
    strcpy(trimmedCommand, cmd);
    strtrim(trimmedCommand);


    if(strncmp(trimmedCommand, "login", 5) == 0) {
        int logged_in = Login(trimmedCommand);
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

        /// TODO: Rezolvare problema cu logout !! cu ajutorul chestiei de la create_document cu idThread si idUtilizator si legatura dintre astea 2

        // preluare username din instructiune
        char username[30];
        const char *delim = " ";
        char *token;
        bool ok = 1;

        token = strstr(trimmedCommand, delim); //prima aparitie a spatiului

        if(token != NULL) {

        // avanseaza peste spatii
        token += strlen(delim);
        // gaseste urmatoarea aparitie a spatiului
        char *usernameEnd = strstr(token, delim);

        if(usernameEnd != NULL) {
            // copiere username in sir
            strncpy(username, token, usernameEnd - token);
            username[usernameEnd - token] = '\0';
        } else {
            ok = 0;
            }
        } else ok = 0;

        if(ok == 0) {
            strcpy(mesaj, "");
            strcpy(mesaj, "Logout unsuccesful! Try again!");
            mesaj[strlen(mesaj)] = '\0';
        } else {
            // verificare status + actualizare
            int user_status = verifyUserStatus(username);
            if(user_status == 0) {
                strcpy(mesaj, "");
                strcpy(mesaj, "You're logged out!");
                mesaj[strlen(mesaj)] = '\0';
            } else {
                strcpy(mesaj, "");
                strcpy(mesaj, "Logout unsuccesful! Try again!");
                mesaj[strlen(mesaj)] = '\0';
            }
        }
    }
    else if(strncmp(trimmedCommand, "signup", 6) == 0) {
        int signup_status = Signup(trimmedCommand);
        if(signup_status == -1) {
            strcpy(mesaj, "");
            strcpy(mesaj, "Signup failed!");
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
    else if(strcmp(trimmedCommand, "close session") == 0) {
        strcpy(mesaj, "");
        strcpy(mesaj, "stop");
        mesaj[strlen(mesaj)] = '\0';
    }
    else if(strncmp(trimmedCommand, "modify_docdata", 14) == 0) {
        const char* xmlFilePath = "documente.xml";
        XMLFileParser(xmlFilePath); //testare
    }
    else if(strncmp(trimmedCommand, "create_document", 15) == 0) {
        // preluare idUtilizator
        char nume[30] = ""; char idUtilizator[30] = "";
        strcpy(nume, trimmedCommand+16);

        printf("Document name: %s", nume);

        // TODO: preluare idUtilizator si verificare daca e logat cineva de pe thread-ul curent - daca da, id-ul acelui user va fi preluat ca idUtilizator

        // TODO: verificare validitate denumire fisier

        if(VerificareDenumireInvalida(nume) == -1) {
            strcpy(mesaj, "");
            strcpy(mesaj, "Invalid name! Please try again!");
            mesaj[strlen(mesaj)] = '\0';
        } else if(VerificareDenumireInvalida(nume) == 0) {
            strcpy(mesaj, "");
            strcpy(mesaj, "Error: document name already used! Please try again!");
            mesaj[strlen(mesaj)] = '\0';
        } else {
            //addDocument(db, nume, "");
            strcpy(mesaj, "");
            strcpy(mesaj, "New document created!");
            mesaj[strlen(mesaj)] = '\0';
        }
    }
    else if(strncmp(cmd, "edit", 4) == 0) {
        /// Editing = 1;

    }
    else if(strncmp(cmd, "download_document", 17) == 0)
    {
        //Download();
    }
    else if(strncmp(cmd, "save_document", 13) == 0)
    {
        //updateDocument();
    }
    else if(strncmp(cmd, "find_document", 13) == 0)
    {
        //findDocument();
    }
    else if(strcmp(cmd, "delete_document") == 0) {
        //stergereDocument();
    }
}


void raspunde(void *arg)
{
    char command[100];
    struct thData tdL;
    tdL = *((struct thData *)arg);

    /* citim comenzile de la client */
    while(1) {

        if (read(tdL.cl, command, sizeof(command)) <= 0)
        {
            printf("[Thread %d]\n", tdL.idThread);
            perror("Eroare la read() de la client.\n");
            break;
        }

        // Procesare comanda
        printf("[Thread %d] Comanda a fost receptionata...%s\n", tdL.idThread, command);

        /* pregatim mesajul de raspuns */

        //char mesaj[100];
        ////
        snprintf(mesaj, sizeof(mesaj), "Am primit comanda: %s", command);
        //////

        printf("[Thread %d] Se proceseaza comanda...%s\n", tdL.idThread, mesaj);

        procesareComenzi(command);
        // printf("Mesaj dupa procesare: %s\n", mesaj);

        /* returnam mesajul clientului */
        if (write(tdL.cl, mesaj, sizeof(mesaj)) <= 0)
        {
            printf("[Thread %d] ", tdL.idThread);
            perror("[Thread]Eroare la write() catre client.\n");
        }
        else {
            printf("[Thread %d] Mesajul a fost transmis cu succes.\n", tdL.idThread);
            ////
            printf("Mesajul trimis: %s\n", mesaj);
            //////
        }
    }

    // Inchide conexiunea dupa ce bucla se termina
    close(tdL.cl);
}


