#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

//-----------------------------------------------------------------------
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#define max(a, b)  (((a) > (b)) ? (a) : (b))
//#define xor(a, b)  (((a) != (b)) ? 1 : 0 )

char* SplitStr( char *pszStr, char *pszCpy, const int iCpyLen, const char *pszStoper, const int iNum ); // pierwszy element ma inum=0

//-----------------------------------------------------------------------

char* SplitStr( char *pszStr, char *pszCpy, const int iCpyLen, const char *pszStoper, const int iNum )
{
   int   iCount = 0;
   char  *pszStart = pszStr, *pszStop;

   memset( (char*)pszCpy, 0, iCpyLen * sizeof(char) );
   do {
      if( iCount )
         pszStart = pszStop + 1;
      pszStop = strpbrk( pszStart, pszStoper );
      iCount ++;
      }
   while( pszStop && iCount <= iNum );

   if( iCount > iNum )
      {
      if( pszStop )
         strncpy( pszCpy, pszStart, min( iCpyLen, ( int )( pszStop - pszStart )  )  );
      else
         strncpy( pszCpy, pszStart, iCpyLen );  // wcsncpy
      }
   else
      pszCpy = NULL;
   return( pszCpy );
}


//------------------------------------ games -----------------------------------

struct stoneMove
{
    bool whiteSide;     // white;1,2;3,4
    int  FieldFrom[2]; 
    int  FieldTo[2]; 
};

#define BOARD_SIZE 8 // 8*8

//0  - pusty
//1  - biały pion
//2  - czarny pion
//11 - biała dama
//12 - czarna dama

struct gameData  // dane wymieniane z klientem
{
    int  gameBoard[BOARD_SIZE][BOARD_SIZE];
    bool whiteSide;    // czyja kolej - true białe, false czarne    
    char Msg[ 200 ];
};

class game  // jedna gra
{
private:
    char MsgWhite[ 200 ];
    char MsgBlack[ 200 ];
    char Buf[ 2000 ];

    bool MoveOk( bool White, stoneMove & stMove ); // walidacja ruchu
    
public:
    gameData Data;

    int  index;
 
    int  socketWhite;
    int  socketBlack;

    int  moveCounter; // licznik ruchów
    int  moveTime;  // czas ruchu
    int  gameTime;  // czas gry

    bool Move( bool White, char* strMove ); // true = koniec, uaktualnia gameData

    gameData* ClientData( bool White); 
    void  SetMsg( const char* MsgWhite, const char* MsgBlack);
    void  MoveDecode(char* Buf, stoneMove & anyMove ); // "white;1,2;3,4"
    char* MoveEncode(char* Buf, stoneMove & anyMove );
    char* DataEncode(void);
    bool  Beats( bool White, stoneMove & stMove, int stBeat[2] );


    game(int index=0, int connection_socket_descriptor=0);
};

char* game::DataEncode( void )
{
    char Code[6][3] = {
        "--", // - puste miejsce    0
        "wp", // - bialy pionek     1
        "bp", // - czarny pionek    2
        "wd", // - biala damk       3
        "bd", // - czarna damka     4
        "xx"  // - niedostepne pole 5
        };

    strcpy( Buf, Data.whiteSide ? "white;" : "black;" );
    
    for( int row=0; row<BOARD_SIZE; row++ )
        {
        for( int col=0; col<BOARD_SIZE; col++ )
            {
            strcat( Buf, Code[ Data.gameBoard[row][col] ] );
            strcat( Buf, "," );
            }
        Buf[ strlen( Buf )-1 ] = '.';
        }
    Buf[ strlen( Buf )-1 ] = ';';
    strcat( Buf, "Grasz białymi - " ); strcat( Buf, MsgWhite );
    strcat( Buf, ";" );
    strcat( Buf, "Grasz czarnymi - " );strcat( Buf, MsgBlack );
    return Buf;
}

void game::MoveDecode(char* Buf, stoneMove & anyMove ) // "white;1;2;3;4" OK
{
    char Tmp[100];

    SplitStr( Buf, Tmp, sizeof(Tmp), ";", 0 );
    anyMove.whiteSide = strcmp( Tmp, "white") == 0;
 
    SplitStr( Buf, Tmp, sizeof(Tmp), ";", 1 );
    anyMove.FieldFrom[0] =  atoi(Tmp);
    SplitStr( Buf, Tmp, sizeof(Tmp), ";", 2 );
    anyMove.FieldFrom[1] =  atoi(Tmp);

    SplitStr( Buf, Tmp, sizeof(Tmp), ";", 3 );
    anyMove.FieldTo[0] =  atoi(Tmp);
    SplitStr( Buf, Tmp, sizeof(Tmp), ";", 4 );
    anyMove.FieldTo[1] =  atoi(Tmp);


    //printf("decode:%s %d,%d-%d,%d\n", Buf, anyMove.FieldFrom[0] , anyMove.FieldFrom[1] , anyMove.FieldTo[0], anyMove.FieldTo[1] );
}

