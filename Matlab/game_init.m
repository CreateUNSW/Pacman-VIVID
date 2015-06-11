s = serial('COM10', 'BaudRate', 57600, 'Terminator', '');
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
%% Create a game state global variable
global game;
game = [];
game(1).header       = 1;
game(1).command      = bitor(START, MUSIC_COMMAND);
game(1).override_dir = MUS_BEGIN;
game(1).pacman       = struct('position', {}, 'heading', {});
game(1).g            = struct('position', {}, 'heading', {});
game(2).g            = struct('position', {}, 'heading', {});
game(3).g            = struct('position', {}, 'heading', {});

game(1).pacman(1).position = struct('x', 5, 'y', 7);
game(1).pacman(1).heading  = R;

game(1).g(1).position = struct('x', 4, 'y', 5);
game(1).g(1).heading  = R;

game(2).g(1).position = struct('x', 5, 'y', 5);
game(2).g(1).heading  = U;

game(3).g(1).position = struct('x', 6, 'y', 5);
game(3).g(1).heading  = L;
