

img = snapshot(cam);
% imshow(img);
 img2 = rgb2hsv(img);
 [bw,maskedImage] = testRED(img2);
 bw = bwareaopen(bw,100);