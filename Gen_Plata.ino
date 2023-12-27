/*                         *** Generador de Plata Coloidal de 200mL ***

                         >>> CONFIGURACIÓN Y FUNCIONAMIENTO AL INICIAR <<<
    1 - Detección de los cortes de red mientras funciona: al reiniciar los electrodos no deben estar en contacto con el agua
    2 - Mediante el pulsador se puede configurar para producir Plata Coloidad entre 10ppm y 100ppm
    3 - Realiza un test de conductividad del agua (TDS), para que funcione con una corriente > 1mA
    4 - Al iniciar se calcula el tiempo de la electrolisis para 200mL, en función de los ppm y la corriente que circula entre los electrodos 

                                       # IMPORTANTE #
  >>> Cuando se construye el generador, es necesario realizar una calibración de las variables Factor y Shunt <<<                            
____________________________________________________________________________________
                                    Escrito por: J_RPM
                                     http://j-rpm.com
                                 v1.2 >>> Diciembre de 2023
____________________________________________________________________________________

*/
#include <LiquidCrystal.h>

#define REINICIAR asm("jmp 0x0000")

#define Pulsa       2    // Pulsador
#define Relay       3    // Conexión del Relay
#define Connect    A5    // Medida para detectar el contacto de los electrodos con el agua 

const String HW = "(v1.2)";   // Versión actual del programa
const float Factor = 1.4;     // (CALIBRAR) Factor multiplicador para calcular el tiempo en función de las 'ppm' de plata coloidal
const float Shunt = 4.93;     // (CALIBRAR) Factor multiplicador para calcular la corriente a partir del valor del conversor ADC
const int umbral = 50;        // Valor umbral del conversor analógico, para detectar cuando están los electrodos sumergidos 

int valor_PPM = 50;           // Valor ppm por defecto
int menu = 5;                 // Menú del ppm por defecto
int segPoducidos = 0;
int ahoraSeg = 0;
int minutos = 0;
int segundos = 0;
int fase = 0;        
int conta = 0;
float corriente = 0;
float mAref = 0;
float ahora_PPM = 0;

bool iniciando = true;
bool calibraTDS = true;
bool errorInit = true;
bool pulsado = false;
unsigned long now = millis();

