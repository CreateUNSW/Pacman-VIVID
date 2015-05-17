%% Set up the serial communication to arduino
% NOTE: This will need to be changed to the write port on your PC
% The next 4 lines need to be run outside of this script for some reason.
s = serial('COM11', 'BaudRate', 57600, 'Terminator', '');
fopen(s);
% To clean up ALL open com ports, run this line
% fclose(instrfind);

if (strcmp (get (s, 'status'), 'closed'))
   error('Please open the file to communicate with the arduino...'); 
end
%% Set up local variables used by script
% Game definitions
GAME_SIZE       = 11;
% Game commands
NOP             = 0;
START           = 1;
STOP            = 2;
HIDE            = 4;
PAUSE           = 8;
RESUME          = 16;
MUSIC_COMMAND   = 32;
MANUAL_OVERRIDE = 64;
% Override directives
MUS_DEATH   = 64;
MUS_VICTORY = 32;
MUS_BEGIN   = 16;
U = 8;
R = 4;
D = 2;
L = 1;
% Note: Use bitor to compose two commands/directives. e.g. bitor(val1,val2)
%% Create game structure
% Print details of what serial comms. have been done
game(1).header       = 1;
game(1).command      = bitor(START, MUSIC_COMMAND);
game(1).override_dir = MUS_BEGIN;
game(1).pacman       = struct('position', {}, 'heading', {});
game(1).g            = struct('position', {}, 'heading', {});
game(2).g            = struct('position', {}, 'heading', {});
game(3).g            = struct('position', {}, 'heading', {});

game(1).pacman(1).position = struct('x', 1, 'y', 2);
game(1).pacman(1).heading  = U;

game(1).g(1).position = struct('x', 3, 'y', 4);
game(1).g(1).heading  = D;

game(2).g(1).position = struct('x', 5, 'y', 6);
game(2).g(1).heading  = L;

game(3).g(1).position = struct('x', 7, 'y', 8);
game(3).g(1).heading  = R;

% NOTE: To access each ghost, you simply need to do: 
% game(x).g.[position or heading] = y; Where x is the ghost you want.
% All other references will be to game(1).header -- for example
 
%% Activate serial communications defined by serial argument s


A(1) = game(1).header;
A(2) = game(1).command;
A(3) = game(1).override_dir;
% Because x and y are each 4 bits of the same byte, shift x up, add y
A(4) = game(1).pacman.position.x * 16 + game(1).pacman.position.y;
A(5) = game(1).pacman.heading;

A(6) = game(1).g.position.x * 16 + game(1).g.position.y;
A(7) = game(1).g.heading;

A(8) = game(2).g.position.x * 16 + game(2).g.position.y;
A(9) = game(2).g.heading;

A(10) = game(3).g.position.x * 16 + game(3).g.position.y;
A(11) = game(3).g.heading;
% Print A to console
A
% Write 10 times then quit.
for i = 1:100
   fwrite(s, A+i);
   pause(0.1);
end

% fwrite(s,A);
% pause(1);
% Print details of what serial comms. have been done
s
% End of file, cleanup
fclose(s);
delete(s);
