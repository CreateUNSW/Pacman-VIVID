function [] = game_send(curGame)
    global mySerial;
    %% Set up the serial communication to arduino
    % NOTE: This will need to be changed to the write port on your PC
    % The next 4 lines need to be run outside of this script for some reason.
    A = [];
    A(1) = curGame(1).header;
    A(2) = curGame(1).command;
    A(3) = curGame(1).override_dir;
    % Because x and y are each 4 bits of the same byte, shift x up, add y
    A(4) = curGame(1).pacman.position.x * 16 + curGame(1).pacman.position.y;
    disp(A(4));
    A(5) = curGame(1).pacman.heading;

    A(6) = curGame(1).g.position.x * 16 + curGame(1).g.position.y;
    A(7) = curGame(1).g.heading;

    A(8) = curGame(2).g.position.x * 16 + curGame(2).g.position.y;
    A(9) = curGame(2).g.heading;

    A(10) = curGame(3).g.position.x * 16 + curGame(3).g.position.y;
    A(11) = curGame(3).g.heading;
    
    % Write serial to the arduino
    fwrite(mySerial, A);
end