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
        pacVid = timer('Period', 0.25, 'ExecutionMode','FixedSpacing');
        pacVid.TimerFcn = {@PacCam,axes};
%         pacVid.StopFcn = {@StopPacVid};
        start(pacVid);
    end
    
function PacCam(~,~,axes)
global cam;
global intData;
img = snapshot(cam);
set(axes,'CData',img);
if intData==1
    robots = detectRobots(img);
end
    
function robots = detectRobots(img)
newHSV = rgb2hsv(img);
[mask,segImg] = testRED(newHSV);
[x,y] = getCentre(mask);
robots.x = x;
robots.y = y;
% need to add inky and blinky


function [x,y] = getCentre(mask)
global boundary;
mask = bwareaopen(mask,100);
mask = bwmorph(mask,'erode',2);
props = regionprops(mask,'Centroid');
centroid = props.Centroid;
x = 1;
y = 1;
while centroid(1)>boundary.xx(x)&&x<9
   x = x+1;
end
while centroid(2)>boundary.yy(y)&&y<9
    y = y+1;
end
    
    
        
    
    
% function StopPacVid()