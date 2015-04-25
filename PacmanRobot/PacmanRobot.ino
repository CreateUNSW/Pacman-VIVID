#define HEADER 0xC8
#define U 0b1000
#define R 0b0100
#define D 0b0010
#define L 0b0001

typedef struct {
  unsigned int x : 4;
  unsigned int y : 4;
  char heading;
} pose;

struct gs{
  byte header;
  byte state;
  pose Pose[5];
} gameState;

byte pacMap[9][9] = {{R|D,   R|D|L,  R|L,    D|L,    0,     R|D,    R|L,    R|D|L,  D|L},
                  {U|D,   U|R|D,  R|D|L,  U|R|L,  R|L,   U|R|L,  R|D|L,  U|D|L,  U|D},
                  {U|R,   U|D|L,  U|R,    D|L,    0,     R|D,    U|L,    U|R|D,  U|L},
                  {0,     U|D,    R|D,    U|R|L,  R|D|L, U|R|L,  D|L,    U|D,    0  },
                  {R|D,   U|R|D|L,U|D|L,  R,      U|R|L, L,      U|R|D,  U|R|D|L,D|L},
                  {U|D,   U|D,    U|R,    R|D|L,  R|L,   R|D|L,  U|L,    U|D,    U|D},
                  {U|R,   U|D|L,  R|D,    U|R|D|L,R|L,   U|R|D|L,D|L,    U|R|D,  U|L},
                  {R|D,   U|R|L,  U|L,    U|D,    0,     U|D,    U|R,    U|R|L,  D|L},
                  {U|R,   R|L,    R|L,    U|R|L,  R|L,   U|R|L,  R|L,    R|L,    U|L}};
                  
                  
pose Pose;
void setup() {
  Pose.x = 9;
  Pose.y = 8;
  Pose.heading = 'n';
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.print("Size of Pose:  ");
  Serial.println(sizeof(Pose));
  Serial.println("Contents of Pose:");
  Serial.println(Pose.x);
  Serial.println(Pose.y);
  Serial.println(Pose.heading);
  Serial.print("Size of game state:  ");
  Serial.println(sizeof(gameState));
  int i,j;
  byte temp = 0;
  for(i=0;i<9;i++){
    for(j=0;j<9;j++){
      temp = pacMap[i][j];
      Serial.print(temp,BIN);// works
      
      //doesn't work yet?
      /*
      Serial.print(temp&U,BIN);
      Serial.print(temp&R,BIN);
      Serial.print(temp&D,BIN);
      Serial.print(temp&L,BIN);
      */
      
      Serial.print(" ");
    }
    Serial.println();
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
