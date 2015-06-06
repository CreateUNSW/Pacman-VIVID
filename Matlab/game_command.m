function [game_com, game_override_dir] = game_command(command)
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


if strcmp(command,'start')
    game_com = bitor(START, MUSIC_COMMAND);
    game_override_dir = MUS_BEGIN;
elseif strcmp(command,'stop')
    game_com = STOP;
elseif strcmp(command,'pause')
    game_com = PAUSE;
elseif strcmp(command,'resume')
    game_com = RESUME;
elseif strcmp(command, 'death')
    game_com = MUSIC_COMMAND;
    game_override_dir = MUS_DEATH;
elseif strcmp(command, 'victory')
    game_com = MUSIC_COMMAND;
    game_override_dir = MUS_VICTORY;
end