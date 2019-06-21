/*
  Autor     : Jorge Lucas Vicilli Jabczenski

  GitHub    : https://github.com/JorgeJabczenski
  Facebook  : https://www.facebook.com/taporralucas
  Instagram : https://www.instagram.com/molocolotov/

  Legenda : 

  hh = hora
  mm = minuto
  ss = segundo
  x  = número do alarme
  i  = intervalo de tempo entre as doses (em horas)

  Comandos :

  Configurar um novo alarme       - set X hh:mm:ss
  Apagar um alarme específico     - clo X
  Apagar todos os alarmes         - cla
  Mostrar os horários dos alarmes - alm
  Mostar ajuda                    - hlp
  Mostrar o horário               - hor
  Ajustar a hora do aparelho      - ajh hh:ss
  Configurar vários alarmes       - str hh:mm i

*/

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// RTC
#include <DS1307.h>
DS1307 rtc(A4, A5);

// MOTOR DE PASSO
#include <Stepper.h>
const int stepsPerRevolution = 500;
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);

// DISPLAY LCD I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 16, 2);

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=


// LUGAR PARA ARMAZENAR OS HORÁRIOS
char* horarios []  = {
  "@#:@%:%#",
  "@@:$#:@$",
  "@%:&*:**",
  "@#:##:##",
  "@$:#$:#$",
  "@#:$#:$#",
  "@$:@$:@$",
  "@@:$@:$@",
  "@&:&&:&&",
  "@$:$$:$$"
};

#define bot            2 // BOTÃO DE USO GERAL (NAO USADO)
#define ajustarPasso  22 // BOTÃO PARA AJUSTAR A POSIÇÃO DO MOTOR DE PASSO
#define rele          50 // RELE PARA ACENDER A LÂMPADA
#define tampa         51 // QUANDO ACABAR O REMEDIO, CONFERE A ABERTURA DA TAMPA PARA REPOSIÇÃO
#define LED           52 // LED DE AVISO GERAL
#define buzz          53 // BUZZER PARA ALARME

String hora;
String ultimaRetiradaDoCopo;
String proximoHorario = "--:--:--";
char horaParaVerificar[9];       
byte remedio = 0;                      // QUANTIDADE DE REMEDIOS JÁ TOMADOS

bool flagHora = false;

int timeReset = 1000;
unsigned long tR = millis();
unsigned long tL = millis();

// CHARS PARA O DISPLAY
uint8_t bell     [8] = {0x4, 0xe , 0xe , 0xe , 0x1f, 0x0 , 0x4};
uint8_t clock    [8] = {0x0, 0xe , 0x15, 0x17, 0x11, 0xe , 0x0};
//uint8_t note     [8] = {0x2, 0x3 , 0x2 , 0xe , 0x1e, 0xc , 0x0};
//uint8_t heart    [8] = {0x0, 0xa , 0x1f, 0x1f, 0xe , 0x4 , 0x0};
//uint8_t duck     [8] = {0x0, 0xc , 0x1d, 0xf , 0xf , 0x6 , 0x0};
//uint8_t check    [8] = {0x0, 0x1 , 0x3 , 0x16, 0x1c, 0x8 , 0x0};
//uint8_t cross    [8] = {0x0, 0x1b, 0xe , 0x4 , 0xe , 0x1b, 0x0};
//uint8_t retarrow [8] = {0x1, 0x1 , 0x5 , 0x9 , 0x1f, 0x8 , 0x4};

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

// Caso o horário estiver certo, a função retorna 1, caso não, zero
int verificaHora(char *hora)
{

  if (hora[0] > '2' || hora[1] > '9' || hora[3] > '5' || hora[4] > '9' || hora[6] > '5' || hora[7] > '9' || hora[2] != ':' || hora[5] != ':')  {
    return 0;
  } else if (!(isDigit(hora[0])) || !(isDigit(hora[1])) || !(isDigit(hora[3])) || !(isDigit(hora[4])) || !(isDigit(hora[6])) || !(isDigit(hora[7]))) {
    return 0;
  } else {
    return 1;
  }
}

// EMITIR O ALARME SONORO
void bleep(int v, int del)
{
  for (int i = 0; i < v; i++) {
    digitalWrite(buzz, HIGH);
    delay(100);
    digitalWrite(buzz, LOW);
    delay(del);
  }
}

