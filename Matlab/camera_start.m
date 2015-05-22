function camera_start(mode,axes)
    global cam;
    global plotOverlay;
    if strcmp(mode,'calibration')
        preview(cam,axes);
    elseif strcmp(mode,'game')
        plotOverlay.h1 = plot(axes.Parent,NaN,NaN,'r*','MarkerSize',12,'LineWidth',2);
        plotOverlay.pacman = plot(axes,NaN,NaN,'yo','MarkerSize',48,'LineWidth',2);
        plotOverlay.blinky = plot(axes,NaN,NaN,'ro','MarkerSize',48,'LineWidth',2);
        plotOverlay.pinky = plot(axes,NaN,NaN,'mo','MarkerSize',48,'LineWidth',2);
        plotOverlay.inky = plot(axes,NaN,NaN,'bo','MarkerSize',48,'LineWidth',2);
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