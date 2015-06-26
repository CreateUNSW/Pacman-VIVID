function game_command(command)
global game;
%% Set up local variables used by script
% Game definitions
% GAME_SIZE       = 11;
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
% U = 8;
% R = 4;
% D = 2;
% L = 1;
% Note: Use bitor to compose two commands/directives. e.g. bitor(val1,val2)


if strcmp(command,'start')
    game(1).command = bitor(START, MUSIC_COMMAND);
    game(1).override_dir = MUS_BEGIN;
elseif strcmp(command,'stop')
    game(1).command = bitor(STOP, MUSIC_COMMAND);
    game(1).override_dir = MUS_DEATH;
elseif strcmp(command,'pause')
    game(1).command = bitor(PAUSE,MUSIC_COMMAND);
    game(1).override_dir = MUS_VICTORY;
elseif strcmp(command,'resume')
    game(1).command = RESUME;
elseif strcmp(command, 'death')
    game(1).command = MUSIC_COMMAND;
    game(1).override_dir = MUS_DEATH;
elseif strcmp(command, 'victory')
    game(1).command = MUSIC_COMMAND;
    game(1).override_dir = MUS_VICTORY;
end
game_send(game);