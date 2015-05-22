function CREATE_Pacman(mode)
camera_setup();
pause(1);
if strcmp(mode,'calibration')
    PacGui2();
elseif strcmp(mode,'game')
    PacGui();
    try
        s = load('intConfig.mat');
        global dotPoints;
        global boundary;
        dotPoints = t.dotPoints;
        boundary = t.boundary;
    catch MExc
        disp('No stored map config file');
    end
end