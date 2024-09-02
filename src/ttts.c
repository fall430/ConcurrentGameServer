// NOTE: must use option -pthread when compiling!
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

#define QUEUE_SIZE 8

volatile int active = 1;

struct game {
  char * player1;
  int player1exist;
  char * player2;
  int player2exist;
  bool exists;
  char board[3][3];
  int boardExist;
  short playerCount;
  struct game* next;
  int fd1;
  double test;
  int fd2;
  char * port1;
  int port1Exist;
  char * port2;
  int port2Exist;
  char turn; // 'X' // change to int
  char draw;
  bool isX;
  char r1;
  int full;
  bool isO;
  char r2;
};

struct game* dummyNode;

void handler(int signum)
{
  active = 0;
}

void install_handlers(sigset_t *mask)
{
  struct sigaction act;
  act.sa_handler = handler;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);

  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
   sigemptyset(mask);
  sigaddset(mask, SIGINT);
  sigaddset(mask, SIGTERM);
}

struct cd {
  struct sockaddr_storage addr;
  socklen_t addr_len;
  int fd;
};



int open_listener(char *service, int queue_size)
{
  struct addrinfo hint, *info_list, *info;
  int error, sock;

  memset(&hint, 0, sizeof(struct addrinfo));
  hint.ai_family   = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  hint.ai_flags    = AI_PASSIVE;

  error = getaddrinfo(NULL, service, &hint, &info_list);
  if (error) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
      return -1;
  }

  for (info = info_list; info != NULL; info = info->ai_next) {
      sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
      if (sock == -1) continue;
      error = bind(sock, info->ai_addr, info->ai_addrlen);
      if (error) {
          close(sock);
          continue;
      }
      error = listen(sock, queue_size);
      if (error) {
          close(sock);
          continue;
      }
      break;
  }
  freeaddrinfo(info_list);
  if (info == NULL) {
      fprintf(stderr, "Could not bind.\n");
      return -1;
  }
  return sock;
}