// CONFIGURAR UM NOVO ALARME
int setarAlarme(String horario)
{
  int hor = horario[4] - '0';
  if (isdigit(horario[4])) {

    for (int i = 6; i < 14; i++) {
      horarios[hor][i - 6] = horario[i];
    }
  } else {
    Serial.println("\nNo lugar do 'X' bote um numero de 0 a 9\n");
    Serial1.println("\nNo lugar do 'X' bote um numero de 0 a 9\n");
    return 0;
  }

  /*horarios[hor][5] = ':';
    horarios[hor][6] = '0';
    horarios[hor][7] = '0';*/
  horarios[hor][8] = '\0';

  printHorarios();
  showTimeOnSerial();
  return 1;
}

// LIMPAR TODOS OS ALARMES
void clearAll(void)
{
  proximoHorario = "--:--:--";
  Serial.println ("Limpando todos os horarios");
  Serial1.println ("Limpando todos os horarios");
  for (int i = 0; i < 10; i++)
    horarios[i][0] = '@';

  printHorarios();
}

// LIMPAR UM ALARME ESPECÍFICO
void clearOne(String horario)
{
  int hor = horario[4] - '0';
  horarios[hor][0] = '@';
  proximoHorario = "--:--:--";
  printHorarios();
}

// MOSTRA OS HORARIOS DOS REMEDIOS NO MONITOR SERIAL E NO BLUETOOTH
void printHorarios(void)
{
  Serial.println("");
  for (int i = 0; i < 10; i++) {
    Serial.print(i);
    Serial.print(" : ");
    if (horarios[i][0] == '@')
    {
      Serial.println("--:--:--");
    } else {
      Serial.println(horarios[i]);
    }
  }
  Serial.println("");

  Serial1.println("");
  for (int i = 0; i < 10; i++) {
    Serial1.print(i);
    Serial1.print(" : ");
    if (horarios[i][0] == '@')
    {
      Serial1.println("--:--:--");
    } else {
      Serial1.println(horarios[i]);
    }
  }
  Serial1.println("");
}

// MOSTRA A HORA NO MONITOR SERIAL E NO BLUETOOTH
void showTimeOnSerial(void)
{

  hora = rtc.getTimeStr();

  //Mostra as informações no Serial Monitor
  Serial.print("Hora : ");
  Serial.print(hora);
  Serial.print(" ");
  Serial.print("Data : ");
  Serial.print(rtc.getDateStr(FORMAT_SHORT));
  Serial.println(" ");
  //Serial.println(rtc.getDOWStr(FORMAT_SHORT));

  Serial1.print("Hora : ");
  Serial1.print(hora);
  Serial1.print(" ");
  Serial1.print("Data : ");
  Serial1.print(rtc.getDateStr(FORMAT_SHORT));
  Serial1.println(" ");
  //Serial1.println(rtc.getDOWStr(FORMAT_SHORT));

}

// MOSTRA OS COMANDOS DE AJUDA NO MONITOR SERIAL E NO BLUETOOTH
void printarAjuda()
{
  
  Serial.println("\nPara configurar o horario de um alarme, digite 'set x hh:mm:ss', onde x eh o numero do seu alarme (0-9)");
  Serial.println("Exemplo --> set 1 13:45:00\n");

  Serial.println("Para limpar todos os alarmes, digite 'cla'\n");

  Serial.println("Para limpar um alarme especifico, digite 'clo x', onde x eh o alarme que voce quer limpar (0-9)");
  Serial.println("Exemplo --> clo 8\n");

  Serial.println("Para saber as horas, digite 'hor'\n");

  Serial.println("Para saber os horarios dos seus alarmes, digite 'alm'\n");

  Serial.println("Para ajustar o horario do seu aparelho, digite 'ajh hh:mm'");
  Serial.println("Exemplo --> ajh 09:15\n");

  Serial1.println("\nPara configurar o horario de um alarme, digite 'set x hh:mm:ss', onde x eh o numero do seu alarme (0-9)");
  Serial1.println("Exemplo --> set 1 13:45:00\n");

  Serial1.println("Para limpar todos os alarmes, digite 'cla'\n");

  Serial1.println("Para limpar um alarme especifico, digite 'clo x', onde x eh o alarme que voce quer limpar (0-9)");
  Serial1.println("Exemplo --> clo 8\n");

  Serial1.println("Para saber as horas, digite 'hor'\n");

  Serial1.println("Para saber os horarios dos seus alarmes, digite 'alm'\n");

  Serial1.println("Para ajustar o horario do seu aparelho, digite 'ajh hh:mm'");
  Serial1.println("Exemplo --> ajh 09:15\n");

}

