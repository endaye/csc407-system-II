/*-------------------------------------------------------------------------*
 *---                                                                   ---*
 *---           spaceInvadersClient.cpp                                 ---*
 *---                                                                   ---*
 *---    This file defines the client for the space invaders program.   ---*
 *---                                                                   ---*
 *---   ----    ----    ----    -----    ----    ----    ----    ----   ---*
 *---                                                                   ---*
 *---   Version 1.0             2011 February 28      Joseph Phillips ---*
 *---                                                                   ---*
 *--- Version 2.0   2014 November 06  Joseph Phillips ---*
 *---                                                                   ---*
 *---      Made multi-threaded, more self-contained.                    ---*
 *---                                                                   ---*
 *-------------------------------------------------------------------------*/


/*
 * Compile with:
 *  g++ -o spaceInvadersClient spaceInvadersClient.cpp -lncurses -lpthread
 */


#include  "headers.h"
#include  <pthread.h> // For pthread_create(), pthread_join()
#include  <unistd.h>  // For sleep()
#include  <sys/socket.h>  // For socket()
#include  <netdb.h> // For getaddrinfo()
#include  <errno.h> // For errno var


//                  //
//    Types and classes specific to this program:   //
//                  //

//  PURPOSE:  To hold information about the server to which to connect.
//  (Assumes there are global constants:
//  const char*   INITIAL_HOST  (e.g. = "localhost.localdomain")
//  const int     INITIAL_PORT  (e.g. = 20000)
//  const int     C_STRING_MAX  (e.g. = 256)

 class  ServerCommInfo
 {
  //  0.  Constants:
  //  PURPOSE:  To tell the char used to separate the hostname from the
  //  port number in a user-provided URL.
  static const char HOST_NAME_PORT_SEPARATORY_CHAR    = ':';

  //  PURPOSE:  To tell the char used to separate specific domains in a URL
  static const char IP_ADDR_SUBDOMAIN_SEPARATORY_CHAR = '.';


  //  I.  Member vars:a
  //  PURPOSE:  To hold the name of the server.
  char    hostName[C_STRING_MAX];

  //  PURPOSE:  To hold the port to which to connect on the server.
  int     portNumber;

  //  PURPOSE:  To hold the file descriptor being used to talk with the server.
  int   connectFD;

  //  II.  Disallowed auto-generated methods:
  //  No copy constructor:
  ServerCommInfo    (const ServerCommInfo& );

  //  No copy-assignment op:
  ServerCommInfo& operator=(const ServerCommInfo&);

  protected :
  //  III.  Protected methods:

  public :
  //  IV.  Constructor(s), assignment op(s), factory(s) and destructor:
  //  PURPOSE:  To give '*this' its initial (illegal) values.  No parameters.
  //  No return value.
  ServerCommInfo    ()
  throw() :
  portNumber(-1),
  connectFD(-1)
  {
    //  I.  Application validity check:
    //  II.  Initialize members:
    hostName[0] = '\0';

    //  III.  Finished:
  }

  //  PURPOSE:  To release resources.  No parameters.  No return value.
  ~ServerCommInfo   ()
  throw()
  {
    //  I.  Application validity check:
    //  II.  Release resources:
    if  (getConnectFD() != -1)
    {
      //  YOUR CODE HERE to close 'getConnectFD()'
      close(getConnectFD());
    }

    //  III.  Finished:
  }

  //  V.  Accessors:
  //  PURPOSE:  To return a pointer to the name of the server.  No parameters.
  const char* getHostNamePtr  ()
  const
  throw()
  { return(hostName); }

  //  PURPOSE:  To return the port to which to connect on the server.  No
  //  parameters.
  int     getPortNumber ()
  const
  throw()
  { return(portNumber); }

  //  PURPOSE:  To return the file descriptor being used to talk with the
  //  server.  No parameters.
  int   getConnectFD  ()
  const
  throw()
  { return(connectFD); }


  //  VI.  Mutators:

  //  VII.  Methods that do main and misc work of class:
  //  PURPOSE:  To attempt to initialize 'hostName' and 'portNumber' by parsing
  //  'urlNamePtr'.  Initializes 'hostName' to 'INITIAL_HOST' if hostname
  //  not apparent in 'urlNamePtr'.  Initializes 'portNumber' to
  //  'INITIAL_PORT' if port number not apparent in 'urlNamePtr'.  Returns
  //  'true' if parse and initialization deemed successful, or 'false'
  //  otherwise.
  bool    didParse  (const char*  urlNamePtr)
  throw()
  {
    //  I.  Application validity check:
    //  I.A.  Disallow missing names:
    if  (urlNamePtr == NULL)
      return(false);

    //  I.B.  Disallow empty names:
    //  I.B.1.  Fast-forward past any spaces:
    while  ( isspace(*urlNamePtr) )
      urlNamePtr++;

    //  I.B.2.  Disallow empty names:
    if  (*urlNamePtr == '\0')
      return(false);

    //  II.  Attempt to parse 'urlNamePtr':
    //  II.A.  Look for char that signifies both hostname and port might be
    //         available:
    char* cPtr;
    const char* ccPtr = strchr(urlNamePtr,HOST_NAME_PORT_SEPARATORY_CHAR);

    //  II.B.  Handle according to whether or not found
    //         'HOST_NAME_PORT_SEPARATORY_CHAR':
    if  (ccPtr == NULL)
    {
      //  II.B.1.  Case did *not* find 'HOST_NAME_PORT_SEPARATORY_CHAR'.
      //       Assume what was given was either all hostname or all port.
      //  II.B.1.a.  Give both 'hostName' and 'portNumber' default values:
      strncpy(hostName,INITIAL_HOST,C_STRING_MAX-1);
      hostName[C_STRING_MAX-1]  = '\0';
      portNumber    = INITIAL_PORT;

      //  II.B.1.b.  See if 'urlNamePtr' begins with a digit:
      if  ( isdigit(*urlNamePtr) )
      {
  //  II.B.1.b.I.  'urlNamePtr' *does* begin with a digit.  It could
  //     either be a host number (e.g. 127.0.0.1) or a port
  //     (e.g. 20000).
  //  II.B.1.b.I.A.  A host number is expected to have at least one
  //        'IP_ADDR_SUBDOMAIN_SEPARATORY_CHAR':
       if  ( strchr(urlNamePtr,IP_ADDR_SUBDOMAIN_SEPARATORY_CHAR) == NULL )
       {
    //  II.B.1.b.I.A.1. No 'IP_ADDR_SUBDOMAIN_SEPARATORY_CHAR' found,
    //            assume number is port number:
    //  II.B.1.b.I.A.1.a.  Get port number:
         portNumber = strtol(urlNamePtr,&cPtr,10);

    //  II.B.1.b.I.A.1.b.  Complain if extraneous chars found:
         if  ( (*cPtr != '\0')  &&  !isspace(*cPtr) )
           return(false);
       }
       else
       {
    //  II.B.1.b.I.A.2. 'IP_ADDR_SUBDOMAIN_SEPARATORY_CHAR' *is* found,
    //          assume number is host number:
         strncpy(hostName,urlNamePtr,C_STRING_MAX-1);
         hostName[C_STRING_MAX-1] = '\0';
       }

     }
     else
     {
  //  II.B.1.b.II.  'urlNamePtr' does *not* begin with a digit.  Must be
  //      host name:
       strncpy(hostName,urlNamePtr,C_STRING_MAX-1);
       hostName[C_STRING_MAX-1] = '\0';
     }

   }
   else
   {
      //  II.B.2.  Case *did* find 'HOST_NAME_PORT_SEPARATORY_CHAR'.
      //     Assume have both hostname and port:
      //  II.B.2.a.  Get hostname portion:
      //  II.B.2.a.I.  Determine how many chars belong to the hostname:
    int numCharsToCopy  = C_STRING_MAX-1;
    int hostNameNumChars= (ccPtr - urlNamePtr);

      //  II.B.2.a.I.A.  ... zero is not an acceptable answer:
    if  (hostNameNumChars == 0)
      return(false);

    if  (hostNameNumChars < numCharsToCopy)
      numCharsToCopy  = hostNameNumChars;

      //  Get II.B.2.a.II.  Get 'hostName[]':
    strncpy(hostName,urlNamePtr,numCharsToCopy);
    hostName[numCharsToCopy]  = '\0';

      //  II.B.2.b.  Get portnum:
      //  II.B.2.b.I.  Make sure it looks like a number:
    if  ( !isdigit(*(ccPtr+1)) )
     return(false);

      //  II.B.2.b.II.  Get the number:
   portNumber = strtol(ccPtr+1,&cPtr,10);

      //  II.B.2.b.III.  Complain if extraneous chars found:
   if  ( (*cPtr != '\0')  &&  !isspace(*cPtr) )
    return(false);
}

    //  III.  Finished:
return(true);
}


  //  PURPOSE:  To set 'connectFD' to a file descriptor of a socket to connect
  //  with the server.  Returns 'true' if successful or 'false' otherwise.
bool    didConnect  ()
throw()
{
    //  I.  Parameter validity check:

    //  II.  Attempt to get socket descriptor:
    //  YOUR CODE HERE:
    //  Please attempt to set member variable 'connectFD' to a file descriptor
    //  of a socket for talking with the server whose name is given by
    //  'getHostNamePtr()' and whose port is given by 'getPortNumber()'.
  const char* machineName = getHostNamePtr();
  int port = getPortNumber();

// Create a socket
  connectFD = socket(AF_INET, // AF_INET domain
        SOCK_STREAM, // Reliable TCP
        0);
  struct addrinfo* hostPtr;
  int status = getaddrinfo(machineName,NULL,NULL,&hostPtr);

  if (status != 0)
  {
    fprintf(stderr,gai_strerror(status));
    return(false);
  }

  // Connect to server
  struct sockaddr_in server;
  // Clear server datastruct
  memset(&server, 0, sizeof(server));
  // Use TCP/IP
  server.sin_family = AF_INET;
  // Tell port # in proper network byte order
  server.sin_port = htons(port);
  // Copy connectivity info from info on server ("hostPtr")
  server.sin_addr.s_addr =
  ((struct sockaddr_in*)hostPtr->ai_addr)->sin_addr.s_addr;
  status = connect(connectFD,(struct sockaddr*)&server,sizeof(server));

  if  (status < 0)
  {
    fprintf(stderr,"Could not connect %s:%d\n",machineName,port);
    return(false);
  }


    //  III.  If get here then have connected to server:
  return(true);
}

};



