// Inclusión de librerias
#include <LiquidCrystal.h> 
LiquidCrystal lcd(53, 51,50, 49, 48, 47);

// Variables globales
volatile bool bandRX = false; //Bandera de recepción de señal UART
volatile bool bandTimer1 = false; //Bandera de id del timer 1 16 bits
volatile bool bandTimer0 = false; //Bandera de id del timer 0 8 bits
volatile bool bandADC = false; //Bandera de lectura del ADC 0
volatile bool bandLCD = false; //Bandera de LCD
volatile bool bandTitileo = false; //Bandera de titileo
volatile bool ajuste = false; //Bandera ajuste 
volatile bool bandButton_Sel = 0; //Bandera de id de botón de selección
volatile bool bandButton_Sel_2 = 0; //Bandera de id de botón de flecha
volatile bool bandButton_Ac = 0; //Bandera de id de botón de aceptar
volatile bool bandButton_silen = 0; //Bandera de id de botón de silencio
volatile bool bandCursor = 0; //Bandera de identificación del cursor 
unsigned int cambioLCD = 0; //Bandera de identificación del cambio de switch
volatile bool bandPresion = false; //Bandera de identificación cuando hacer la conversión ADC de la presión
volatile bool bandMaxMin = false; //Bandera detecta máximos y mínimos
volatile bool band_12H = false; //Bandera detecta AM o PM
volatile bool band_AlarmaLCD = false; //Bandera para imprimir Alarma una sola vez

unsigned int ContDuty = 31250; // Valor de conteo asociado al Duty de PWM, por defecto para el 50%
unsigned int ContTimer0 = 0; // Contador asociado a num interrupciones del timer de 8 bits
unsigned int v; //Valor voltaje
unsigned int ValADC; //Valor ADC
unsigned int T = 0; //Valor temperatura
unsigned int P = 0; //Valor Presión
unsigned int PIP = 0; //Valor PIP
unsigned int PEEP = 0; //Valor PEEP
unsigned int PIP_F = 0; //Valor PIP final
unsigned int PEEP_F = 0; //Valor PEEP final
unsigned int FR = 0; //Valor frecuencia de respiración
unsigned int TI = 0; //Valor tiempo de inhalación
unsigned int TE = 0; //Valor tiempo de exhalación
unsigned int Al_PIP = 30; //Valor de alarma PIP
unsigned int Al_FR = 30; //Valor de alarma FR
unsigned int CLK_Hor = 12; //Valor valor reloj hora
unsigned int CLK_Min = 00; //Valor valor reloj minutos
unsigned int CLK_S = 00; //Valor valor reloj segundos
String F12 = "am"; //Valor am - pm reloj
unsigned int ContPeriodo = 0; //Contador período
unsigned int Periodo = 0; // Período de la señal de presión 
unsigned int contTI = 0; // Tiempo de inspriración
unsigned int IE = 0; //Relación TI:TE
unsigned int contDescon = 0; //Contador alarma Desconexion


//void serialEvent() { //Recepción de datos por serial
  //bandRX = false; // Cambio del estado de la bandera de recepción
//}
ISR (TIMER1_COMPA_vect) { //Rutina de atención de interrución PWM
  OCR1B = ContDuty;       //Actualiza el valor de conteo asociado al Duty
  bandTimer1 = true;
}
ISR (TIMER0_COMPA_vect) { //Rutina de atención de interrrupción Contador
  bandPresion = true;
  ContTimer0 ++;                  //Aumenta el contador del timer de 8 bits
  if (ContTimer0%500 == 0) {
    bandTitileo = !(bandTitileo); //Se configura el titileo
  }
  if (ContTimer0 == 1000){        //ContTimer0 igual a 1s
    ContTimer0 = 0;               //Reinicia el contador del timer 0 
    CLK_S ++;                     //Aumenta el contador de segundos 
    if (CLK_S > 59){              //Se restringuen los segundos a 59
      CLK_S = 0;                  //Reinicia el contador de segundos
      CLK_Min ++;                 //Aumenta el contador de min
      if (CLK_Min > 59) {         // Se restringuen los minutos a 59
        CLK_Min = 0;
        CLK_Hor ++;
        if (CLK_Hor == 12){             //Contador de hora igual a 12 para formato 12 horas
          band_12H = true;              //Activa la bandera de 12 horas
          if (band_12H == false){       //Mientras la bandera de 12 horas sea falsa, el reloj esta en "am"
            F12 = "am";
          }
          else {                        //Bandera 12 horas verdadera, reloj en pm
            F12 = "pm";
          }
        }
      }
      if (CLK_Hor > 12){                //Contador hora mayor que 12, comienza en 1
       CLK_Hor = 1;
      }
    }
  }
}
void ButtonPress_Sel () { //Si se presiona el botón
  bandButton_Sel = true; //Cambia el estado de la bandera de selección
}
void ButtonPress_Sel_2 () { //Si se presiona el botón de selección 2
  bandButton_Sel_2 = true; //Cambia el estado de la bandera de selección 2
}
void ButtonPress_Ac () { //Si se presiona el botón
  bandButton_Ac = true; //Cambia el estado de la bandera de aceptar
}
void ButtonPress_Silen () { //Si se presiona el botón
  bandButton_silen = true; //Cambia el estado de la bandera de boton de silencio
}