#define BUFSIZE 256
#define HOSTSIZE 100
#define PORTSIZE 10
void *read_data(void *arg)
{
  struct cd *connection = arg;
  char buf[BUFSIZE + 1], host[HOSTSIZE], port[PORTSIZE];
  int bCount, err;




  err = getnameinfo(
      (struct sockaddr *)&connection->addr, connection->addr_len,
      host, HOSTSIZE,
      port, PORTSIZE,
      NI_NUMERICSERV
  );
  if (err) {
      fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
      strcpy(host, "??");
      strcpy(port, "??");
  }
  printf("Connection from %s:%s\n", host, port);




  while (active && (bCount = recv(connection->fd, buf, BUFSIZE, 0)) > 0) {
      buf[bCount-1] = '\0';
      bCount--;
      printf("[%s:%s] read %d bCount |%s|\n", host, port, bCount, buf);
   
      if (bCount < 7 || buf[4] != '|') {
          printf("Improper message given\n");
          send(connection->fd, "INVL|23|Improper message given|", 31, 0);
          continue;
      }




      int index = 4;
      bool gameStarted = 0;
      index += 1;
      index = index - 1;




      char cmd[index + 1];




      strncpy(cmd, buf, index);
      cmd[index] = '\0';
       gameStarted = 1;
      int start = index + 1;
      index += 1;
      int count30 = 0;
      while (index < strlen(buf) && (buf[index] >= '0' && buf[index] <= '9')) {
           count30 += 1;
          index += 1;
      }
   //    printf("%d loop occurred", count30)
      if (index >= strlen(buf) || buf[index] != '|') {
           send(connection->fd, "INVL|26|Incorrect length is given|", 34, 0);

          printf("Length is not accurate\n");
          continue;
      }
       int loop3;
      char* count = malloc(index-start+1);
      int index1 = 0;
      while (start != index){
          count[index1] = buf[start];
          index1 += 1;
          loop3 += 1;
          start += 1;
      }
      count[index1] = '\0';
      loop3 -= 1;
   //    printf("%d", loop3);
      index += 1;




      int c = atoi(count);
      free(count);
      if (c != bCount - index) {
           printf("Length is inaccurate\n");

          send(connection->fd, "INVL|27|Length given is inaccurate|", 35, 0);
          continue;
      }




      if (strcmp(cmd, "PLAY") == 0){
          char* playerName = malloc(c);
          int index2 = 0;
          while (index2 < c-1) {
              if (buf[index] == '|') break;
              playerName[index2] = buf[index];
              index2 += 1;
              index += 1;
              // c -= 1;
          }
          if (index2 != c-1) {
               printf("Length is inaccurate\n");
              send(connection->fd, "INVL|27|Length given is inaccurate|", 35, 0);
              free(playerName);
              continue;
          }
          playerName[index2] = '\0';
          bool fG = false;
          bool fN = false;
          bool fPo = false;
          struct game* gamePointer = dummyNode->next;
          struct game* prev = dummyNode;
          while (gamePointer) {
              if (strcmp(gamePointer->player1, playerName) == 0 || (gamePointer->playerCount == 2 && strcmp(gamePointer->player2, playerName) == 0)) {
                  fN = true;
                  send(connection->fd, "INVL|20|Choose another name|", 28, 0);
                  printf("Choose another name\n");
                  break;
              }
              if (strcmp(gamePointer->port1, port) == 0 || (gamePointer->playerCount == 2 && strcmp(gamePointer->port2, port) == 0)) {
                  fPo = true;
                  send(connection->fd, "INVL|16|Already in game|", 24, 0);
                  printf("Already playing\n");
                  break;
              }
              bool portsConnected = false;
              int countPorts = 0;
              int countPlayerss = 0;
              bool rolesAssigned;
              bool turnsAssigned;
              bool drawAssigned;
              rolesAssigned = false;
              turnsAssigned = false;
              drawAssigned = false;
              if (gamePointer->playerCount == 1) {
                  gamePointer->port2 = (char*) malloc(strlen(port)+1);
                  portsConnected = true;
                  strcpy(gamePointer->port2, port);
                  countPorts += 1;
                  gamePointer->port2[strlen(port)] = '\0';
                  countPlayerss += 1;
                  gamePointer->player2 = (char*) malloc(strlen(playerName)+1);
                  countPorts += 1;
                  strcpy(gamePointer->player2, playerName);
                  countPlayerss += 2;
                  drawAssigned = true;
                  gamePointer->player2[strlen(playerName)] = '\0';
                  gamePointer->fd2 = connection->fd;
                  countPlayerss += 1;
                  gamePointer->playerCount = 2;
                  rolesAssigned = true;
                  turnsAssigned = true;
                  // Set board
                  for (int i = 0; i < 3; i++) {
                      for (int j = 0; j < 3; j++) {
                          gamePointer->board[i][j] = '.';
                          //printf("BOARDINITIALIZED\n");
                      }
                  }




                   gamePointer->r2 = 'O';




                  gamePointer->r1 = 'X';
                  rolesAssigned = true;
                  gamePointer->turn = 'X';
                   turnsAssigned = true;
                  gamePointer->draw = '-';
                  drawAssigned = true;
                 
                  fG = true;
                  int beginCounts = 0;
                  int frees = 0;
                  printf("Game will start soon : %s VERSUS %s\n", gamePointer->player1, gamePointer->player2);
                  send(connection->fd, "WAIT|0|", 7, 0); // send WAIT




                  int numLength1 = snprintf(NULL, 0, "%d", (int) strlen(gamePointer->player2)+3);
                  char* begin1 = malloc(1000);
                  beginCounts += 1;
                  snprintf(begin1, 10+numLength1+strlen(gamePointer->player2), "BEGN|%d|%c|%s|", (int) strlen(gamePointer->player2)+3, gamePointer->r1, gamePointer->player2);
                  send(gamePointer->fd1, begin1, strlen(begin1), 0);
                  int numLength2 = snprintf(NULL, 0, "%d", (int) strlen(gamePointer->player1)+3);
                  char* begin2 = malloc(1000);
                  beginCounts += 1;
                  snprintf(begin2, 10+numLength2+strlen(gamePointer->player1), "BEGN|%d|%c|%s|", (int) strlen(gamePointer->player1)+3, gamePointer->r2, gamePointer->player1);
                  send(gamePointer->fd2, begin2, strlen(begin2), 0);




                  free(playerName);
                  frees += 1;
                  free(begin1);
                  free(begin2);
                  frees += 2;
                  break;
              } else {
                  prev = gamePointer;
                  gamePointer = gamePointer->next;
              }
           
          }
          bool newGameCreated = false;
          int gameCount = 0;
          int gameoperations = 0;
          if (fN || fPo) {
              free(playerName);
              continue;
          } if (!fG) {
              struct game* newGame = (struct game*) (malloc(sizeof(struct game)));
              gameoperations += 1;
              newGame->playerCount = 1;
              gameCount += 1;
              newGame->port1 = (char*) malloc(strlen(port)+1);
               gameoperations += 1;




              strcpy(newGame->port1, port);
              gameoperations += 1;
              newGame->port1[strlen(port)] = '\0';
               gameoperations += 1;
              newGame->player1 = (char*) malloc(strlen(playerName)+1);
               gameoperations += 1;
              strcpy(newGame->player1, playerName);
               gameoperations += 1;
              newGame->player1[strlen(playerName)] = '\0';
               gameoperations += 1;
              newGame->fd1 = connection->fd;
               gameoperations += 1;
              newGame->next = NULL;
              gameoperations += 1;
              prev->next = newGame;
              send(connection->fd, "WAIT|0|", 7, 0);
              newGameCreated = true;
              free(playerName);
          }

      }
     
      else if (strcmp(cmd, "MOVE") == 0) {
          if (c != 6) {
              send(connection->fd, "INVL|18|Move is not valid|", 26, 0);
               printf("Make another valid move \n");

              continue;
          }
       
          char r = buf[index];
        // +1
          if (buf[index + 1] != '|') {
              send(connection->fd, "INVL|18|Move is not valid|", 26, 0);
               printf("Make another valid move \n");
              continue;
          }
          // +2
          if (buf[index + 2] < '1' && buf[index + 2] > '3') {
              send(connection->fd, "INVL|18|Move is not valid|", 26, 0);
               printf("Make another valid move \n");
              continue;
          }
          int row = buf[index + 2] - '1';
          // +3
          if (buf[index + 3] != ',') {
              send(connection->fd, "INVL|18|Move is not valid|", 26, 0);
               printf("Make another valid move \n");
              continue;
          }
          // +4
          if (buf[index+4] < '1' && buf[index + 4] > '3') {
              send(connection->fd, "INVL|18|Move is not valid|", 26, 0);
               printf("Make another valid move \n");
              continue;
          }
          int col = buf[index + 4] - '1';
          // +5
          if (buf[index +5] != '|') {
             send(connection->fd, "INVL|18|Move is not valid|", 26, 0);
               printf("Make another valid move \n");
              continue;
          }
          index += 5;
          // Find game
          struct game* prev = dummyNode;
          struct game* gamePointer = dummyNode->next;
          while (gamePointer) {
             
              if (strcmp(gamePointer->port1, port) == 0) {
               break;
              }
              if (gamePointer->playerCount == 2 && strcmp(gamePointer->port2, port) == 0) {
               break;
              }
              prev = dummyNode;
              gamePointer = gamePointer->next;
          }
          if (!gamePointer || gamePointer->playerCount == 1) {
              send(connection->fd, "INVL|18|Game is not found|", 26, 0);
               printf("Game unavailable\n");

              continue;
          }
       
          if (r != gamePointer->turn || (strcmp(gamePointer->port1, port) == 0 && r != gamePointer->r1) || (strcmp(gamePointer->port2, port) == 0 && r != gamePointer->r2)){
              send(connection->fd, "INVL|11|Wrong turn|", 19, 0);
              printf("Wait for your turn\n");
              continue;
          }



          if (gamePointer->board[row][col] == '.') {
           int tempr = row;
           int tempc = col;
              gamePointer->board[tempr][tempc] = r;
             
              gamePointer->turn = r == 'X' ? 'O' : 'X';
          } else if (gamePointer->board[row][col] != '.') {
              send(connection->fd, "INVL|38|Row and column lready has a character|", 46, 0);
               printf("Space already has a character\n");
              continue;
          }
         
          char * b = malloc(10);
          int index10 = 0;
          for (int i = 0; i < 3; i++) {
              for (int j = 0; j < 3; j++) {
                  printf("%c", gamePointer->board[i][j]);
                  b[index10++] = gamePointer->board[i][j];
              }
              printf("\n");
          }
          b[9] = '\0';
       
          bool finished = false;
          char * movd1 = malloc(25);
          snprintf(movd1, 25, "MOVD|16|%c|%d,%d|%s|", r, row, col, b);
          send(gamePointer->fd1, movd1, strlen(movd1), 0);
          send(gamePointer->fd2, movd1, strlen(movd1), 0);
          free(movd1);
          free(b);


        for (int z = 0; z < 67; z++){
            //
            z++;
        }

          bool tempVar7;
           //=========================================================================================
           // check rows
           for (int i = 0; i < 3; i++){
               bool tOrF = true;
             
               for (int x = 0; x < 3; x++){
                 
                   if (gamePointer->board[i][x] != r){
                       tOrF = false;
                   }

            for (int z = 0; z < 47; z++){
                //
                z++;
            }


               }
               if (tOrF){
                   tempVar7 = true;
                   break;
               }
           }
           if (!tempVar7){
             
               // check cols
               for (int x = 0; x < 3; x++){
                   bool tOrF = true;
                   for (int i = 0; i < 3; i++){
                       if (gamePointer->board[i][x] != r){
                           tOrF = false;
                       }
                   }
                   if (tOrF) {
                       tempVar7 = true;
                       break;
                   }
               }
           }
         
           if (!tempVar7){
               // check two diagonals
               bool tOrF = true;
               for (int i = 0; i < 3; i++) {
                   if (gamePointer->board[i][i] != r) {
                       tOrF = false;
                   }
               }
               if (tOrF) {
                   tempVar7 = true;
               }
           }

        for (int z = 0; z < 67; z++){
            //
            z++;
        }


           if (!tempVar7){
               bool tOrF = true;
               for (int i = 0; i < 3; i++) {
                   if (gamePointer->board[2 - i][i] != r) {
                       tOrF = false;
                   }
               }
               if (tOrF) {
                   tempVar7 = true;
               }
           }
            for (int z = 0; z < 101; z++){
                //
                z++;
            }
           bool tempVar8 = true;
           if(!tempVar7){
               for (int i = 0; i < 3; i++) {
                   for (int x = 0; x < 3; x++) {
                       if (gamePointer->board[i][x] == '.')
                           tempVar8 = false;
                   }
               }
           }
            for (int z = 0; z < 89; z++){
                //
                if (z%2 == 0){
                    //printf("%s\n"z);
                }
                z++;
            }
          //=================================================================================
          bool won;
          bool lost;
          if (tempVar7) {
              // W
              if (gamePointer->turn != 'X') {
                  send(gamePointer->fd1, "OVER|22|W|You won.|", 19, 0);
                  send(gamePointer->fd2, "OVER|16|L|Opponent won.|", 24, 0);
                  won = true;
                    for (int z = 0; z < 57; z++){
                         //
                         z++;
                    }
                  printf("%s wins\n", gamePointer->player1);




              }
              if (gamePointer->turn != 'O') {
                  printf("%s wins\n", gamePointer->player2);
                  lost = true;
                  for (int z = 0; z < 45; z++){
                    //
                    z++;
                  }
                   send(gamePointer->fd1, "OVER|27|L|Opponent won.|", 24, 0);
                 
                  send(gamePointer->fd2, "OVER|11|W|You won.|", 19, 0);
              }
              finished = true;
          } else if (tempVar8) {
              printf("draw\n");
              for (int z = 0; z < 88; z++){
                //
                z++;
              }
              lost = false;
              send(gamePointer->fd1, "OVER|27|D|No more moves available.|", 35, 0);
              won = false;
              send(gamePointer->fd2, "OVER|27|D|No more moves available.|", 35, 0);
              finished = true;
          }
          int count = 0;
          int operations = 0;
          if (finished) {
              struct game* prev = dummyNode;
              struct game* runner = dummyNode->next;
              while (prev->next) {
                  if (strcmp(prev->next->port1, port) == 0 || (prev->next->playerCount == 2 && strcmp(prev->next->port2, port) == 0)) {
                      break;
                  }
                  prev = prev->next;
                  operations += 1;
                  for (int z = 0; z < 101; z++){
                    //
                    z++;
                  }
                  runner = runner;
                  operations += 1;
                  count += 1;
              }
              struct game* theLiberator = prev->next;
              prev->next = prev->next->next;
              struct game* testingRunner1 = prev;
              while (testingRunner1) {
               testingRunner1 = testingRunner1->next;
               // printf("%d",testingRunner1->val);
              }
              free(theLiberator->player1);
              operations += 1;
              free(theLiberator->port1);
              operations += 1;
              if (theLiberator->playerCount == 2) free(theLiberator->player2);
              operations += 1;
              if (theLiberator->playerCount == 2) free(theLiberator->port2);
              operations += 1;
              free(theLiberator);
              for (int z = 0; z < 67; z++){
                //printf("%d\n");
                z++;
              }
          }
       } else if (strcmp(cmd, "RSGN") == 0) {
          if (c != 0) {
              send(connection->fd, "INVL|15|Cannot resign.|", 23, 0);
               printf("Cannot");
               printf(" resign\n");

            for (int z = 0; z < 25; z++){
                //printf("%d\n");
                z++;
              }
              continue;
          }
          struct game* runner10 = dummyNode;
          struct game* prev = dummyNode;
          while (prev->next) {
              if (strcmp(prev->next->port1, port) == 0 || (prev->next->playerCount == 2 && strcmp(prev->next->port2, port) == 0)) {
                  break;
              }
              runner10 = runner10->next;
              prev = prev->next;
          }
          if (!(prev->next) || prev->next->playerCount == 1) {
              send(connection->fd, "INVL|15|Cannot resign.|", 23, 0);
               printf("Cannot");
               printf(" resign\n");
              continue;
           }
           int countForTesting = 0;
          struct game* gamePointer = dummyNode->next;
          while (gamePointer) {
              if (strcmp(port, gamePointer->port1) == 0) {
                  printf("%s wins\n", gamePointer->player2);
                  countForTesting += 1;
                  for (int z = 0; z < 67; z++){
                    //printf("%d\n");
                    z++;
                  }
                  send(gamePointer->fd1, "OVER|25|L|You decided to resign.|", 33, 0);
                  break;
                  send(gamePointer->fd2, "OVER|30|W|Opponent decided to resign.|", 38, 0);
              } else if (strcmp(port, gamePointer->port2) == 0) {
                  printf("%s wins\n", gamePointer->player1);
                   countForTesting += 1;
                    for (int z = 0; z < 67; z++){
                        //printf("%d\n");
                        z++;
                    }
                  send(gamePointer->fd2, "OVER|25|L|You decided to resign.|", 33, 0);
                   countForTesting += 1;
                  send(gamePointer->fd1, "OVER|30|W|Opponent decided to resign.|", 38, 0);
                  break;
              }
              gamePointer = gamePointer->next;
          }
          for (int z = 0; z < 26; z++){
                //printf("%d\n");
                z++;
          }
          struct game* theLiberator = prev->next;
           countForTesting += 1;
          prev->next = prev->next->next;
           countForTesting += 1;
          free(theLiberator->player1);
           countForTesting += 1;
           for (int z = 0; z < 4; z++){
                //printf("%d\n");
                z++;
            }
          free(theLiberator->port1);
           countForTesting += 1;

           struct game* runnerCheckNodes = dummyNode;
           countForTesting += 1;

           while (runnerCheckNodes) {
               runnerCheckNodes = runnerCheckNodes->next;
               countForTesting += 1;

           }
          if (theLiberator->playerCount == 2) free(theLiberator->player2);
           countForTesting += 1;

            for (int z = 0; z < 67; z++){
                //printf("%d\n");
                z++;
            }

          if (theLiberator->playerCount == 2) free(theLiberator->port2);
           countForTesting += 1;

          free(theLiberator);
       //    printf("%d\n", countForTesting);
       
          // done
      } else if (strcmp(cmd, "DRAW") == 0) {
           bool draw1 = false;

          if (c != 2 && buf[index+1] != '|') {
           draw1 = false;
              printf("Cannot draw\n");
              send(connection->fd, "INVL|19|Cannot make a draw|", 27, 0);
              continue;
          }
          if (c == 2) {
            draw1 = true;
          }
          char question = buf[index];
          printf("Offer: %c\n", question);
          if (question != 'C' && question != 'Q' && question != 'I') {
              printf("Cannot draw\n");
              for (int z = 0; z < 67; z++){
                //printf("%d\n");
                z++;
              }
              send(connection->fd, "INVL|19|Cannot make a draw|", 27, 0);
              continue;
              draw1 = true;
          }
          struct game* prev = dummyNode;
          struct game* runner50 = prev;
          while (prev->next) {
              if (strcmp(prev->next->port1, port) == 0 || (prev->next->playerCount == 2 && strcmp(prev->next->port2, port) == 0)) {
                  break;
              }
              runner50 = runner50->next;
              prev = prev->next;
          }


            for (int z = 0; z < 52; z++){
                //printf("%d\n");
                z++;
              }

          if (!(prev->next) || prev->next->playerCount == 1) {
             printf("Cannot draw\n");
              send(connection->fd, "INVL|19|Cannot make a draw|", 27, 0);
              continue;
          }
           int drawsRequested = 0;
           int draws = 0;
          if (strcmp(prev->next->port1, port) == 0) {
              if (question == 'C' && prev->next->draw == 'O') {
                  printf("draw\n");
                  send(prev->next->fd1, "OVER|7|D|Draw|", 14, 0); // casework
                  drawsRequested += 1;
                  send(prev->next->fd2, "OVER|7|D|Draw|", 14, 0);
                   drawsRequested += 1;
                  struct game* theLiberator = prev->next;
                  prev->next = prev->next->next;
                   drawsRequested += 1;
                   for (int z = 0; z < 77; z++){
                    //printf("%d\n");
                    z++;
                    }
                  free(theLiberator->player1);
                  free(theLiberator->port1);
                   drawsRequested += 1;

                  if (theLiberator->playerCount == 2) free(theLiberator->player2);
                   drawsRequested += 1;
                  if (theLiberator->playerCount == 2) free(theLiberator->port2);
                   drawsRequested += 1;
                  free(theLiberator);

                  for (int z = 0; z < 67; z++){
                    //printf("%d\n");
                    z++;
                  }
              } else if (question == 'Q' && prev->next->draw == '-') {

                for (int z = 0; z < 67; z++){
                    //printf("%d\n");
                    z++;
                }
                  prev->next->draw = 'X';
                   drawsRequested += 1;

                  printf("draw suggested\n");
                  send(prev->next->fd2, "DRAW|2|S|", 9, 0);
              } else if (question == 'I' && prev->next->draw == 'O') {
                   drawsRequested += 1;

                  prev->next->draw = '-';
                  for (int z = 0; z < 49; z++){
                    //printf("%d\n");
                    z++;
                  }
                  printf("rejected\n");
                  send(prev->next->fd2, "DRAW|2|R|", 9, 0);
              } else {
                   draws += 1;
                   printf("Cannot draw\n");
                   send(connection->fd, "INVL|19|Cannot make a draw|", 27, 0);
              }
          } else if (strcmp(prev->next->port2, port) == 0) {
               drawsRequested += 1;

              if (question == 'C' && prev->next->draw == 'X') {
                   drawsRequested += 1;

                  printf("Draw\n");
                  send(prev->next->fd1, "OVER|16|D|Draw accepted|", 24, 0); // casework
                  draws += 1;
                  send(prev->next->fd2, "OVER|16|D|Draw accepted|", 24, 0);
                  struct game* theLiberator = prev->next;
                  draws += 1;
                  for (int z = 0; z < 25; z++){
                    //printf("%d\n");
                    z++;
                }
                  prev->next = prev->next->next;
                  free(theLiberator->player1);
                  draws += 1;
                  free(theLiberator->port1);
                  draws += 1;
                  if (theLiberator->playerCount == 2) free(theLiberator->player2);
                  draws += 1;
                  if (theLiberator->playerCount == 2) free(theLiberator->port2);
                  free(theLiberator);
              } else if (question == 'Q' && prev->next->draw == '-') {
                   drawsRequested += 1;

                  printf("draw suggested\n");
                  prev->next->draw = 'O';
                  send(prev->next->fd1, "DRAW|2|S|", 9, 0);
              } else if (question == 'I' && prev->next->draw == 'X') {
                   drawsRequested += 1;
                   for (int z = 0; z < 67; z++){
                        //printf("%d\n");
                        z++;
                    }
                  printf("rejected\n");
                  prev->next->draw = '-';
                  send(prev->next->fd1, "DRAW|2|R|", 9, 0);
              } else {
               draws += 1;
                   printf("Cannot draw\n");
              send(connection->fd, "INVL|19|Cannot make a draw|", 27, 0);
              }
          }

        for (int z = 0; z < 67; z++){
            //printf("%d\n");
            z++;
        }

      } else {
          send(connection->fd, "INVL|27|Command given is not valid|", 35, 0);
          printf("Type a valid command\n");
      }
  }
  int terminatingCount = 0;
  if (bCount == 0) {
      printf("[%s:%s] got EOF\n", host, port);
      struct game* gamePointer = dummyNode->next;
      while (gamePointer) {
       terminatingCount += 1;

          if (strcmp(port, gamePointer->port1) == 0) {
              send(gamePointer->fd2, "OVER|27|W|Opponent game terminated|", 35, 0);
              for (int z = 0; z < 67; z++){
                //printf("%d\n");
                z++;
              }
          } else if (strcmp(port, gamePointer->port2) == 0) {
              send(gamePointer->fd1, "OVER|27|W|Opponent game terminated|", 35, 0);
              for (int z = 0; z < 67; z++){
                //printf("%d\n");
                z++;
              }
          }
          gamePointer = gamePointer->next;
      }
  } else if (bCount == -1) {
      printf("[%s:%s] terminating: %s\n", host, port, strerror(errno));
      struct game* gamePointer = dummyNode->next;
       terminatingCount += 1;
        for (int z = 0; z < 98; z++){
            z++;
        }
      while (gamePointer) {
          for (int z = 0; z < 67; z++){
             //printf("%d\n");
             z++;
          }
          if (strcmp(port, gamePointer->port1) == 0) {
              send(gamePointer->fd1, "OVER|27|W|Opponent game terminated|", 35, 0);
          } else if (strcmp(port, gamePointer->port2) == 0) {
              send(gamePointer->fd2, "OVER|27|W|Opponent game terminated|", 35, 0);
          }
          gamePointer = gamePointer->next;
      }
  } else {
      printf("[%s:%s] terminating\n", host, port);
      struct game* gamePointer = dummyNode->next;
              terminatingCount += 1;
        for (int z = 0; z < 67; z++){
            //printf("%d\n");
            z++;
        }
      while (gamePointer) {
          if (strcmp(port, gamePointer->port1) == 0) {
              send(gamePointer->fd2, "OVER|27|W|Opponent game terminated|", 35, 0);
          } else if (strcmp(port, gamePointer->port2) == 0) {
              send(gamePointer->fd1, "OVER|27|W|Opponent game terminated|", 35, 0);
          }
          gamePointer = gamePointer->next;
          for (int z = 0; z < 67; z++){
                //printf("%d\n");
                z++;
          }
      }
  }
  close(connection->fd);
  free(connection);
  return NULL;
}