//                  //
//          Global constants:       //
//                  //

//  PURPOSE:  To tell a sequence of strings to be used over time to display
//  the live invaders.
const char* liveInvader[NUM_INVADER_FRAMES]   = {"-*-","/*/","|*|","\\*\\"};

//  PURPOSE:  To tell the string to use to display dead invaders.
const char  deadInvader[COLS_PER_INVADER+1]   = "   ";

//  PURPOSE:  To tell the string to use to display the defender.
const char  defender[DEFENDER_WIDTH+1]    = "/|\\";


//                  //
//          Global variables:       //
//                  //

//  PURPOSE:  To hold 'true' while the game is still on, or 'false' otherwise.
bool    shouldContinueGame      = true;


//  PURPOSE:  To serve as a global space into which formatted error messages
//  and other text may be written.
char    cText[C_STRING_MAX];


//  PURPOSE:  To point to the window used for most of the output.
WINDOW*          mainWindowPtr;


//  PURPOSE:  To point to the window used for error output.
WINDOW*          errorWindowPtr;


//                  //
//          Global functions:       //
//                  //

//  PURPOSE:  To be the "robust" version of read() in the manner advocated by
//  Bryant and O'Hallaron.  'fd' tells the file descriptor from which to
//  read.  'usrbuf' tells the buffer into which to read.  'n' tells the
//  length of 'usrbuf'.  Returns number of bytes read or -1 on some type
//  of error.
ssize_t rio_read  (int  fd, char* usrbuf, size_t  n)
throw()
{
  //  I.  Application validity check:

  //  II.  Attempt to read:
  //  YOUR CODE HERE
  //  (Well, actually Bryant's and O'Hallaron's code from the end of lecture 8)

  size_t  nleft = n;
  ssize_t nread;
  char*   bufp = usrbuf;
  while (nleft > 0) {
    if ((nread = read(fd, bufp, nleft)) < 0) 
    {
      if (errno==EINTR) nread = 0;      
      else return -1;  
    }    
    else if (nread == 0) break;
    nleft -= nread;
    bufp  += nread;
  }
  return (n - nleft);
  

  //  III.  Finished:
}