char* game::MoveEncode(char* Buf, stoneMove & anyMove ) // "white;1,2;3,4" OK
{
    sprintf( Buf, "%s;%d,%d;%d,%d", anyMove.whiteSide ? "white" : "black", anyMove.FieldFrom[0], anyMove.FieldFrom[1], anyMove.FieldTo[0], anyMove.FieldTo[1] );
    return Buf;
}

void game::SetMsg( const char* MsgWhite, const char* MsgBlack)
{
    strcpy( this->MsgWhite, MsgWhite );
    strcpy( this->MsgBlack, MsgBlack );
}

gameData* game::ClientData( bool White) 
{
    strcpy( Data.Msg, White ? MsgWhite : MsgBlack);
    return &Data;
}

game::game(int index, int connection_socket_descriptor) {
    this->index=index;
    Data.whiteSide=true;

    // ustaw board
    memset(Data.gameBoard,0,sizeof(Data.gameBoard));
    
    for(int row=0; row<BOARD_SIZE; row++) {
        for(int col=0; col<BOARD_SIZE; col++)
            {
            if( !((col+row)%2) )
                Data.gameBoard[row][col]=5; // niedostepne
            else if(row<3)
                Data.gameBoard[row][col]=1; // bialy
            else if(row>4)
                Data.gameBoard[row][col]=2; // czarny
            else
                Data.gameBoard[row][col]=0; // pusty
            }
        }

    socketWhite=connection_socket_descriptor;
    socketBlack=0;
    moveCounter=0;
    moveTime=0;
    gameTime=0;
};

const char* MsgWaitPartnerMove = "Zaczekaj na ruch partnera!";
const char* MsgClear = "";
const char* MsgMakeMove = "Masz parę - graj";

bool game::Move( bool White, char* strMove) 
{     
    stoneMove stMove;
    MoveDecode( strMove, stMove );
    if( MoveOk( White, stMove ) )
        {
        int stBeat[2] ;
        if( Beats( White, stMove, stBeat ) )
            Data.gameBoard[ stBeat[0] ][ stBeat[1] ] = 0;

        Data.gameBoard[ stMove.FieldTo[0] ][ stMove.FieldTo[1] ]=Data.gameBoard[ stMove.FieldFrom[0] ][ stMove.FieldFrom[1] ];

        Data.gameBoard[ stMove.FieldFrom[0] ][ stMove.FieldFrom[1] ]=0; // co z damką i biciem? todo
        Data.whiteSide = !Data.whiteSide;

        printf( "ruch ok: %s\n", strMove);
        }
    return false;
}; 


bool game::MoveOk( bool White, stoneMove & stMove )
{
    //return true;

    bool Ret = false;
    int stBeat[2];

    SetMsg(MsgClear,MsgClear);    

    if( White != Data.whiteSide )
        strcpy( White ? MsgWhite : MsgBlack, MsgWaitPartnerMove );
    else if( stMove.FieldTo[0] > 7 || stMove.FieldTo[1] > 7 || stMove.FieldTo[0] < 0 || stMove.FieldTo[1] < 0 )
        strcpy( White ? MsgWhite : MsgBlack, "Przekroczenie zakresu" );
    else if( Data.gameBoard[ stMove.FieldTo[0] ][ stMove.FieldTo[1] ] != 0 )
        strcpy( White ? MsgWhite : MsgBlack, "pole jest zajęte" );

    // bicie ??
    else if( Beats( White, stMove, stBeat ) )
        Ret = true;
    else if( abs(stMove.FieldTo[1]-stMove.FieldFrom[1])!=1  
          && ((stMove.FieldTo[0]-stMove.FieldFrom[0])*(White ? 1 : -1)) != 1 )  
        strcpy( White ? MsgWhite : MsgBlack, "błędny ruch" );
    else
        Ret = true;

    return Ret;   
}

bool game::Beats( bool White, stoneMove & stMove, int stBeat[2] )
{
    //return false;

    bool Ret = false;
    if( abs(stMove.FieldTo[1]-stMove.FieldFrom[1])==2 
        && abs(stMove.FieldTo[0]-stMove.FieldFrom[0])==2 )
        {
        stBeat[0] = stMove.FieldFrom[0] + (stMove.FieldTo[0]-stMove.FieldFrom[0])/2 ;
        stBeat[1] = stMove.FieldFrom[1] + (stMove.FieldTo[1]-stMove.FieldFrom[1])/2 ;

        int State = Data.gameBoard[ stBeat[0] ][ stBeat[1] ];
        if( White )
            Ret = ( State == 2 ) || ( State == 4 );
        else
            Ret = ( State == 1 ) || ( State == 3 );
        }
    return Ret;
}
  
