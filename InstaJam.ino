/*
 * InstaJam
 * 
 * At the push of a button, program takes input from a microphone, identifies and classifies and prints key of tune being played
 * 
 * Fernando Garcia
 * 
 * Feb 20 2016
 */

//clipping indicator
boolean clipping = 0;

//data storage variables
byte newData = 0;
byte prevData = 0;
unsigned int time = 0;//keeps time and sends vales to store in timer[] occasionally
int timer[10];//stores timing of events
int slope[10];//stores slope of events
unsigned int totalTimer;//used to calculate period
unsigned int period;//period of wave
byte index = 0;//current storage index
float freq=0.00;//storage for frequency calculations
int maxSlope = 0;//max slope used as trigger point
int newSlope;//keeps slope data

//variables used to characterize key
const char *note;
boolean confirm;
const char *halfNotes[7]={"","","","","","",""};
int arraySize=7;
int m=0;
int l=0;
const char *characteristic;
bool done=false;

//variables for decided whether you have a match
byte noMatch = 0;//counts how many non-matches you've received to reset variables if it's been too long
byte slopeTol = 10;//slope tolerance
int timerTol = 10;//timer tolerance

//variables for amp detection
unsigned int ampTimer = 0;
byte maxAmp = 0;
byte checkMaxAmp;
byte ampThreshold = 30;//raise if you have a very noisy signal

//variables for button actioning 
int buttonPin = 2;  
int ledPin = 13;
int buttonState = LOW;
int ledState = LOW;
int openMic = LOW;     
const long interval = 10000;//10s of listening
unsigned long previousMillis;    


void setup(){
  
  Serial.begin(9600);
//  pinMode(13,OUTPUT);//led indicator pin
  pinMode(ledPin,OUTPUT);//output pin
  pinMode(buttonPin, INPUT);//button pin
  
  cli();//diable interrupts
  
  //set up continuous sampling of analog pin 0 at 38.5kHz
 
  //clear ADCSRA and ADCSRB registers
  ADCSRA = 0;
  ADCSRB = 0;
  
  ADMUX |= (1 << REFS0); //set reference voltage
  ADMUX |= (1 << ADLAR); //left align the ADC value- so we can read highest 8 bits from ADCH register only
  
  ADCSRA |= (1 << ADPS2) | (1 << ADPS0); //set ADC clock with 32 prescaler- 16mHz/32=500kHz
  ADCSRA |= (1 << ADATE); //enabble auto trigger
  ADCSRA |= (1 << ADIE); //enable interrupts when measurement complete
  ADCSRA |= (1 << ADEN); //enable ADC
  ADCSRA |= (1 << ADSC); //start ADC measurements
  
  sei();//enable interrupts
}

ISR(ADC_vect) {//when new ADC value ready
  
//  PORTB &= B11101111;//set pin 12 low
  prevData = newData;//store previous value
  newData = ADCH;//get value from A0
  if (prevData < 127 && newData >=127){//if increasing and crossing midpoint
    newSlope = newData - prevData;//calculate slope
    if (abs(newSlope-maxSlope)<slopeTol){//if slopes are ==, record new data and reset time
      slope[index] = newSlope;
      timer[index] = time;
      time = 0;
      if (index == 0){//new max slope just reset
//        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
        index++;//increment index
      }
      else if (abs(timer[0]-timer[index])<timerTol && abs(slope[0]-newSlope)<slopeTol){//if timer duration and slopes match, sum timer values
        totalTimer = 0;
        for (byte i=0;i<index;i++){
          totalTimer+=timer[i];
        }
        period = totalTimer;//set period. reset new zero index values to compare with
        timer[0] = timer[index];
        slope[0] = slope[index];
        index = 1;//set index to 1
//        PORTB |= B00010000;//set pin 12 high
        noMatch = 0;
      }
      else{//crossing midpoint but not match
        index++;//increment index
        if (index > 9){
          reset();
        }
      }
    }
    else if (newSlope>maxSlope){//if new slope is much larger than max slope
      maxSlope = newSlope;
      time = 0;//reset clock
      noMatch = 0;
      index = 0;//reset index
    }
    else{//slope not steep enough
      noMatch++;//increment no match counter
      if (noMatch>9){
        reset();
      }
    }
  }
    
  if (newData == 0 || newData == 1023){//if clipping
//    PORTB |= B00100000;//set pin 13 high- turn on clipping indicator led
    clipping = 1;//currently clipping
  }
  
  time++;//increment timer at rate of 38.5kHz
  
  ampTimer++;//increment amplitude timer
  if (abs(127-ADCH)>maxAmp){
    maxAmp = abs(127-ADCH);
  }
  if (ampTimer==1000){
    ampTimer = 0;
    checkMaxAmp = maxAmp;
    maxAmp = 0;
  }
  
}

void reset(){//clear out some variables
  index = 0;//reset index
  noMatch = 0;//reset match couner
  maxSlope = 0;//reset slope
}


void checkClipping(){//manage clipping indicator LED
  if (clipping){//if currently clipping
//    PORTB &= B11011111;//turn off clipping indicator led
    clipping = 0;
  }
}

float scaleDown(float freq){//characterize note independently of octave 
  while (freq>30.87){
    freq=freq/2;
  }
  return freq;
}