//  PURPOSE:  To initialize the communication in 'serverCommInfo' from the
//  command line argument parameters 'argc' and 'argv', and from whatever
//   else the user enters.  No return value.
void  initializeCommParams (int argc, const char* argv[], ServerCommInfo& serverCommInfo)
throw()
{
  //  I.  Parameter validity check:

  //  II.  Initialize 'portNumber' and 'hostName':
  char  hostNameSpace[C_STRING_MAX];
  char  defaultHostName[C_STRING_MAX];

  snprintf(defaultHostName,C_STRING_MAX,"%s:%d",INITIAL_HOST,INITIAL_PORT);

  if  ( (argc <= 1)  ||  !serverCommInfo.didParse(argv[1]) )
  {
    do
    {
      printf("Hostname:Port [%s]? ",defaultHostName);
      fgets(hostNameSpace,C_STRING_MAX,stdin);

      if  (hostNameSpace[0] == '\n')
        strncpy(hostNameSpace,defaultHostName,C_STRING_MAX);
    }
    while  ( !serverCommInfo.didParse(hostNameSpace) );
  }

  //  III.  Finished:
}



//  PURPOSE:  To initialize ncurses in general, and 'mainWindowPtr' and
//  'errorWindowPtr' in particular.  No return value.
void  startGame (const ServerCommInfo&  serverCommInfo)
throw ()
{
  //  I.  Parameter validity check:

  //  II.  Initiailize window:
  //  YOUR CODE HERE:
  //  * Start ncurses
  //  * Turn off buffering (say cbreak())
  //  * Clear the screen
  //  * Make getch() quit after 0.5 seconds (to check if game is still on)
  //    (say halfdelay(5))
  //  * Allow usage of arrow keys
  //  * Don't echo chars when typed
  //    (We'll place them on screen ourselves
  //     only if they are a printable char)
  //  * Turn off scrolling in the main window
  //  * Set 'mainWindowPtr' to be a subwindow that starts at row 0, column 0.
  //    It should be 'MAX_NUM_ROWS-1' rows high and 120 columns wide.
  //  * Set 'errorWindowPtr' to be a subwindow that starts at row
  //    'MAX_NUM_ROWS-1', column 0.  It should be 1 row high and 120 columns
  //  wide.
  initscr();
  cbreak();
  clear();
  usleep(500);
  halfdelay(5);
  keypad(stdscr,TRUE);
  noecho();
  scrollok(stdscr,TRUE);
  mainWindowPtr = newwin(MAX_NUM_ROWS-1, 120, 0, 0);
  errorWindowPtr = newwin(1, 120, MAX_NUM_ROWS-1, 0);

  //  III.  Finished:
}


