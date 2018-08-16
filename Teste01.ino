
//====================RELÓGIO===================================== 
//Carrega a biblioteca do RTC DS1307
#include <DS1307.h>
// RTC DS1307 ligado as portas A4 e A5
DS1307 rtc(A4, A5);
//================================================================



void setup()
{
  //Aciona o relogio
  rtc.halt(false);

/*
    rtc.setDOW(THURSDAY);      //Define o dia da semana
    rtc.setTime(1, 18, 0);     //Define o horario
    rtc.setDate(16, 8, 2018);   //Define o dia, mes e ano
*/

  //Definicoes do pino SQW/Out
  rtc.setSQWRate(SQW_RATE_1);
  rtc.enableSQW(true);

  Serial.begin(9600);
  pinMode(12, OUTPUT);

}

String horaRemedio1 = "01:52:00";
String horaRemedio2 = "01:53:00";
String horaRemedio3 = "01:54:00";

bool flagCopo = true;
bool flagHora = false;

int presencaCopo;

void loop()
{
  String hora = rtc.getTimeStr();

  //Mostra as informações no Serial Monitor
    Serial.print("Hora : ");
    Serial.print(hora);
    Serial.print(" ");
    Serial.print("Data : ");
    Serial.print(rtc.getDateStr(FORMAT_SHORT));
    Serial.print(" ");
    Serial.println(rtc.getDOWStr(FORMAT_SHORT));


  if (hora == horaRemedio1 || hora == horaRemedio2 ||hora == horaRemedio3) {
    digitalWrite(12, HIGH);
    flagHora = true;
  }

  if (flagHora) {
    while (analogRead(A1) < 100) {

    }
    flagHora = false;
    digitalWrite(12, LOW);
  }

  
  delay(500);

}