// CONFIGURAR ALARMES COM UM INTERVALO DE TEMPO ESPECÍFICO
void repeteHorarios(String horario)
{
  char h[8];
  for (int i = 0; i < 5; i++)
  {
    h[i] = horario[i + 4];
  }
  h[5] = ':';
  h[6] = '0';
  h[7] = '0';

  if (!verificaHora(h)) {
    Serial.println ("Digite um horario valido!");
    Serial1.println("Digite um horario valido!");
  } else {

    int n = 0;
    char temporario[6];
    temporario [5] = '\0';

    for (int i = 0; i < 5; i++)
    {
      if (i != 2)
      {
        n += (horario[i + 4] - '0') % 10;
        if (i != 4)
          n *= 10;
      }
    }

    int intervalo = horario[10] - '0';

    if (horario[11] != '\0')
    {
      intervalo *= 10;
      intervalo += horario[11] - '0';
    }


    int quantidade = 24 / intervalo;

    for (int i = 0; i < quantidade; i++)
    {
      n += (intervalo * 100);
      if (n >= 2400)
        n -= 2400;
      int nn = n;

      for (int j = 5; j > 0; j--)
      {
        if (j != 3)
        {
          temporario[j - 1] = (nn % 10) + '0';
          nn /= 10;
        } else {
          temporario[j - 1] = ':';
        }
      }

      //Serial.print  ("Temporario --> ");
      //Serial.println(temporario);
      strcpy(horarios[i], temporario);
      horarios[i][5] = ':';
      horarios[i][6] = '0';
      horarios[i][7] = '0';
    }
    Serial.println ("Novos Horários Configurados!\n");
    Serial1.println("Novos Horários Configurados!\n");
    printHorarios();
    showTimeOnSerial();
  }
}

// EXECUTA O MOVIMENTO DO MOTOR DE PASSO
void movePasso()
{
  myStepper.step(-2048 / 8);
  delay(1000);

  remedio++;
}