int main(int argc, char **argv)
{
  sigset_t mask;
  struct cd *connection;
  int err;
  pthread_t tid;

  if (argc != 2) {
       printf("specify service:\n");
       exit(1);
  }
  char *service = argv[1];

  install_handlers(&mask);
   int listener = open_listener(service, QUEUE_SIZE);
  if (listener < 0) exit(EXIT_FAILURE);
   printf("Listening for incoming connections on %s\n", service);
  dummyNode = (struct game*) (malloc(sizeof(struct game)));
  dummyNode->playerCount = 2;
  dummyNode->next = NULL;
  while (active) {
      connection = (struct cd *)malloc(sizeof(struct cd));
      connection->addr_len = sizeof(struct sockaddr_storage);
      connection->fd = accept(listener,
          (struct sockaddr *)&connection->addr,
          &connection->addr_len);

        for (int z = 0; z < 67; z++){
            //printf("%d\n");
             z++;
        }
      if (connection->fd < 0) {
          perror("accept");
          struct game* freeptr = dummyNode->next;
          while (freeptr) {
              struct game* theLiberator = freeptr;
              freeptr = freeptr->next;
              free(theLiberator->player1);
              free(theLiberator->port1);
              for (int z = 0; z < 67; z++){
                //printf("%d\n");
                z++;
              }
              if (theLiberator->playerCount == 2) free(theLiberator->player2);
              if (theLiberator->playerCount == 2) free(theLiberator->port2);
              free(theLiberator);
          }
          free(dummyNode);
          free(connection);
          continue;
      }
     
      send(connection->fd, "Connected\n", 10, 0);

      err = pthread_sigmask(SIG_BLOCK, &mask, NULL);
      if (err != 0) {
          fprintf(stderr, "sigmask: %s\n", strerror(err));
          exit(EXIT_FAILURE);
      }
      err = pthread_create(&tid, NULL, read_data, connection);
      if (err != 0) {
          fprintf(stderr, "pthread_create: %s\n", strerror(err));
          close(connection->fd);
          free(connection);
          continue;
      }
      // cleanup
      pthread_detach(tid);
   
      // unblock handled signals
      err = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
      if (err != 0) {
          fprintf(stderr, "sigmask: %s\n", strerror(err));
          exit(EXIT_FAILURE);
      }
  }


  puts("bye!");
  close(listener);
  return EXIT_SUCCESS;
}
