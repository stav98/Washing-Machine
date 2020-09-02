void help(int, char**);
void print_version(int, char**);
void print_pid(int, char**);
void set_pid(int, char**);
void print_speed(int, char**);
void set_speed(int, char**);
void tmotor_stop(int, char**);
void tmotor_left(int, char**);
void tmotor_right(int, char**);

void add_commands()
{
 cmdAdd("?", help);
 cmdAdd("version", print_version);
 cmdAdd("getpid", print_pid);
 cmdAdd("setpid", set_pid);
 cmdAdd("getspeed", print_speed);
 cmdAdd("setspeed", set_speed);
 cmdAdd("motorstop", tmotor_stop);
 cmdAdd("motorleft", tmotor_left);
 cmdAdd("motorright", tmotor_right);
}

void help(int arg_cnt, char **args)
{
 cmd_t *cmd_entry;
 for (cmd_entry = cmd_tbl; cmd_entry != NULL; cmd_entry = cmd_entry->next)
     {
      Serial.println(cmd_entry -> cmd);     
     }
}

void print_version(int arg_cnt, char **args)
{
 Serial.println(VERSION);
}

//Εμφανίζει συντελεστές PID
void print_pid(int arg_cnt, char **args)
{
 char data[50];
 sprintf(data, "Kp=%f, Ki=%f, Kd=%f", Kp, Ki, Kd);
 Serial.println(data);
}

//Αλλάζει τους συντελεστές PID
void set_pid(int arg_cnt, char **args)
{
 if (arg_cnt > 3)
    {
     Kp = atof(args[1]);
     Ki = atof(args[2]);
     Kd = atof(args[3]);
     myPID.SetTunings(Kp, Ki, Kd);
    }
 else
     Serial.println("Too few arguments");
}

//Εμφανίζει τις ζητούμενες στροφές του κινητήρα
void print_speed(int arg_cnt, char **args)
{
 Serial.print("Current Speed: ");
 Serial.println(Setpoint);
}

//Αλλάζει τις ζητούμενες στροφές του κινητήρα
void set_speed(int arg_cnt, char **args)
{
 if (arg_cnt > 1)
    {
     //Setpoint = atoi(args[1]);
     target_rpm = atoi(args[1]);
     start_flag = true;
    }
 else
     Serial.println("Too few arguments");
}

void tmotor_stop(int arg_cnt, char **args)
{
 motorSTOP();
 Serial.println("OK");
}

void tmotor_left(int arg_cnt, char **args)
{
 motorREV();
 Serial.println("OK");
}

void tmotor_right(int arg_cnt, char **args)
{
 motorFWD();
 Serial.println("OK");
}