// CALCULA QUAL É O PROXIMO HORARIO DE ALARME PARA MOSTRAR NO LCD
void calcProxHorario()
{
  char* alarmes [10]  = {
    "@#:@%:%#",
    "@@:$#:@$",
    "@%:&*:**",
    "@#:##:##",
    "@$:#$:#$",
    "@#:$#:$#",
    "@$:@$:@$",
    "@@:$@:$@",
    "@&:&&:&&",
    "@$:$$:$$"
  };
  int n1 = 0, n2 = 0, pht = 0;
  int cmp [10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int increment = 500;
  String h1 = rtc.getTimeStr();
  //char h2[4] = '\0\0\0\0';
  String temporario;
  temporario[8] = '\0';
  bool pass = false;


  for (int i = 0; i < 10; i++)
  {
    if (horarios[i][0] != '@')
    {
      //Serial.print("PASSEI ");
      //Serial.println(i);
      cmp [i] = 0;
      temporario = horarios[i];
      for (int j = 0; j < 5; j++)
      {
        if (j != 2)
        {
          cmp[i] += (temporario[j] - '0');
          if (j != 4)
          {
            cmp[i] *= 10;
          }
        }
      }

      //Serial.print  ("cmp [i] : ");
      //Serial.println(cmp[i]);
    }
  }

  for (int j = 0; j < 5; j++)
  {
    if (j != 2)
    {
      n2 += (h1[j] - '0');
      if (j != 4)
      {
        n2 *= 10;
      }
    }
  }

  for (int i = 0; i < 10; i++)
  {
    for (int j = 1; j < 10; j++)
    {
      if (cmp[j - 1] > cmp[j])
      {
        int t = cmp[j - 1];
        cmp[j - 1] = cmp[j];
        cmp[j] = t;
      }
    }
  }
  /*
    for (int i = 0; i < 10; i++)
    {
      Serial.println(cmp[i]);
    }

    Serial.print  ("n2:  ");
    Serial.println(n2);
  */
  while (!pass)
  {
    for (int i = 0; i < 10; i++)
    {
      if (cmp[i] > -1)
      {
        if (n2 == cmp[i])
        {
          //Serial.println("ALO");
          pht = cmp[i];
          pass = true;
          break;
        } else {
          //Serial.println("SOMA");
          //Serial.println(n2);
        }
        /*Serial.print  ("horario :");
          Serial.print  (n2);
          Serial.print  ("   cmp[");
          Serial.print  (i);
          Serial.print("]  :");
          Serial.println(cmp[i]);*/
      }
      //Serial.println(cmp[i]);
      //delay(1);

    }
    if (n2 == 2400) n2 -= 2400;
    n2 += 1;
  }

  //Serial.print("\npht : ");
  //Serial.println(pht);

  //String proximoHorario = "fff";

  for (int j = 5; j > 0; j--)
  {
    if (j != 3)
    {
      proximoHorario[j - 1] = (pht % 10) + '0';
      pht /= 10;
    } else {
      proximoHorario[j - 1] = ':';
    }
  }

  proximoHorario[5] = ':';
  proximoHorario[6] = '0';
  proximoHorario[7] = '0';

  Serial.print("Prox hor : ");
  Serial.println(proximoHorario);

}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void setup()
{

  // INICIA A COMUNICAÇÃO SERIAL
  Serial.begin (9600);
  Serial1.begin(9600);

  // INICIA O MOTOR DE PASSO
  myStepper.setSpeed(60);

  //Aciona o relogio
  rtc.halt(false);
  //Definicoes do pino SQW/Out
  rtc.setSQWRate(SQW_RATE_1);
  rtc.enableSQW(true);

  // Alterar a data/hora DO RTC
  /*
     rtc.setDOW(MONDAY);        //Define o dia da semana
     rtc.setTime(02, 23, 00);     //Define o horario
     rtc.setDate(19, 11, 2018);    //Define o dia, mes e ano
  */

  pinMode(buzz, OUTPUT);
  digitalWrite(buzz, LOW);
  pinMode(rele, OUTPUT);
  digitalWrite(rele, HIGH);

  pinMode(bot         , INPUT_PULLUP);   // Botão de uso geral
  pinMode(tampa       , INPUT_PULLUP); 
  pinMode(ajustarPasso, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();

  lcd.createChar(0, bell);
  //lcd.createChar(1, note);
  lcd.createChar(2, clock);
  //lcd.createChar(3, heart);
  //lcd.createChar(4, duck);
  //lcd.createChar(5, check);
  //lcd.createChar(6, cross);
  //lcd.createChar(7, retarrow);

  Serial.println("Digite 'hlp' para acessar os comandos de ajuda\n");
  Serial.println("Caso esteja no computador, certifique-se que seu monitor serial esta no modo 'Nenhum-final-de-linha'\n\n");
  Serial1.println("Digite 'hlp' para acessar os comandos de ajuda\n");

}


void loop()
{

  //Serial.println(analogRead(A0));

  // A cada segundo, verifica se o copo está no stand e outputa a hora no serial
  if ((millis() - tR) > timeReset)
  {
    if ((!flagHora) && analogRead(A0) < 600)
    {
      lcd.clear();
      lcd.home();
      lcd.print("INSIRA O COPO");
      bleep(1, 1000);
    } else {
      lcd.clear();
      lcd.home();
      lcd.printByte(2);
      lcd.print(" - ");
      lcd.print(rtc.getTimeStr());
      lcd.setCursor(0, 1);
      lcd.printByte(0);
      lcd.print(" - ");
      lcd.print(proximoHorario);
    }
    tR = millis();
  }

  // COMPARA O HORARIO ATUAL COM O HORARIO DOS REMEDIOS
  for (int i = 0; i < 10; i++) {
    if (hora == horarios[i]) {
      flagHora = true;
      break;
    }
  }

  // Caso seja a hora, solta o remédio e liga o alarme
  if (flagHora) {
    digitalWrite(rele, LOW);
    movePasso();

    while (analogRead(A0) > 800) {
      // ESPERA ATÉ A RETIRADA DO COPO
      bleep(3, 100);
      delay(1000);
    }
    ultimaRetiradaDoCopo = rtc.getTimeStr();
    Serial.print("O remédio foi tomado as: ");
    Serial.println(ultimaRetiradaDoCopo);
        
    // CASO TENHA ACABADO OS REMEDIOS, ESPERA A REPOSIÇÃO DOS MESMOS
    if (remedio == 8)
    {
      remedio = 0;
      digitalWrite(rele, LOW);
      lcd.clear();
      lcd.home();
      lcd.print("Abastecer !!");

      for (int i = 0; i < 10; i++){
        bleep(5, 200);
        delay(500);
      }

      digitalWrite(rele, HIGH);
      lcd.clear();
      delay(200);
    }

    digitalWrite(rele, HIGH);
    flagHora = false;
  }

  // ESPERA OS COMANDOS DO SERIAL
  if (Serial.available()) {
    String palavra = Serial.readString();
    char cmd[4];
    cmd[0] = palavra[0];
    cmd[1] = palavra[1];
    cmd[2] = palavra[2];
    cmd[3] = '\0';

    // seta uma hora de alarme
    if (!strcmp(cmd, "set")) {

      for (int c = 6; c < 14; c++)
        horaParaVerificar[c - 6] = palavra[c];

      horaParaVerificar[8] = '\0';

      if (verificaHora(horaParaVerificar)) {
        if (setarAlarme(palavra)) {
          Serial.println ("Novo alarme configurado!");
          calcProxHorario();
        }
      } else {
        Serial.println ("Erro ao tentar configurar novo alarme");
      }
    }

    // limpa um horario em especifico
    if (!strcmp(cmd, "clo")) {
      clearOne(palavra);
      calcProxHorario();
    }

    // repete horarios
    if (!strcmp(cmd, "str")) {
      repeteHorarios(palavra);
      calcProxHorario();
    }

    // limpa todos os horarios
    if (!strcmp(cmd, "cla")) {
      clearAll();
    }

    // mostra os horarios dos alarmes
    if (!strcmp(cmd, "alm")) {
      printHorarios();
    }

    // mostra os comandos de ajuda
    if (!strcmp(cmd, "hlp")) {
      printarAjuda();
    }

    // mostra a hora
    if (!strcmp(cmd, "hor")) {
      showTimeOnSerial();
    }

    // ajusta a hora do aparelho
    if (!strcmp(cmd, "ajh")) {
      int hora = 0, minuto = 0;
      char hor[3], min[3];
      hor[0] = palavra[4];
      hor[1] = palavra[5];
      hor[2] = '\0';
      min[0] = palavra[7];
      min[1] = palavra[8];
      min[2] = '\0';

      hora = hor[0] - 48;
      hora *= 10;
      hora += hor[1] - 48;

      minuto = min[0] - 48;
      minuto *= 10;
      minuto += min[1] - 48;

      if (hora > 23 || hora < 0 || minuto > 59 || minuto < 0) {
        Serial.println("ERRO!\nInsira um horario valido!");
      } else {
        rtc.setTime(hora, minuto, 00);
        Serial.print  ("Novo horario : ");
        Serial.println(rtc.getTimeStr());
      }

    }
  }

  // ESPERA OS COMANDOS DO BLUETOOTH
  if (Serial1.available()) {
    String palavra = Serial1.readString();
    char cmd[4];
    cmd[0] = palavra[0];
    cmd[1] = palavra[1];
    cmd[2] = palavra[2];
    cmd[3] = '\0';

    // seta uma hora de alarme
    if (!strcmp(cmd, "set")) {

      for (int c = 6; c < 14; c++)
        horaParaVerificar[c - 6] = palavra[c];

      horaParaVerificar[8] = '\0';

      if (verificaHora(horaParaVerificar)) {
        if (setarAlarme(palavra)) {
          Serial1.println ("Novo alarme configurado!");
          calcProxHorario();

        }
      } else {
        Serial1.println ("Erro ao tentar configurar novo alarme");
      }
    }

    // limpa um horario em especifico
    if (!strcmp(cmd, "clo")) {
      clearOne(palavra);
    }

    // repete horarios
    if (!strcmp(cmd, "str")) {
      repeteHorarios(palavra);
      calcProxHorario();
    }

    // limpa todos os horarios
    if (!strcmp(cmd, "cla")) {
      clearAll();
    }

    // mostra os horarios dos alarmes
    if (!strcmp(cmd, "alm")) {
      printHorarios();
    }

    // mostra os comandos de ajuda
    if (!strcmp(cmd, "hlp")) {
      printarAjuda();
    }

    // mostra a hora
    if (!strcmp(cmd, "hor")) {
      showTimeOnSerial();
    }

    // ajusta a hora do aparelho
    if (!strcmp(cmd, "ajh")) {
      int hora = 0, minuto = 0;
      char hor[3], min[3];
      hor[0] = palavra[4];
      hor[1] = palavra[5];
      hor[2] = '\0';
      min[0] = palavra[7];
      min[1] = palavra[8];
      min[2] = '\0';

      hora = hor[0] - 48;
      hora *= 10;
      hora += hor[1] - 48;

      minuto = min[0] - 48;
      minuto *= 10;
      minuto += min[1] - 48;

      if (hora > 23 || hora < 0 || minuto > 59 || minuto < 0) {
        Serial1.println("ERRO!\nInsira um horario valido!");
      } else {
        rtc.setTime(hora, minuto, 00);
        Serial1.print  ("Novo horario : ");
        Serial1.println(rtc.getTimeStr());
      }

    }
  }

  // Ajustar Motor de Passo
  if (digitalRead(ajustarPasso) == LOW)
  {
    myStepper.step(-10);
    delay(100);
  }

  hora = rtc.getTimeStr();
}
