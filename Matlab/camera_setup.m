% need to ensure that it looks good on laptop
    % variables that can be set

    %cam.Zoom
    %cam.Exposure
    %cam.WhiteBalance
    %cam.Contrast
    %cam.Saturation
    %cam.Brightness
    %cam.Sharpness
    %cam.Gamma

function camera_setup()
    global cam;
    % camList = webcamlist
    delete(cam);
    cam = webcam(1) % can set exposure and other properties from this handle
    cam.Resolution = '352x288';
%     cam.ExposureMode = 'manual';
%     cam.WhiteBalanceMode = 'manual';