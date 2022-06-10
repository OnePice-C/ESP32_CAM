char buff[50];
byte sz = 0;

void setup() 
{
  Serial.begin(115200);
  pinMode(4, OUTPUT);
}
  
void loop() 
{
    while (Serial.available()) 
    {
      byte C = Serial.read();
      if( sz < sizeof(buff))
      {
        buff[sz++] = C;
        if( C == 'b')
        {
          Serial.write('b');
          digitalWrite(4, HIGH);
          delay(100);
          digitalWrite(4, LOW);
          delay(100);
          sz=0;
         }  
      }
    }
  
}
