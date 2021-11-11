extern "C" {
// #include "/mnt/c/cs/network/unpv13e/lib/unp.h"
#include "unp.h"
}
#undef min
#undef max
#include <unordered_map>
#include <unordered_set>
#include <ctype.h>
using namespace std;
// global variables
unordered_map<int, string> clients;
unordered_set<string> names;
fd_set allset;
char* secret;
unordered_map<char, int> secretmap;
int maxfdp1;
// get secret
char * getsecret(const char* filename, int max_len){
    // open file
    FILE * file = fopen(filename, "r");
    // get dicsize
    int dicsize = 0;
    char* word = (char*)malloc(1026);
    long unsigned int len = 0;
    while ((getline(&word, &len, file)) != -1) {
        dicsize++;
    }
    rewind(file);
    printf("disize %i\n", dicsize);
    // get the word
    while(1){
        int loc = rand() % dicsize;
        printf("loc %i\n",loc);
        int count = 0;
        while (count<=loc) {
            getline(&word, &len, file);
            count++;
        }
        if (strlen(word)<max_len+2)
        {
            break;
        }
        
    }
    word[strlen(word)-1] = '\0';
    // count char number
    string s;
    s.append(word);
    for (char c:s)
    {
        secretmap[tolower(c)]++;
    }

    return word;
}

// establish connect
void connectclient(int listenfd){
    // too many connections
    if (clients.size()>=5)
        return;
    // connect
    struct sockaddr_in remote_client;
    int addrlen = sizeof( remote_client );
    int newsd = accept(listenfd, (struct sockaddr *)&remote_client, (socklen_t *)&addrlen );
    maxfdp1 = max(maxfdp1, newsd+1);
    // request username
    char const *send = "Welcome to Guess the Word, please enter your username.\n";
    writen(newsd,(void*) send, strlen(send));
    // save client info
    clients[newsd]="";
    FD_SET(newsd,&allset);
    printf("Connect to %i, right now has %lu clients\n", newsd, clients.size());
    return;
}

// delete the client info
void disconnectclient(int sockfd){
    // has username
    if (clients[sockfd]!=""){
        names.erase(clients[sockfd]);
    }
    // clear all data
    clients.erase(sockfd);
    FD_CLR(sockfd,&allset);
}

// check whether guess word correct
bool checkword(int sockfd, char* guess){
    if (strlen(guess)!=strlen(secret))
    {
        string line = "Invalid guess length. The secret word is ";
        line = line + to_string(strlen(secret)) + " letter(s).\n";
        const char * send = line.c_str();
        writen(sockfd,(void*) send, strlen(send));
        return false;
    }
    
    unordered_map<char, int> guessmap;
    int cp = 0;
    int cw = 0;
    // start check
    for (int i = 0; i < strlen(guess); i++)
    {
        // correct location
        char c = tolower(guess[i]);
        if (c==tolower(secret[i]))
        {
            cp++;
        }
        guessmap[c]++;
    }
    // check correct words
    for (auto itr = secretmap.begin(); itr != secretmap.end(); itr++)
    {
        char c = itr->first;
        cw += min(guessmap[c],secretmap[c]);
    }
    // send messages
    string line = clients[sockfd];
    // correct word
    if (cp==strlen(guess))
    {
        line.append(" has correctly guessed the word ");
        line.append(secret);
        line.append("\n");
    }
    // info
    else{
        line.append(" guessed ");
        line.append(guess);
        line.append(": ");
        line.append(to_string(cw));
        line.append(" letter(s) were correct and ");
        line.append(to_string(cp));
        line.append(" letter(s) were correctly placed.\n");
    }
    // send to all clients
    for(auto itr = clients.begin(); itr != clients.end(); itr++){
        const char * send = line.c_str();
        writen(itr->first,(void*) send, strlen(send));
        // correct word, disconnect from client
        if(cp == strlen(guess)){
            close(itr->first);
        }
    }
    if(cp == strlen(guess))
        return true;
    else
        return false;
}

// process client request
bool processclient(int sockfd){
    char buf[1024];
    int nbytes;
    // client disconnected
    if((nbytes=read(sockfd, buf, 1024))==0)
    {
        disconnectclient(sockfd);
        return false;
    }
    buf[nbytes-1] = '\0';
    printf("Received %s from %i\n",buf,sockfd);
    string cname = clients[sockfd];
    bool finish = false;
    // haven't have username 
    if (cname=="")
    {
        string name;
        name.append(buf);
        // name not used
        if (names.find(name)==names.end())
        {
            // save name
            names.insert(name);
            clients[sockfd]=buf;
            // send message
            string line = "Let's start playing, ";
            line.append(buf);
            line.append("\n");
            const char * send = line.c_str();
            writen(sockfd,(void*) send, strlen(send));
            printf("New name %s assigned to %i\n",buf,sockfd);
            // send current status
            line = "There are ";
            line.append(to_string(clients.size()));
            line.append(" player(s) playing. The secret word is ");
            line.append(to_string(strlen(secret)));
            line.append(" letter(s).\n");
            send = line.c_str();
            writen(sockfd,(void*) send, strlen(send));
            printf("%lu players are guessing %zu letters\n",clients.size(),strlen(secret));
        }
        // name used
        else{
            // send message
            string line = "Username ";
            line.append(buf);
            line.append(" is already taken, please enter a different username\n");
            const char * send = line.c_str();
            writen(sockfd,(void*) send, strlen(send));
            printf("Name used for %i, with %zu names\n", sockfd, names.size());
        }
    }
    // word guess
    else{
        finish = checkword(sockfd, buf);
    }
    return finish;
}
int main(int argc, char const *argv[])
{
    // read arguments
    int seed = atoi(argv[1]);
    int port = atoi(argv[2]);
    const char* filename = argv[3];
    int max_len = min(1024, atoi(argv[4]));
    srand(seed);
    // socket
    struct sockaddr_in	servaddr;
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    // bind
    bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(port);
	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
    // listen
    listen(listenfd, 5);
    // start
    fd_set rset;
    bool finish = true;
    while(1){
        // initialize
        if (finish)
        {
            maxfdp1 = listenfd+1;
            FD_ZERO(&allset);
            FD_SET(listenfd,&allset);
            // get secret work
            secretmap.clear();
            secret = getsecret(filename, max_len);
            printf("Start: Secret: %s\n", secret);
            // clear
            clients.clear();
            names.clear();
            finish = false;
        }
        
        rset = allset;
        select(maxfdp1, &rset, NULL, NULL, NULL);
        // new client wants to connect
        if(FD_ISSET(listenfd, &rset)){
            connectclient(listenfd);
        }
        // check every client
        for (auto itr = clients.begin(); itr != clients.end(); )
        {
            auto pitr = itr;
            itr++;
            // receive from client
            if (FD_ISSET(pitr->first,&rset))
            {
                finish = processclient(pitr->first);
            }
            
            
        }
        
    }
    return 0;
}