//  PURPOSE:  To listen to the keyboard commands from the user and to send
//  the corresponding space-invader requests to the server.  'vPtr'
//  points to a 'ServerCommInfo' object that holds information on the
//  server that governs the game.  Returns NULL.
void* attendToUser (void* vPtr)
throw()
{
  //  I.  Application validity check:

  //  II.  Attend to user.
  request_t request;
  int   key;
  int   connectFD = ((const ServerCommInfo*)vPtr)->getConnectFD();

  //  II.A.  Each iteration handles one potential keyboard command:
  while  ( (key = getch() ) != QUIT_CHAR )
  {
    //  II.A.1.  Quit loop if game is over:
    if  ( !shouldContinueGame )
      break;

    //  II.A.2.  Handle key:
    switch  (key)
    {
      case ERR :
      break;

    //  YOUR CODE HERE:
    //  If the user typed 'KEY_LEFT' then:
    //  (1) put 'LEFT_REQUEST' in 'request' (in network endianness, it is a short)
    //  (2) send 'REQUEST_LENGTH' bytes in 'request' to file descriptor 'connectFD'
      case KEY_LEFT:
      request = htons(LEFT_REQUEST);
      write(connectFD,&request,REQUEST_LENGTH);
      break;
      
    //  If the user typed 'KEY_RIGHT' then:
    //  (1) put 'RIGHT_REQUEST' in 'request' (in network endianness, it is a short)
    //  (2) send 'REQUEST_LENGTH' bytes in 'request' to file descriptor 'connectFD'
      case KEY_RIGHT:
      request = htons(RIGHT_REQUEST);
      write(connectFD,&request,REQUEST_LENGTH);
      break;

    //  If the user typed space or newline then:
    //  (1) put 'SHOOT_REQUEST' in 'request' (in network endianness, it is a short)
    //  (2) send 'REQUEST_LENGTH' bytes in 'request' to file descriptor 'connectFD'
      case '\n':
      request = htons(SHOOT_REQUEST);
      write(connectFD,&request,REQUEST_LENGTH);
      break;

      case ' ':
      request = htons(SHOOT_REQUEST);
      write(connectFD,&request,REQUEST_LENGTH);
      break;

    //  If the user typed anything else then:
    //  Do 'beep()'
      default:
      beep();
    }

  }

  //  II.B.  Tell server about disconnect request (if one was made): 
  if  (key == QUIT_CHAR)
  {
    //  (1) put 'DISCONNECT_REQUEST' in 'request' (in network endianness, it is a short)
    //  (2) send 'REQUEST_LENGTH' bytes in 'request' to file descriptor 'connectFD'
    request = htons(DISCONNECT_REQUEST);
    write(connectFD,&request,REQUEST_LENGTH);
  }

  //  III.  Finished:
  return(NULL);
}


