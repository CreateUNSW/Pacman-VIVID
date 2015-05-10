% camera setup

camList = webcamlist
cam = webcam(2) % can set exposure and other properties from this handle
cam.ExposureMode = 'manual';
cam.WhiteBalanceMode = 'manual';
preview(cam);

% need to ensure that it looks good on laptop
% variables that can be set

%cam.Zoom
%cam.Exposure
%cam.WhiteBalance
%cam.Contrast
%cam.Saturation
%cam.Brightness
%cam.Exposure
%cam.Sharpness
%cam.Gamma