void setup() {
  // configuración LCD
  pinMode (52, OUTPUT);
  digitalWrite (52, LOW);
  lcd.begin(20,4);
  lcd.clear();
  
  // configuración UART
  Serial.begin(115200, SERIAL_8N1);
  while (!(Serial)) {};
  Serial.println ("Config exitosa Señal ADC");
  
  //Configuración del PWM
  //1. Asociación del pin OC1B asociado en modo directo
  bitSet(TCCR1A, COM1B1);
  bitClear(TCCR1A, COM1B0);
  //2. Asociación del pin OC1B asociado en modo directo
  bitSet(TCCR1A, WGM11);
  bitSet(TCCR1A, WGM10);
  bitSet(TCCR1B, WGM13);
  bitSet(TCCR1B, WGM12);
  //3. Selección entrada de reloj, prescaler y máximo conteo
  bitSet(TCCR1B, CS12);   //(clk_I/O)/256 = 16MHz
  bitClear(TCCR1B, CS11);
  bitClear(TCCR1B, CS10);
  OCR1A = 62499;      //Máximo conteo para frecuencia de 1Hz
  //4. Duty
  OCR1B = ContDuty; //Valor de conteo para el Duty
  //5. Habilitación de la interrupción
  bitSet(TIMSK1, OCIE1A);
  //6. Definición de pines de salida
  pinMode(12, OUTPUT); //Pin de salida para el PWM

  //Configuración Timer - Contador
  //1. Modo de funcionamiento y sin pines asociados
  bitClear(TCCR0B, WGM02);
  bitSet(TCCR0A, WGM01);
  bitClear(TCCR0A, WGM00);
  //3. Selección entrada de reloj, prescaler y máximo conteo
  bitClear(TCCR0B, CS02);   //(clk_I/O)/64 = 16MHz
  bitSet(TCCR0B, CS01);
  bitSet(TCCR0B, CS00);
  OCR0A = 249;      //Máximo conteo para frecuencia de 1kHz 249
  //2.Habilitación de la interrupción
  bitSet(TIMSK0, OCIE0A);
  
  // Configuración pines digitales
  pinMode (4, OUTPUT);
  digitalWrite (4, LOW);
  pinMode (13, OUTPUT);
  digitalWrite (13, LOW);
  pinMode (11, INPUT);
  pinMode (8, INPUT);
  pinMode (21, INPUT);
  digitalWrite (21, HIGH);
  pinMode (20, INPUT);
  digitalWrite (20, HIGH);
  pinMode (19, INPUT);
  digitalWrite(19, HIGH);
  pinMode (18, INPUT);
  digitalWrite(18, HIGH);
  
  //Configuración interrupciones de botones
  attachInterrupt (digitalPinToInterrupt(19), ButtonPress_Sel, RISING);
  attachInterrupt (digitalPinToInterrupt(20), ButtonPress_Sel_2, RISING);
  attachInterrupt (digitalPinToInterrupt(21), ButtonPress_Ac, RISING);
  attachInterrupt (digitalPinToInterrupt(18), ButtonPress_Silen, RISING);
}