//  PURPOSE:  To tell the user that a connection to the server 
//  '*serverCommInfoPtr' was denied.  No return value.
void  handleConnectionDenied  (const ServerCommInfo*  serverCommInfoPtr
  )
throw()
{
  //  I.  Application validity check:

  //  II.  Tell that connection was denied:
  snprintf(cText,C_STRING_MAX,
    "%s:%d is alive but refused our request to connect, sorry.",
    serverCommInfoPtr->getHostNamePtr(),
    serverCommInfoPtr->getPortNumber()
    );
  //  YOUR CODE HERE
  //  * Move to row 10, column 0 of the WHOLE screen
  //  * Print 'cText' to the WHOLE screen
  //  * Make the text visible
  //  * 'sleep()' for 6 seconds
  move(10, 0);
  addstr(cText);
  refresh();
  sleep(6);

  //  III.  Finished:
}


//  PURPOSE:  To display the board as described in 'update[]'.  'ouchCount'
//  tells how many times the defender has been hit.
//  'bottommostInvaderRankRowPtr' points to a 'short' integer which holds
//  the invader's bottommost rank's row on the screen.
//  'leftMostInvaderColPtr' points to a 'short' integer which holds
//  the invader's leftmost file's column.  No return value.
void  handleWholeBoard  (char*    update,
 int    ouchCount,
 short*   bottommostInvaderRankRowPtr,
 short*   leftMostInvaderColPtr
 )
