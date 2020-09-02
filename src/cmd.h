#define MAX_MSG_SIZE    30

void cmdInit(uint32_t speed);
void cmdPoll();
void cmd_display();
void cmdAdd(char *name, void (*func)(int argc, char **argv));
uint32_t cmdStr2Num(char *str, uint8_t base);

// command line structure
typedef struct _cmd_t
        {
         char *cmd;
         void (*func)(int argc, char **argv);
         struct _cmd_t *next;
        } cmd_t;

// command line message buffer and pointer
static uint8_t msg[MAX_MSG_SIZE];
static uint8_t *msg_ptr;

// linked list for command table
static cmd_t *cmd_tbl_list, *cmd_tbl;

// text strings for command prompt (stored in flash)
const char cmd_prompt[] PROGMEM = "CMD >> ";
const char cmd_unrecog[] PROGMEM = "CMD: Bad Command.";

/**************************************************************************/
/*!
    Generate the main command prompt
*/
/**************************************************************************/
void cmd_display()
{
  char buf[50];
  strcpy_P(buf, cmd_prompt);
  Serial.print(buf);
  //Serial.print("this running on core ");
  //Serial.println(xPortGetCoreID());
}

/**************************************************************************/
/*!
    Parse the command line. This function tokenizes the command input, then
    searches for the command table entry associated with the commmand. Once found,
    it will jump to the corresponding function.
*/
/**************************************************************************/
void cmd_parse(char *cmd)
{
  uint8_t argc, i = 0;
  char *argv[30];
  char buf[50];
  cmd_t *cmd_entry;
  fflush(stdout);
  // parse the command line statement and break it up into space-delimited
  // strings. the array of strings will be saved in the argv array.
  //Αν η εντολή δεν έχει μηδενικό μήκος
  if (strlen(cmd) > 0)
     {
      argv[i] = strtok(cmd, " "); //Χώρισε βάσει των κενών
      do
        {
         argv[++i] = strtok(NULL, " ");
        } while ((i < 30) && (argv[i] != NULL));
      // save off the number of arguments for the particular command.
      argc = i;
      // parse the command table for valid command. used argv[0] which is the
      // actual command name typed in at the prompt
      for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = cmd_entry->next)
          {
           if (!strcmp(argv[0], cmd_entry -> cmd))
              {
               cmd_entry -> func(argc, argv);
               cmd_display();
               return;
              }
          }
      // command not recognized. print message and re-generate prompt.
      strcpy_P(buf, cmd_unrecog);
      Serial.println(buf);
     }
  cmd_display(); //Εμφάνισε prompt
}

/**************************************************************************/
/*!
    This function processes the individual characters typed into the command
    prompt. It saves them off into the message buffer unless its a "backspace"
    or "enter" key.
*/
/**************************************************************************/
void cmd_handler()
{
  char c = Serial.read();
  switch (c)
         {
          case '\r':
            // terminate the msg and reset the msg ptr. then send
            // it to the handler for processing.
            *msg_ptr = '\0';
            Serial.print("\r\n");
            cmd_parse((char *)msg);
            msg_ptr = msg;
            break;
          case '\b':
          case 127:
            // backspace
            Serial.print(c);
            if (msg_ptr > msg)
               {
                msg_ptr--;
               }
            break;
          default:
            // normal character entered. add it to the buffer
            Serial.print(c);
            *msg_ptr++ = c;
            break;
         }
}

/**************************************************************************/
/*!
    This function should be set inside the main loop. It needs to be called
    constantly to check if there is any available input at the command prompt.
*/
/**************************************************************************/
void cmdPoll()
{
  while (Serial.available())
        {
         cmd_handler();
        }
}

/**************************************************************************/
/*!
    Initialize the command line interface. This sets the terminal speed and
    and initializes things.
*/
/**************************************************************************/
void cmdInit(uint32_t speed)
{
  // init the msg ptr
  msg_ptr = msg;
  // init the command table
  cmd_tbl_list = NULL;
  // set the serial speed
  Serial.begin(speed);
}

/**************************************************************************/
/*!
    Add a command to the command table. The commands should be added in
    at the setup() portion of the sketch.
*/
/**************************************************************************/
void cmdAdd(char *name, void (*func)(int argc, char **argv))
{
  // alloc memory for command struct
  cmd_tbl = (cmd_t *)malloc(sizeof(cmd_t));

  // alloc memory for command name
  char *cmd_name = (char *)malloc(strlen(name)+1);

  // copy command name
  strcpy(cmd_name, name);

  // terminate the command name
  cmd_name[strlen(name)] = '\0';

  // fill out structure
  cmd_tbl->cmd = cmd_name;
  cmd_tbl->func = func;
  cmd_tbl->next = cmd_tbl_list;
  cmd_tbl_list = cmd_tbl;
}

/**************************************************************************/
/*!
    Convert a string to a number. The base must be specified, ie: "32" is a
    different value in base 10 (decimal) and base 16 (hexadecimal).
*/
/**************************************************************************/
uint32_t cmdStr2Num(char *str, uint8_t base)
{
  return strtol(str, NULL, base);
}
