function game_update = (pac_x, pac_y, pac_h, g1_x, g1_y, g1_h, g2_x, g2_y, g2_h, g3_x, g3_y, g3_h)
    game(1).pacman(1).position = struct('x', pac_x, 'y', pac_y);
    game(1).pacman(1).heading  = pac_h;

    game(1).g(1).position = struct('x', g1_x, 'y', g1_y);
    game(1).g(1).heading  = g1_h;

    game(2).g(1).position = struct('x', g2_x, 'y', g2_y);
    game(2).g(1).heading  = g2_h;

    game(3).g(1).position = struct('x', g3_x, 'y', g3_y);
    game(3).g(1).heading  = g3_h;
end