throw()
{
  //  I.  Application validity check:

  //  II.  Update whole board:
  //   *  Whole board update syntax:
  //   *  "ww"            +
  //   *  bottom-most invader rank row (16-bit int) +
  //   *  left-most invader col (16-bit int)    +
  //   *  1st rank invader boolean packed bit 32-bit int  +
  //   *  . . .             +
  //   *  last rank invader boolean packed bit 32-bit int +
  //   *  1st invader bullet row (16-bit int)   +
  //   *  1st invader bullet col (16-bit int)   +
  //   *  . . .
  //   *  last invader bullet row (16-bit int)      +
  //   *  last invader bullet col (16-bit int)      +
  //   *  defender 0 col (16-bit int)     +
  //   *  defender 1 col (16-bit int)     +
  //   *  defender 0's bullet row (16-bit int)    +
  //   *  defender 0's bullet col (16-bit int)    +
  //   *  defender 1's bullet row (16-bit int)    +
  //   *  defender 1's bullet col (16-bit int)
  static int  interval  = 0;
  short   row;
  short   col;
  short   index;

  //  II.A.  Clear the screen:
  //  YOUR CODE HERE:
  //  * clear 'mainWindowPtr'
  wclear(mainWindowPtr);

  //  II.B.  Get 'bottommostInvaderRankRow' and 'leftMostInvaderCol':
  //  YOUR CODE HERE:
  *bottommostInvaderRankRowPtr   = ntohs(  *(((short*)update) + 1)); // <-- Replace with
                //  *(((short*)update) + 1), but in
              // host endianness.
  *leftMostInvaderColPtr   = ntohs(  *(((short*)update) + 2)); // <-- Replace with 
                //  *(((short*)update) + 2), but in
              // host endianness.

  //  II.C.  Display live invaders:
  char*   bufferCursor  = (char*)(((short*)update) + 3); // <-- Replace with
              // (char*)(((short*)update) + 3)

  interval++;

  for  (short rankIndex = 0;  rankIndex < NUM_INVADER_RANKS;  rankIndex++)
  {
    int   currentBitPosition  = 0x1;
    //  YOUR CODE HERE:
    int   bitArray    = ntohl(*((int*)bufferCursor)); // <-- Replace with
                   // *((int*)bufferCursor) converted
               // to host endianness
               // (it is 32 bits)

    bufferCursor += SIZE32;

    for  (short fileIndex = 0; fileIndex < NUM_INVADERS_PER_RANK; fileIndex++)
    {

      if  ( (bitArray & currentBitPosition) != 0 )
      {
       row  = getInvaderRowGivenRankAndBottommostRankRow
       (rankIndex,*bottommostInvaderRankRowPtr);
       col  = getInvadersLeftmostColGivenFileAndLeftmostCol
       (fileIndex,*leftMostInvaderColPtr);
  //  YOUR CODE HERE:
  //  Move to 'row', 'col' of 'mainWindowPtr' and print
  //  'liveInvader[interval % NUM_INVADER_FRAMES]'
       wmove(mainWindowPtr, row, col);
       waddstr(mainWindowPtr,liveInvader[interval % NUM_INVADER_FRAMES]); 
     }

     currentBitPosition <<= 1;
   }

 }

  //  II.D.  Display live invader bullets:
 for  (index = 0;  index < MAX_NUM_INVADER_BULLETS;  index++)
 {
    //  YOUR CODE HERE:
    row      = ntohs(*(short*)bufferCursor); // <-- Replace with '*(short*)bufferCursor'
                // changed to host endianness
    bufferCursor  += SIZE16;
    col      = ntohs(*(short*)bufferCursor); // <-- Replace with '*(short*)bufferCursor'
                // changed to host endianness
    bufferCursor  += SIZE16;

    if  ( (row != ILLEGAL_ROW)  &&  (col != ILLEGAL_COL) )
    {
      //  YOUR CODE HERE
      //  move to 'row', 'col' of 'mainWindowPtr' and display character '*'.
      wmove(mainWindowPtr, row, col);
      waddch(mainWindowPtr,'*');
    }

  }

  //  II.E.  Display live defender:
  col    = ntohs(*(short*)bufferCursor); // <-- Replace with '*(short*)bufferCursor'
          // changed to host endianness
  bufferCursor  += SIZE16;

  if  (col != ILLEGAL_COL)
  {
    //  YOUR CODE HERE
    //  move to 'defenderRow', 'col' of 'mainWindowPtr' and display string
    //  'defender'.
    wmove(mainWindowPtr, defenderRow, col);
    waddstr(mainWindowPtr, defender);
  }

  //  II.F.  Display live defender bullet:
  row    = ntohs(*(short*)bufferCursor); // <-- Replace with '*(short*)bufferCursor'
          // changed to host endianness
  bufferCursor  += SIZE16;
  col    = ntohs(*(short*)bufferCursor); // <-- Replace with '*(short*)bufferCursor'
          // changed to host endianness
  bufferCursor  += SIZE16;

  if  ( (row != ILLEGAL_ROW)  &&  (col != ILLEGAL_COL) )
  {
    //  YOUR CODE HERE
    //  move to 'row', 'col' of 'mainWindowPtr' and display character '|'.
    wmove(mainWindowPtr, row, col);
    waddch(mainWindowPtr,'|');
  }

  //  II.G.  Display 'ouchCount':
  snprintf(cText,C_STRING_MAX,"Ouch count: %d",ouchCount);
  //  YOUR CODE HERE
  //  Move to 0,0 of 'mainWindowPtr' and display 'cText':
  wmove(mainWindowPtr, 0, 0);
  waddstr(mainWindowPtr,cText);

  //  III.  Finished:
  //  YOUR CODE HERE:
  //  Make all of your changes to 'mainWindowPtr' visible.
  wrefresh(mainWindowPtr);
}


//  PURPOSE:  To handle when the defender wins.  No parameters.  No return
//  value.
void  handleWon   ()
throw()
{
  //  I.  Application validity check:

  //  II.  Display update:
  //  YOUR CODE HERE:
  //  * Clear the WHOLE screen
  //  * Move to row 10, column 20 of the WHOLE screen
  //  * Write "Congratulations!  You destroyed all the invaders!"
  //  * Make the changes visible
  //  * 'sleep()' for 4 seconds

  clear();
  move(10, 20);
  addstr("Congratulations!  You destroyed all the invaders!");
  refresh();
  sleep(4);

  //  III.  Finished;
}