const char *freqTree(float freq){//dirty binary tree to match frequency to note
  if (freq<22.48){
    if (freq<18.90){
      if (freq<17.32){
        if (0.00<freq<16.84){
          if (freq==0.00){
            note="**";
          }
          else{
            note="C ";
          }
        }
        else{
          note="C#";        
        }
      }
      else{
        if (freq<17.84){
          note="C#";       
        }
        else{
          note="D ";        
        }  
      }
    }
    else{
      if (freq<20.60){
        if (freq<20.03){
          note="D ";
        }
        else{
          note="E ";        
        }
      }
      else{
        if (freq<21.22){
          note="E ";
        }
        else{
          note="F ";        
        }
      }
    }
  }
  else{
    if (freq<26.73){
      if (freq<24.50){
        if (freq<23.81){
          note="F#";
        }
        else{
          note="G ";
        }
      }
      else{
        if (freq<25.23){
          note="G ";
        }
        else{
          note="G#";
        }
      }
    }
    else{
      if (freq<29.14){
        if (freq<28.32){
          note="A ";
        }
        else{
          note="A#";
        }
      }
      else{
        if (freq<30.01){
          note="A#";
        }
        else{
          note="B ";
        }
      }
    }
  }
  return note;
}

void halfNoteArray(float freq){//listens for halfnotes, prints unique array

  int j;
  int k;
  boolean unique=true;
  if (note[1] == '#'){
    for (j=0;j<=m;j++){
      if (note == halfNotes[j]){
        unique=false;
        break;
      }
    }
    if (unique==true){
      halfNotes[m] = note;
      m = m+1; 
      for (k=0;k<m;k++){
        Serial.print(halfNotes[k]);
      }
      Serial.print("\n");
      l=l+1;
    }
  }
  
//  else if (note == "**"){
//    Serial.print("No frequency detected");
//  }

}


bool existsInArray(const char *characteristic){//returns whether or not an element exists in an array
  int z;
  for (z=0;z<arraySize;z++){
    if (halfNotes[z] == characteristic)
      return true;
  }
  return false;
}

/*

KEY DETECTION:

Major Keys                     Characteristic Note

0 C =                            
1 F = A#                       A# 
2 Bb = A# D#                   A#
3 Eb = A# D# G#                A#
4 Ab = A# D# G# C#             A#
5 Db = A# D# G# C# F#          Same as B?
6 Gb = A# D# G# C# F# B        B# 
7 Cb = A# D# G# C# F# B E      Same as C#?

1 G = F#
2 D = F# C#
3 A = F# C# G#
4 E = F# C# G# D#
5 B = F# C# G# D# A#
6 F# = F# C# G# D# A# E
7 C# = F# C# G# D# A# E B

*/

void findKey(){//Depending on the notes identified, function returns the key to match
  switch (l){
s    case 0:
      Serial.print("Key: C");
      break;
    case 1:
      if (existsInArray("A#") == true){
        Serial.print("Key: F");
        break;
      }
      else{
        Serial.print("Key: G");
        break;
     }
    case 2:
      if (existsInArray("A#") == true){
        Serial.print("Key: Bb");
        break;
      }
      else{
        Serial.print("Key: D");
        break;
      }
    case 3:
      if (existsInArray("A#") == true){
        Serial.print("Key: Eb");
        break;
      }
      else{
        Serial.print("Key: A");
        break;
      }
    case 4:
      if (existsInArray("A#") == true){
        Serial.print("Key: Ab");
        break;
      }
      else{
        Serial.print("Key: E");
        break;
      }
    case 5:
      if (existsInArray("B ") == true){  //simplify with bit operations
        if (existsInArray("E ") == true){
          Serial.print("Key: Cb or C#");
          break;
        }
        else{
          if (existsInArray("E ") == true){
            Serial.print("Key: Gb");
            break;
          }
        }
      }
      else{
        if (existsInArray("E ") == true){
          Serial.print("Key: F#");
          break;
        }
        else{
          Serial.print("Key: Db or B");
          break;
        }
      }
  }
}

void deleteArray(){//Resets contents of array
  int s;
  for (s=0;s<arraySize;s++){
    halfNotes[s]="";
  }
}

void loop(){

  //Start clock
  unsigned long currentMillis = millis();

  //Depending on the state of a button, the program will take a different path
  buttonState = digitalRead(buttonPin);

  //When button is pressed, program begins listening
  if (buttonState == HIGH){
    l=0;  //reset variables      
    done=false;
    previousMillis = currentMillis;
    openMic = HIGH;
    digitalWrite(ledPin,HIGH);

  }
  else if (currentMillis - previousMillis > interval){
    if (done==false){

      openMic = LOW;
      digitalWrite(ledPin,LOW);
      findKey();
      deleteArray();
   
    done=true;
    Serial.print("\n");
    Serial.print("Done");
    Serial.print("\n");
    Serial.print("\n");
    }
  } 

/*
  Serial.print("                      ");
  Serial.print(currentMillis);   
  Serial.print("                                    ");
  Serial.print(previousMillis);
  Serial.print("\n");
*/

  //When listening mode is on, check that reading is within bounds
  if (openMic == HIGH){
//    checkClipping()
    if (checkMaxAmp>ampThreshold){
      if (period == 0){// adjust tolerance to avoid freq=0
        freq = 0;
      }
      else{
        freq = 38462/float(period);//calculate frequency timer rate/period        
      }
   
    //Characterize note, independently of octave
    freq = scaleDown(freq);

    //Identify note based on frequency
    note = freqTree(freq);

    //Listen for characteristic half notes and print unique key
    halfNoteArray(freq);
    
    }
  }
}
