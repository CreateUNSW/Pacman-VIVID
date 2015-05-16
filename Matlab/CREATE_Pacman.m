function CREATE_Pacman(mode)
camera_setup();
pause(1);
if strcmp(mode,'calibration')
    PacGui2();
elseif strcmp(mode,'game')
    PacGui();
end