//  PURPOSE:  To handle when the defender is hit.  'ouchCount' tells the
//  cumulative number of times the defender has been hit.  No return value.
void  handleDefenderHit (int    ouchCount
  )
throw()
{
  //  I.  Application validity check:

  //  II.  Handle the killing of an invader:
  // Defender killed update syntax
  //     "KK"       +
  //     defender num (16-bit int)  +
  //     defender col (16-bit int)

  //  II.  Display update:
  snprintf(cText,C_STRING_MAX,"Ouch count: %d",ouchCount);
  //  YOUR CODE HERE:
  //  * Move to 0, 0 of 'mainWindowPtr'
  //  * Write 'cText' to that window
  //  * Make changes visible
  wmove(mainWindowPtr, 0, 0);
  addstr(cText);


  //  III.  Finished:
}


//  PURPOSE:  To display when a particular invader has been killed.  Which
//  invader is given in 'update', and 'bottommostInvaderRankRow' and
//  'leftMostInvaderCol' tell the bottom-most row and the left-most
//  column of the formation of invaders.
void  handleInvaderKilled (char*  update,
 short  bottommostInvaderRankRow,
 short  leftMostInvaderCol
 )
throw()
{
  //  I.  Application validity check:

  //  II.  Handle the killing of an invader:
  // Invader killed update syntax
  //     "kk"       +
  //     invader rank (16-bit int)  +
  //     invader file (16-bit int)

  //  II.A.  Get arguments from 'update[]'
  //  YOUR CODE HERE:
  short rankIndex   = ntohs(*( ((short*)update) + 1)); // <-- Replace with the short value
             // *( ((short*)update) + 1), converted from
           // network to host endianness
  short fileIndex = ntohs(*( ((short*)update) + 2)); // <-- Replace with the short value
             // *( ((short*)update) + 2), converted from
           // network to host endianness
  short row   = getInvaderRowGivenRankAndBottommostRankRow
  (rankIndex,bottommostInvaderRankRow);
  short col   = getInvadersLeftmostColGivenFileAndLeftmostCol
  (fileIndex,leftMostInvaderCol);

  //  II.B.  Display update:
  //  YOUR CODE HERE:
  //  * Move to 'row','col' of 'mainWindowPtr'
  //  * Print "BOOM" to that window
  //  * Make the window visible
  //  * Sleep 20000 microseconds (use usleep())
  //  * Move to 'row','col' of 'mainWindowPtr'
  //  * Print "    " to that window
  //  * Make the window visible
  wmove(mainWindowPtr,row,col);
  waddstr(mainWindowPtr,"BOOM");
  wrefresh(mainWindowPtr);
  usleep(20000);
  wmove(mainWindowPtr,row,col);
  waddstr(mainWindowPtr,"    ");
  wrefresh(mainWindowPtr);


  //  III.  Finished:
}



//  PURPOSE:  To listen to the updates coming from the server and update the
//  screen accordingly.  'vPtr' points to a 'ServerCommInfo' object that
//  holds information on the server that governs the game.  Returns NULL.
void* attendToServer    (void*  vPtr
  )
