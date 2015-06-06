function [] = game_send(s,game)
    %% Set up the serial communication to arduino
    % NOTE: This will need to be changed to the write port on your PC
    % The next 4 lines need to be run outside of this script for some reason.
    
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
    
    % Write serial to the arduino
    fwrite(s, A);
end