// Inicializa la librería del display, con los pines de conexión de Arduino 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Define los 8 carácteres CGRAM del LCD
  byte es_0[8]={ //0
    B00001,
    B11101,
    B00001,
    B00001,
    B00001,
    B00001,
    B00001,
    B00000,
  };
  byte es_1[8]={ //1
    B10000,
    B00000,
    B10000,
    B00000,
    B10000,
    B00000,
    B10000,
    B00000,
  };
  byte es_2[8]={ //2
    B00100,
    B00000,
    B00100,
    B00000,
    B00100,
    B00000,
    B00100,
    B00000,
  };
  byte es_3[8]={ //3
    B00001,
    B00000,
    B00001,
    B00000,
    B00001,
    B00000,
    B00001,
    B00000,
  };
  byte es_4[8]={ //4
    B00100,
    B01110,
    B00100,
    B00000,
    B00100,
    B01110,
    B00100,
    B00000,
  };
  byte es_5[8]={ //5
    B10010,
    B00111,
    B10010,
    B10000,
    B10000,
    B00000,
    B10000,
    B00000,
  };
  byte es_6[8]={  //6
    B10010,
    B10111,
    B10010,
    B10000,
    B10000,
    B10000,
    B10000,
    B00000,
  };
  byte es_7[8]={ //7
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00100,
    B00000,
  };

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Setup 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void setup() {
  // Configura los pines
  pinMode(Pulsa, INPUT); 
  digitalWrite(Pulsa, HIGH);  // Pull-Up
  pinMode(Relay, OUTPUT); 
  digitalWrite(Relay, LOW);   // Desconecta la tensión del generador
 
  // Open the Serial port
  Serial.begin(9600);
  Serial.println(F("------------------------------------"));
  Serial.print(F("Generador de Plata Coloidad de 200mL."));
  Serial.println(HW);

  // Inicializa el LCD 
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print(F("Gen_Plata "));
  lcd.print(HW);
  lcd.setCursor(0, 1);
  lcd.print(F("Coliodal (J_RPM)"));
  delay(1000);

  // Almacena los caracteres extra en la CGRAM del LCD
  lcd.createChar(0x08, es_0);     //Electrodo IZDA
  lcd.createChar(0x09, es_1);     //ON - 1  
  lcd.createChar(0x0A, es_2);     //ON - 2
  lcd.createChar(0x0B, es_3);     //ON - 3
  lcd.createChar(0x0C, es_4);     //ON - 4
  lcd.createChar(0x0D, es_5);     //ON - 5
  lcd.createChar(0x0E, es_6);     //Electrodo DCH
  lcd.createChar(0x0F, es_7);     //PARADO
  delay(500);

  // Presenta los 8 caracteres CGRAM en el LCD
  // CGRAM se encuentra: 0x00...0x07 
  // y también en: 0x08...0x0F
  lcd.setCursor(0, 1);
  lcd.print(F("CGRAM: "));
  for (int i=0x08; i<=0x10; i++){
    lcd.print((char)i);
  }
  delay(2000);
  
  // Inicia el generador
  lcd.setCursor(0, 1);
  lcd.print(F("GENERADOR 200mL."));
  
  delay(2000);
  
  // Muestra INICIO por el puerto serie
  Serial.println(F("# INICIO #"));
 
  // Tiempo máximo para cambiar la configuración = 5"
  now = millis() + 5000;
  
} // Setup
////////////////////////////////////////////////////////////////////////////
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
////////////////////////////////////////////////////////////////////////////
void loop() {
  // Al reiniciar, se comprueba que están los electrodos fuera del agua
  if (iniciando == true) {
    TestElectrodos();
    iniciando = false;     
  }
  
  // >>> Tiempo de configuración con TimeOut
  if (now > millis()) {  
 
    if (errorInit == true) {
      // ERROR al iniciar ... electrodos sumergidos
      lcd.setCursor(0, 0);
      lcd.print(F("ERROR ELECTRODOS"));
      delay(5000);
      REINICIAR;
      
    }else {
      lcd.setCursor(0, 1);
      lcd.print(F("PULSA = MODIFICA"));
      
      // Test del pulsador
      if (digitalRead(Pulsa) == LOW) {
        pulsado = true;
        menu = menu + 1;
        if (menu > 7) menu = 0;
      }

      // Actualiza el tiempo total 
      CalculaSegundos();

      // Muestra el valor PPM
      lcd.setCursor(0, 0);
      lcd.print(F("Objetivo: "));
      if (menu < 7) lcd.print (" ");
      lcd.print (valor_PPM);
      lcd.print ("ppm");
       
       // Test del pulsador
      if (pulsado == true) {
        while (digitalRead(Pulsa) == LOW) {
        lcd.setCursor(0, 1);
        lcd.print(F(">Soltar PULSADOR"));
        delay(100);
        }
        now = millis() + 3000;
        pulsado = false;
      }
      
    }
      
  // >>> Funcionamiento
  } else {
    
    // Toma la medida del conversor analógico y calcula la corriente
    int valor = analogRead(Connect); 
    corriente = (valor * Shunt) / 1000;     //mA
    Serial.print(F("Valor: "));
    Serial.println(valor);
    Serial.print(F("Corriente: "));
    Serial.print(corriente);
    Serial.println(F("mA"));
    
    lcd.setCursor(0, 0);
    
    // Comprueba si están los electrodos dentro del agua, para contabiliar el tiempo de la electrolisis
    if (valor > umbral) {
      if (calibraTDS == false) {
        ahoraSeg = ahoraSeg - 1;
        if (ahoraSeg < 1) ahoraSeg == 0;
        conta = 0;
      }
      
      
      // Calcula el ppm actual, en función de los segundos transcurridos
      if (ahoraSeg > 0 && calibraTDS == false) {
        float TDS = Factor * (2 / mAref);
        segPoducidos = (60 * valor_PPM * TDS) - ahoraSeg;
        ahora_PPM = (segPoducidos / TDS) / 60;
      }
      
      // Muestra el valor PPM actual
      lcd.setCursor(0, 0);
      lcd.print(F("        >"));
      lcd.setCursor(0, 0);
      if (ahoraSeg > 0) {
        lcd.print(ahora_PPM);
        lcd.print(F("ppm"));
      }else {
        lcd.print(F("<RETIRAR"));
      }
      
      // Muestra el valor PPM objetivo
      lcd.setCursor(0, 8);
      lcd.print(F(">"));
      if (menu < 7) lcd.print (" ");
      lcd.print (valor_PPM);
      lcd.print ("ppm");
      Serial.println("Electrodos en contacto con el agua");
    }else {
      lcd.setCursor(0, 0);
      if(ahoraSeg > 0) {
        lcd.print(F("SUMERGIR! "));
        Serial.println("Introducir los electrodos en el agua >>> TIEMPO DETENIDO");
      }else {
        lcd.print(F("<PUL=INI"));
        Serial.println("PULSAR para reiniciar");
      }
    }

    // Calcula el tiempo que falta para terminar (mm:ss)
    minutos = ahoraSeg / 60;
    segundos = ahoraSeg - (minutos * 60);     
   
    // Comprueba si ha finalizado el tiempo
    if (ahoraSeg < 1) {
      digitalWrite(Relay, LOW);   // Desconecta la tensión del generador
      lcd.setCursor(0, 1);
      lcd.print(F("FIN DEL PROCESO!"));
      Serial.println(F("FIN DEL PROCESO!"));
      
     // Test del pulsador, para REINICIAR
     if (digitalRead(Pulsa) == LOW) {
       while (digitalRead(Pulsa) == LOW) {
        lcd.setCursor(0, 1);
        lcd.print(F(">Soltar PULSADOR"));
        delay(100);
       }
       REINICIAR;
     }
      
    }else {
      digitalWrite(Relay, HIGH);  // Conecta la tensión del generador
      // Muestra la animación y actualiza el tiempo restante en LCD
      lcd.setCursor(0, 1);
      if (valor > umbral) {
        fase = fase + 1;
        
        lcd.print((char)0);
        if (fase > 3) {
          lcd.print((char)4);
          lcd.print((char)5);
          fase = 0;
        }else {
          lcd.print((char)fase);
          lcd.print((char)6);
        }
     
      }else {
        lcd.print((char)7);
        lcd.print(F(" "));
        lcd.print((char)7);
      }
      lcd.print(F(" "));
      lcd.print(corriente);
      lcd.print(F("m"));
      
      // Comprueba si está calibrando el tiempo en función del valor TDS del agua
      if (calibraTDS == true) {
        // Comprueba si están los electrodos dentro del agua
        if (valor > umbral) {
          conta = conta + 1;
          lcd.print(F("A TDS#"));
          lcd.print(conta);
          // Toma como referencia la medida máxima de corriente de las 5 muestras
          if (mAref < corriente) mAref = corriente;
          
          // Después de 5 muestras, se reajusta el tiempo en función de la conductividad del agua
          if (conta > 5) {
            conta = 4;
            // Comprueba que la corriente sea > 1mA  
            if (mAref > 1) {
              // La referencia del tiempo inicial es de 2 mA de corriente entre electrodos 
              ahoraSeg = (ahoraSeg * 2) / mAref;
              calibraTDS = false;
            }else {
              lcd.setCursor(0, 0);
              lcd.print(F("Bajo TDS"));
              lcd.setCursor(11, 1);
              lcd.print(F("I.Low"));
            }
          }
        }else {
          conta = 0;
          lcd.print(F("A TDS:?"));
        }
        
        
      // Muestra el tiempo restante en el display 
      }else {
        if (minutos < 10) {
          lcd.print(F("A  "));
        }else if (minutos < 100) {
          lcd.print(F("A "));
        } else{
          lcd.print(F(" "));
        }
        lcd.print(minutos);
        lcd.print(F(":"));
        if (segundos < 10) lcd.print(F("0"));
        lcd.print(segundos);
      }
      
    }
    
    // Base de tiempo 1"
    delay(985);  //CALIBRADO para compensar el retardo de las rutinas
  }
  
 
} // loop