//------------------------------- players ---------------------------------
#define QUEUE_SIZE 3 // max liczba partii


class players // gry+gracze
{
private:
    game* games[QUEUE_SIZE];

public:
    int size;
    void  Del(game* myGame);
    game* Add( int connection_socket_descriptor );
    game* Find( int connection_socket_descriptor, bool & White );
    game* Pair( int connection_socket_descriptor );
    game* Move( int connection_socket_descriptor, char* smove );

    players(void)
    {
        memset(games,0,sizeof(games));
        size = 0;
    };
    ~players( void );
};

players::~players( void )
{
    for( int idx = 0; idx < QUEUE_SIZE; idx++ )
        if( games[idx] ) delete games[idx];

}

game* players::Move( int connection_socket_descriptor, char* smove )
{
    bool White; 

    game* myGame = Find( connection_socket_descriptor, White );

    //printf("Move %d-2\n", connection_socket_descriptor);

    if( myGame ) 
        { // gra w toku
        if( !myGame->socketBlack ) // czy czarny wlaczyl sie do gry?
            ;    
        else if( myGame->Move( White, smove ) ) // zakoncz
            {
            Del( myGame );
            myGame = NULL; // komunikat ???
            }
        }
    else 
        { // nowa gra
        myGame = Pair( connection_socket_descriptor );  // poszukaj wolnej pary  
        if( !myGame )  // brak wolnej pary
            myGame = Add( connection_socket_descriptor );
        }
    return myGame;
}   

game* players::Pair( int connection_socket_descriptor )  // poszukaj wolnej pary 
{
    game* myGame = NULL;
    int idx = 0;
    while( idx < QUEUE_SIZE && (!games[idx] || games[idx]->socketBlack )) idx++;
    
    //printf("parowanie %d - idx %d\n", connection_socket_descriptor, idx);
    if(idx < QUEUE_SIZE) 
        {
        myGame = games[idx];
        myGame->SetMsg(MsgMakeMove,MsgWaitPartnerMove);    
        myGame->socketBlack = connection_socket_descriptor;

        //printf("para %d - %d\n", myGame->socketWhite, myGame->socketBlack );       
        }
    return myGame;
} 


game* players::Add( int connection_socket_descriptor )
{
    game* myGame = NULL;
    int idx = 0;
    while( idx < QUEUE_SIZE && games[idx] ) idx++;
    
    if(idx < QUEUE_SIZE) {
        myGame = new game (idx, connection_socket_descriptor);
        myGame->SetMsg("Czekaj na przystąpienie partnera", "");   
        games[idx] = myGame; 
        size ++;
        }

    return myGame;
}
 
void players::Del( game* myGame )
{   
    if( myGame )
        {
        int idx = 0;
        while( idx < QUEUE_SIZE && games[idx] != myGame ) idx++;

        if(idx < QUEUE_SIZE) 
            {
            // Zamknięcie połączenia/gniazda:
            if( myGame->socketBlack ) close( myGame->socketBlack );
            if( myGame->socketWhite ) close( myGame->socketWhite );

            delete games[idx];
            games[idx]=NULL;
            size--;
            }
        }
}

game* players::Find( int connection_socket_descriptor, bool & White )
{
    int idx = 0;
    game* myGame = NULL;
    while( idx < QUEUE_SIZE
           && ( !games[idx] ||
              ( games[idx]->socketWhite != connection_socket_descriptor && games[idx]->socketBlack != connection_socket_descriptor ))) idx++;

    if(idx < QUEUE_SIZE) {
        White = games[idx]->socketWhite == connection_socket_descriptor;
        myGame = games[idx];
        }
    
    return myGame;
}

//----------------------- server start ---------------------------------------------------------------------------

#define SERVER_PORT 9001

players* Players = NULL;


