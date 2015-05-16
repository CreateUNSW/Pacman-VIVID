function camera_start(mode,axes)
    global cam;
    if strcmp(mode,'calibration')
        preview(cam,axes);
    elseif strcmp(mode,'game')
        pacVid = timer('Period', 0.25, 'ExecutionMode','FixedSpacing');
        pacVid.TimerFcn = {@PacCam,axes};
%         pacVid.StopFcn = {@StopPacVid};
        start(pacVid);
    end
    
function PacCam(~,~,axes)
    global cam;
    img = snapshot(cam);
    set(axes,'CData',img);
    
    
% function StopPacVid()