//////////////////////////////////////////////////////////////
void CalculaSegundos(){
  if (menu == 0) {
    valor_PPM = 10;   
  }else if (menu == 1) {
    valor_PPM = 20;   
  }else if (menu == 2) {
    valor_PPM = 25;   
  }else if (menu == 3) {
    valor_PPM = 30;   
  }else if (menu == 4) {
    valor_PPM = 40;   
  }else if (menu == 5) {
    valor_PPM = 50;   
  }else if (menu == 6) {
    valor_PPM = 75;   
  }else {
    valor_PPM = 100;   
  }

  // Calcula los segundos en fución de la concentración (ppm), para una corriente de 2mA 
  ahoraSeg = 60 * valor_PPM * Factor;
  
}
//////////////////////////////////////////////////////////////
void TestElectrodos(){
  // Comprueba si están los electrodos dentro del agua
  digitalWrite(Relay, HIGH);  // Conecta la tensión del generador
  delay(500);
  int valor = analogRead(Connect); 
  digitalWrite(Relay, LOW);   // Desconecta la tensión del generador

  Serial.print(F("Valor: "));
  Serial.println(valor);
  Serial.println(F("*** CONFIGURACION ***"));
  if (valor > umbral) {
    errorInit = true;
    Serial.println(F("ERROR: electrodos en contacto con el agua"));
  }else {
    errorInit = false;
    Serial.println(F("OK: electrodos aislados"));
  }
}
//////// FIN //////////////////////////////////////////////////////////////////////
