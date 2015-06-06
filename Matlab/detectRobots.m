function [x,y] = detectRobots(img)
newHSV = rgb2hsv(img);
[mask,segImg] = testYELLOW(newHSV);
[x,y] = getCentre(mask);
[mask,segImg] = testRED(newHSV);
[x,y] = getCentre(mask);
[mask,segImg] = testPINK(newHSV);
[x,y] = getCentre(mask);
[mask,segImg] = testBLUE(newHSV);
[x,y] = getCentre(mask);
% robots.x = x;
% robots.y = y;
% need to add inky and blinky


function [x,y] = getCentre(mask)
x = [];
y = [];
global boundary;
mask = bwareaopen(mask,20);
% mask = bwmorph(mask,'erode',2);
props = regionprops(mask,'Centroid');
centroid = [];
if numel(props)~=0
    centroid = props.Centroid;

    x = 1;
    y = 1;
    while x<9&&centroid(1)>boundary.xx(x)
       x = x+1;
    end
    while y<9&&centroid(2)>boundary.yy(y)
        y = y+1;
    end
end