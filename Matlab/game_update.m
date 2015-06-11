function game_update(pac,g1,g2,g3)
    global game;
    game(1).pacman(1).position = struct('x', pac.x, 'y', pac.y);
    game(1).pacman(1).heading  = pac.h;

    game(1).g(1).position = struct('x', g1.x, 'y', g1.y);
    game(1).g(1).heading  = g1.h;

    game(2).g(1).position = struct('x', g2.x, 'y', g2.y);
    game(2).g(1).heading  = g2.h;

    game(3).g(1).position = struct('x', g3.x, 'y', g3.y);
    game(3).g(1).heading  = g3.h;
end