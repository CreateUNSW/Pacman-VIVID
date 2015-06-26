function camera_start(mode,axes)
    global cam;
    global plotOverlay;
    if strcmp(mode,'calibration')
        preview(cam,axes);
    elseif strcmp(mode,'game')
        plotOverlay.h1 = plot(axes.Parent,NaN,NaN,'r*','MarkerSize',12,'LineWidth',2);
        plotOverlay.pacman = plot(axes.Parent,NaN,NaN,'yo','MarkerSize',48,'LineWidth',2);
        plotOverlay.blinky = plot(axes.Parent,NaN,NaN,'ro','MarkerSize',48,'LineWidth',2);
        plotOverlay.pinky = plot(axes.Parent,NaN,NaN,'mo','MarkerSize',48,'LineWidth',2);
        plotOverlay.inky = plot(axes.Parent,NaN,NaN,'bo','MarkerSize',48,'LineWidth',2);
        pacVid = timer('Period', 0.1, 'ExecutionMode','FixedSpacing');
        pacVid.TimerFcn = {@PacCam,axes};
        pause(1);
%         pacVid.StopFcn = {@StopPacVid};
        start(pacVid);
    end
    
function PacCam(~,~,axes)
global cam;
global plotOverlay;
global game;
%global intData;
img = snapshot(cam);

set(axes,'CData',img);
robots = [];
[robots.x,robots.y,robots.centre] = detectRobots(img);
% following line displays coordinates
% [robots.x;robots.y]
robots.h = ['0','0','0','0'];
pac = [];
g1 = [];
g2 = [];
g3 = [];
pac.x = robots.x(1);
pac.y = robots.y(1);
pac.h = robots.h(1);
g1.x = robots.x(2);
g1.y = robots.y(2);
g1.h = robots.h(2);
g2.x = robots.x(3);
g2.y = robots.y(3);
g2.h = robots.h(3);
g3.x = robots.x(4);
g3.y = robots.y(4);
g3.h = robots.h(4);
game_update(pac,g1,g2,g3);
game_send(game);
game(1).command      = 0;
game(1).override_dir = 0;
set(plotOverlay.pacman,'xdata',robots.centre(1,1),'ydata',robots.centre(1,2));
set(plotOverlay.blinky,'xdata',robots.centre(2,1),'ydata',robots.centre(2,2));
set(plotOverlay.pinky,'xdata',robots.centre(3,1),'ydata',robots.centre(3,2));
set(plotOverlay.inky,'xdata',robots.centre(4,1),'ydata',robots.centre(4,2));
set(plotOverlay.h1,'xdata',robots.centre(:,1),'ydata',robots.centre(:,2));


    
% function StopPacVid()