throw()
{
  //  I.  Application validity check:

  //  II.  Attend to server:
  char      update[MAX_UPDATE_LEN];
  int     ouchCount   = 0;
  short     bottommostInvaderRankRow=
  INIIAL_BOTTOMMOST_INVADER_RANK_ROW;
  short     leftMostInvaderCol; // Left-most invader col.
  const ServerCommInfo* serverCommInfoPtr = (const ServerCommInfo*)vPtr;

  //  II.A.  Each iteration handles another update from the server:
  while  (shouldContinueGame)
  {
    //  II.A.1.  Get update from server:
    update[0]   = '\0';
    int remoteLen = rio_read(serverCommInfoPtr->getConnectFD(),update,MAX_UPDATE_LEN);  
    // <-- YOUR CODE to replace the '0' with
              // a 'rio_read()' call that reads
        // 'MAX_UPDATE_LEN' bytes from
        // 'serverCommInfoPtr->getConnectFD()' and
        // puts them into 'update'.

    //  II.A.2.  Ignore when nothing was sent:
    if  ( (remoteLen == -1) && (errno == EAGAIN) )
      continue;

    //  II.A.3.  Do update:
    switch  (update[0])
    {
      case CONNECTION_DENIED_UPDATE :
      handleConnectionDenied(serverCommInfoPtr);
      break;

      case DISCONNECT_UPDATE :
      shouldContinueGame = false;
      break;

      case BEEP_UPDATE :
      beep();
      break;

      case BEGIN_WHOLE_BOARD_UPDATE :
      handleWholeBoard
      (update, ouchCount, &bottommostInvaderRankRow, &leftMostInvaderCol);
      break;

      case HAVE_WON_UPDATE :
      shouldContinueGame = false;
      handleWon();
      break;

      case DEFENDER_KILLED_UPDATE :
      ouchCount++;
      handleDefenderHit(ouchCount);
      break;

      case INVADER_KILLED_UPDATE :
      handleInvaderKilled(update,bottommostInvaderRankRow,leftMostInvaderCol);
      break;

      case ERROR_UPDATE :
      //  YOUR CODE HERE to:
      //  * Move to 0,0 of 'errorWindowPtr'
      //  * Write the text at 'update+2' to 'errorWindowPtr'
      //  * Make the text visible
      wmove(errorWindowPtr, 0, 0);
      waddstr(errorWindowPtr,update+2);
      wrefresh(errorWindowPtr);


      break;

      default :
      //  YOUR CODE HERE to:
      //  * Move to 0,0 of 'errorWindowPtr'
      wmove(errorWindowPtr,0,0);

      snprintf(cText,C_STRING_MAX,
        "Unknown char w/int value %d received.",(int)update[0]
        );
      //  * Write the text in 'cText' to 'errorWindowPtr'
      //  * Make the text visible
      waddstr(errorWindowPtr,cText);
      wrefresh(errorWindowPtr);
    }

  }

  //  III.  Finished:
  return(NULL);
}


//  PURPOSE:  To end ncurses and the game.  No parameters.  No return value.
void      endGame       ()
throw()
{
  //  I.  Application validity check:

  //  II.  End ncurses:
  //  YOUR CODE HERE TO:
  //  (1) destroy the two windows created in 'startGame()'
  //  (2) turn ncurses off
 delwin(mainWindowPtr);
 delwin(errorWindowPtr);
 endwin();

  //  III.  Finished:
}



//  PURPOSE:  To play attempt to connect to the spaceInvaders server program and
//  to let a client play the game.  The hostname and port number are
//  optionally given in the command line parameters 'argc' and 'argv'.
//  Returns 'EXIT_SUCCESS' on success or 'EXIT_FAILURE' otherwise.
int     main (int argc, const char* argv[])
{
  //  I.  Parameter validity check:

  //  II.  Do spaceInvaders client:
  //  II.A.  Request user to scale window to adequate size:
  printf("Please rescale window to be at least %d rows by %d col, then press Enter:\n",
    DEFAULT_NUM_ROWS,DEFAULT_NUM_COLS);
  fgets(cText,C_STRING_MAX,stdin);

  //  II.B.  Get connection parameters:
  ServerCommInfo  serverCommInfo;

  initializeCommParams(argc,argv,serverCommInfo);

  //  II.C.  Attempt to connect and to play the game:
  try
  {

    if  ( serverCommInfo.didConnect() )
    {
      startGame(serverCommInfo);
      //  YOUR CODE HERE
      //  Do attendToUser() in one thread.
      //  Do attendToServer() in another thread.
      //  Both threads need the address of 'serverCommInfo' as an argument.
      //  The mama-thread should wait for both baby threads before doing the
      //    'endGame()'

      pthread_t attendToUser_pid;
      pthread_t attendToServer_pid;

      pthread_create(&attendToUser_pid,NULL,attendToUser,&serverCommInfo);
      pthread_create(&attendToServer_pid,NULL,attendToServer,&serverCommInfo);

      pthread_join(attendToServer_pid,NULL);
      pthread_join(attendToUser_pid,NULL);


      endGame();
    }

  }
  catch  (const char* errMsgPtr)
  {
    fprintf(stderr,"%s\n",errMsgPtr);
    return(EXIT_FAILURE);
  }

  //  III.  Finished:
  return(EXIT_SUCCESS);
}