//funkcja opisującą zachowanie wątku - musi przyjmować argument typu (void *) i zwracać (void *)
void *ThreadBehavior(void *pconnection_socket_descriptor)
{
    int connection_socket_descriptor = *(int*) pconnection_socket_descriptor; 

    //  pthread_detach odlacza wolajacy watek, co gwarantuje zwolnienie zasobow pamieci natychmiast po zakonczeniu dzialania watku, 
    pthread_detach( pthread_self() );
	 

    //-------- test ---------------------------------------------
    char Buf[ 2000 ];
    int cread;
    memset(Buf, 0, 2000);
    
    while( (cread = recv(connection_socket_descriptor, Buf, 2000 , 0)) > 0 )
        {
        //write(connection_socket_descriptor, "OK" , 2); // bez wyslania odpowiedzi klient sie zawiesza
        Buf[cread]=0;    
        //printf("wysyłam1: %d %s\n", connection_socket_descriptor, Buf);
        
        if( cread > 10 ) {
            //--- obsluga zapytania -----
            game* myGame = Players->Move( connection_socket_descriptor, Buf );
            if( myGame )
                {
                char* Data = myGame->DataEncode();
                int csent;    //wyslij do bilych i do czarnych

                // printf("Data %s\n", Data );
                // printf("Sockets white::%d, black::%d\n", myGame->socketWhite, myGame->socketBlack);


                if( myGame->socketBlack ) 
                    {
                    strcpy( Buf, "black;" );
                    strcat( Buf, Data );    
                    printf("odpowiedź do black %d\n", myGame->socketBlack);
                    printf( "%s\n", Buf );
                    csent = write( myGame->socketBlack, Buf, strlen( Buf )) ;
                    }
                if( myGame->socketWhite )
                    {
                    strcpy( Buf, "white;" );
                    strcat( Buf, Data ); 
                    printf("odpowiedź do white %d\n", myGame->socketWhite);
                    printf( "%s\n", Buf );
                    csent = write( myGame->socketWhite, Buf, strlen( Buf ) );
                    }
                }
            }
            
        //---------------------------
        memset(Buf, 0, 2000);
        }

   
	// Zamknięcie połączenia/gniazda:
    //close(connection_socket_descriptor); //usuwanie gry

    delete (int*) pconnection_socket_descriptor;

    printf("koniec %d\n", connection_socket_descriptor);

	// zakonczenie dzialania watku
    pthread_exit(NULL);
}

//funkcja obsługująca połączenie z nowym klientem
void handleConnection(int connection_socket_descriptor) {
    
    int create_result = 0; //wynik funkcji tworzącej wątek

    //uchwyt na wątek
    pthread_t thread1;

    int* ptr = new int;
    *ptr = connection_socket_descriptor;


    // tworzenie wątku z obsługą przez ThreadBehavior
	// pthread_create tworzy nowy watek, ktory wykonuje sie wspolbieznie z watkiem wolajacym. 
	// Nowy watek zaczyna wykonywac podana funkcje start_routine podajac jej arg jako pierwaszy argument. 
	// Nowy watek konczy sie explicite przez wywolanie procedury pthread_exit lub przez powrot z start_routine.	 	 
    create_result = pthread_create(&thread1, NULL, ThreadBehavior, ptr );


    if (create_result){
       printf("Błąd przy próbie utworzenia wątku, kod błędu: %d\n", create_result);
       exit(-1);
    }

}

int main(int argc, char* argv[])
{
   int server_socket_descriptor;
   int connection_socket_descriptor;
   int bind_result;
   int listen_result;
   char reuse_addr_val = 1;
   struct sockaddr_in server_address;
   int Port = SERVER_PORT;


   if( argc > 1 )
        Port = atoi( argv[1]);


   //inicjalizacja gniazda serwera
   printf("Server is starting, port:%d\n", Port);  
   
   memset(&server_address, 0, sizeof(struct sockaddr));
   server_address.sin_family = AF_INET;
   server_address.sin_addr.s_addr = htonl(INADDR_ANY); // bind to all available interfaces, htonl - kolejność bajtów
   server_address.sin_port = htons(Port);

	// Tworzenie gniazda
   server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
   if (server_socket_descriptor < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
       exit(1);
   }
	
	// opcje gniazda -	
	// SO_REUSEADDR - zezwala na ponowne wykorzystanie tego samego numeru portu przez gniazdo bez koniecznosci odczekiwania czasu 
    // okreslonego stala TCP_TIMEWAIT_LEN. 
	// gniazdo zamkniete instrukcja close() jest zachowywane w strukturach jadra w stanie TIME_WAIT, 
	// ale dozwolone jest jego usuniecie przez przypisanie innemu gniazdu tego samego numeru portu.	
   //setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));

	// Związanie gniazda
   bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
   if (bind_result < 0)
   {
       fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
       exit(1);
   }

	// Przygotowanie gniazda na obsługę połączeń i konfiguracja kolejki żądań
   listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
   if (listen_result < 0) {
       fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
       exit(1);
   }

	
    Players = new players();

    // Oczekiwanie i obsługa zgłoszeń
    while(1)
        {
        //printf("przyszlo ?\n");
        connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);
        //printf("przyszlo %d\n", connection_socket_descriptor);
        if (connection_socket_descriptor < 0)
            {
            fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
            exit(1);
            }

            handleConnection(connection_socket_descriptor);
        }
    delete Players;
    Players =NULL;

    close(server_socket_descriptor);
    return(0);
}
