function CREATE_Pacman(mode)
global dotPoints;
global boundary;
global intData;
intData = 0;
camera_setup();
pause(1);
if strcmp(mode,'calibration')
    PacGui2();
elseif strcmp(mode,'game')
    game_init();
    PacGui();
    try
        s = load('intConfig.mat');
        intData = 1;
        dotPoints = s.dotPoints;
        boundary = s.boundary;
    catch MExc
        disp('No stored map config file');
    end
    
end