void loop() {
  if(bandTimer1 == true) {                            //Interrupción del timer de 16 bits
    bandTimer1 = false;                               //Cambia el estado de la bandera
    ValADC = analogRead(A0);                          // ADC temperatura
    if ((ValADC >= 0) && (ValADC < 306)){             //valor de ADC en el rango de T entre 0 - 30 grados
      ContDuty = 9375;                                // Duty al 15%
    }
    else if ((ValADC >= 306) && (ValADC < 511)){       //valor de ADC en el rango de T entre 30 - 50 grados
      ContDuty = 18750;                               // Duty al 30%
    }
    else if ((ValADC >= 511) && (ValADC < 716)){      //valor de ADC en el rango de T entre 50 - 70 grados
      ContDuty = 37499;                               // Duty al 60%
    }
    else if ((ValADC >= 716) && (ValADC < 1023)) {    //valor de ADC en el rango de T entre 70 - 100 grados
      ContDuty = 62499;                               // Duty al 100%
    }
  }
  if ((digitalRead (11) == LOW and digitalRead (8) == LOW) or (digitalRead (11) == LOW and digitalRead (8) == HIGH)) { //Ventana de Ajuste
    if (cambioLCD == 0 or cambioLCD == 2 or cambioLCD == 3) {
      cambioLCD = 1;
      lcd.clear(); //Inicialización de LCD limpia
    }
    lcd.setCursor (3,1);      //Impresión de ventana de ajuste con sus respectivas características 
    lcd.print ("PIP:");
    lcd.setCursor (3,2);
    lcd.print ("FR:");    
    if (ajuste == false){
      lcd.setCursor(7,2);
      lcd.print(Al_FR);
      lcd.setCursor(7,1);
      lcd.print(Al_PIP);
      if (bandCursor == false) {
        lcd.setCursor(0,1);
        lcd.print("->");      //Congiguración de flecha arriba en LCD, con su espacio correspondiente
      }
     else {
        lcd.setCursor(0,2);
        lcd.print("->");     //Congiguración de flecha abajo en LCD, con su espacio correspondiente
      }
    }
    else {
      if (bandCursor == false) {     //Configuración de selección de variable a modificar por medio de * para FR
        lcd.setCursor(0,1);
        lcd.print("* ");
        lcd.setCursor(7,2);
        lcd.print(Al_FR);
        lcd.setCursor(7,1);
        if (bandTitileo == false) {
          lcd.print("    ");    
        }
        else {
          lcd.print(Al_PIP);   
        }
      }
     else {                         //Configuración de selección de variable a modificar por medio de * para PIP
        lcd.setCursor(0,2);
        lcd.print("* ");
        lcd.setCursor(7,1);
        lcd.print(Al_PIP);
        lcd.setCursor(7,2);
        if (bandTitileo == false) {
          lcd.print("    "); 
        }
        else {
          lcd.print(Al_FR);
        }
      }
    }
  } 

   
  if (digitalRead (11) == HIGH and digitalRead (8) == LOW){   //Ventana de monitoreo y Standby
    if (cambioLCD == 0 or cambioLCD == 1 or cambioLCD == 3) {
      cambioLCD = 2;
      PIP = 0;
      PEEP = 0;
      FR = 0;
      IE=0;
      lcd.clear();
    }
    lcd.setCursor (0,1);    //Impresión LCD de ventana de monitoreo y Standby con sus respectivas características y variables
    lcd.print ("PIP:");
    lcd.print (PIP);  
    lcd.setCursor (10,1);
    lcd.print ("Peep:");
    lcd.print (PEEP);
    lcd.setCursor (0,2);
    lcd.print ("FR:");
    lcd.print (FR);
    lcd.setCursor (10,2);
    lcd.print ("I:E:");
    lcd.print("0.0:1.0");
    lcd.setCursor (0,3);
    lcd.print ("Hora:");
    lcd.print (CLK_Hor/10);  //Impresión LCD de hora en formato 12 horas
    lcd.print (CLK_Hor%10);
    lcd.print (":");
    lcd.print (CLK_Min/10);
    lcd.print (CLK_Min%10);
    lcd.print (":");
    lcd.print (CLK_S/10);
    lcd.print (CLK_S%10);
    lcd.print (F12); 
     
  }
  if (digitalRead (11) == HIGH and digitalRead (8) == HIGH){        // Ventana de Monitoreo e Inicio
    if (cambioLCD == 0 or cambioLCD == 1 or cambioLCD == 2) {
      cambioLCD = 3;
      lcd.clear();
    }
    if (bandPresion == true){
      bandPresion = false;
      ValADC = analogRead(A2);
      ContPeriodo++;
      v = ((unsigned long)ValADC * 5000) / 1023; //Convierte ADC a voltaje * 10000 (ADC =1 a 48, resolución 4.8 mV)
      P = (((unsigned long)v * 100)/499); //Convierte a presión
      if(P == 0){                                  //Alarma por desconexión
        contDescon++;
        if (contDescon == 30000){
          contDescon = 0;
          if(band_AlarmaLCD == false){
            lcd.setCursor(0,0);
            lcd.print("Alarma: desconexion");
            band_AlarmaLCD = true;
            digitalWrite(13, HIGH);
            digitalWrite(4, HIGH);
          }
        }
      }
      else{
        band_AlarmaLCD = false;
        lcd.setCursor(0,0);
        lcd.print("                    ");
        digitalWrite(13, LOW);
        digitalWrite(4, LOW);
      }
      if (bandMaxMin == false){
        if (P > PIP) {
          PIP = P;
        }
        if (P < PIP){
          PEEP = PIP;
          PIP_F = PIP;
          contTI = ContPeriodo;
          bandMaxMin = true;
        }
      }
      else {
        if (P < PEEP) {
          PEEP = P;
        }
        if (P > PEEP) {
          PIP = PEEP;
          PEEP_F = PEEP;
          Periodo = ContPeriodo/100;     //Periodo de la señal
          FR = (6000/Periodo);        //Frecuencia respiratoria por minuto de la señal 
          TI = contTI;
          TE = ContPeriodo - TI;
          if (TE > TI){
            IE = TE/TI;
          }
          else {
            IE = TI/TE;
          }
          
          lcd.clear();
          lcd.setCursor (0,1);
          lcd.print ("PIP:");
          lcd.print(PIP_F/1000);
          lcd.print((PIP_F % 1000)/100);
          lcd.print((PIP_F % 100)/10);
          lcd.print(".");
          lcd.print(PIP_F % 10);
          lcd.setCursor (10,1);
          lcd.print ("Peep:");
          lcd.print(PEEP_F/1000);
          lcd.print((PEEP_F % 1000)/100);
          lcd.print((PEEP_F % 100)/10);
          lcd.print(".");
          lcd.print(PEEP_F % 10);
          lcd.setCursor (0,2);
          lcd.print ("FR:");
          lcd.print(FR/10);
          lcd.print(".");
          lcd.print(FR % 10);
          lcd.setCursor (9,2);
          lcd.print ("I:E:");
          if (TE > TI){
            lcd.print("1.0:");
            lcd.print(IE);
            lcd.print(".");
            lcd.print(IE % 1);
          }
          else {
            lcd.print(IE);
            lcd.print(".");
            lcd.print(IE % 1);
            lcd.print(":1.0"); 
          }
          
          lcd.setCursor (0,3);
          lcd.print ("Hora:");
          lcd.print (CLK_Hor/10);
          lcd.print (CLK_Hor%10);
          lcd.print (":");
          lcd.print (CLK_Min/10);
          lcd.print (CLK_Min%10);
          lcd.print (":");
          lcd.print (CLK_S/10);
          lcd.print (CLK_S%10);
          lcd.print (F12); 
          
          ContPeriodo = 0;
          bandMaxMin = false;
        }
      }   
    }


    if (PIP_F >(Al_PIP*10)){          //Configuración de alarma PIP elevada
      if (band_AlarmaLCD == false){   //Cuando la bandera de silencio es falsa se activa el buzzer
        lcd.setCursor(0,0);       
        lcd.print("Alarma: PIP elevada");
        band_AlarmaLCD = true;
        digitalWrite(13, HIGH);
        digitalWrite(4, HIGH);
      }           
    }
    else{
      band_AlarmaLCD = false;
      lcd.setCursor(0,0);
      lcd.print("                    ");
      digitalWrite(13, LOW);
      digitalWrite(4, LOW);
    }

    if (FR > (Al_FR*10)){             //configuración alarma FR elevada
      if (band_AlarmaLCD == false){   //Cuando la bandera de silencio es falsa se activa el buzzer
        lcd.setCursor(0,0);       
        lcd.print("Alarma: FR elevada");
        band_AlarmaLCD = true;
        digitalWrite(13, HIGH);
        digitalWrite(4, HIGH);
      }
    }
    else{
      band_AlarmaLCD = false;
      lcd.setCursor(0,0);
      lcd.print("                    ");
      digitalWrite(13, LOW);
      digitalWrite(4, LOW);
    }

    if((PIP_F-PEEP_F)<= 30){                    //Alarma por obstruccion 
      if(band_AlarmaLCD == false){
        lcd.setCursor(0,0);
        lcd.print("Alarma: Obstruccion");
        band_AlarmaLCD = true;
        digitalWrite(13, HIGH);
        digitalWrite(4, HIGH);
      }
    }
    else{
      band_AlarmaLCD = false;
      lcd.setCursor(0,0);
      lcd.print("                    ");
      digitalWrite(13, LOW);
      digitalWrite(4, LOW);
    }
  }
  
  if (bandButton_Ac == true) {
    bandButton_Ac = false;
    ajuste = !(ajuste);
    lcd.clear();
  }
  
  if (bandButton_Sel == true) {
    bandButton_Sel = false;
    lcd.clear();
    if (ajuste == false) {
      if (bandCursor == false){
        bandCursor = true;
      }
    }
    else {
      if (bandCursor == false){
        Al_PIP--;
      }
      else {
        Al_FR--;
      }
    }
  }
  if (bandButton_Sel_2 == true) {
    bandButton_Sel_2 = false;
    lcd.clear();
    if (ajuste == false) {
      if (bandCursor == true){
        bandCursor = false;
      }
    }
    else {
      if (bandCursor == false){
        Al_PIP++;
      }
      else {
        Al_FR++;
      }
    }
  }
}
