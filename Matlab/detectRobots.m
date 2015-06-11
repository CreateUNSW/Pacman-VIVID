function [x,y,centres] = detectRobots(img)
newHSV = rgb2hsv(img);
[mask,segImg] = testYELLOW(newHSV);
[x(1),y(1),centres(1,:)] = getCentre(mask);
[mask,segImg] = testRED(newHSV);
[x(2),y(2),centres(2,:)] = getCentre(mask);
[mask,segImg] = testPINK(newHSV);
[x(3),y(3),centres(3,:)] = getCentre(mask);
[mask,segImg] = testBLUE(newHSV);
[x(4),y(4),centres(4,:)] = getCentre(mask);


function [x,y,centroid] = getCentre(mask)
global boundary;
x = 0;
y = 0;
centroid = [NaN,NaN];
mask = bwareaopen(mask,15);
props = regionprops(mask,'Centroid');
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