function varargout = PacGui2(varargin)
% PACGUI2 MATLAB code for PacGui2.fig
%      PACGUI2, by itself, creates a new PACGUI2 or raises the existing
%      singleton*.
%
%      H = PACGUI2 returns the handle to a new PACGUI2 or the handle to
%      the existing singleton*.
%
%      PACGUI2('CALLBACK',hObject,eventData,handles,...) calls the local
%      function named CALLBACK in PACGUI2.M with the given input arguments.
%
%      PACGUI2('Property','Value',...) creates a new PACGUI2 or raises the
%      existing singleton*.  Starting from the left, property value pairs are
%      applied to the GUI before PacGui2_OpeningFcn gets called.  An
%      unrecognized property name or invalid value makes property application
%      stop.  All inputs are passed to PacGui2_OpeningFcn via varargin.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help PacGui2

% Last Modified by GUIDE v2.5 15-May-2015 02:54:41

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @PacGui2_OpeningFcn, ...
                   'gui_OutputFcn',  @PacGui2_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before PacGui2 is made visible.
function PacGui2_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   command line arguments to PacGui2 (see VARARGIN)

% Choose default command line output for PacGui2
global specialObject;
global cam;
global plotHandle;
global dotPoints;
global dotPointEntry;
dotPointEntry = 0;
dotPoints = zeros(81,2);
handles.output = hObject;
specialObject = hObject;

delete(cam);

cam = webcam(2);
cam.ExposureMode = 'manual';
cam.WhiteBalanceMode = 'manual';
handles.axes2 = image(zeros(360,480,3),'Parent',handles.axes1);
preview(cam,handles.axes2);
hold on;
plotHandle.h1 = plot(NaN,NaN,'r*');
plotHandle.h2 = line(NaN,NaN,'Color','g');


% Update handles structure
guidata(hObject, handles);

% UIWAIT makes PacGui2 wait for user response (see UIRESUME)
% uiwait(handles.figure1);


% --- Outputs from this function are returned to the command line.
function varargout = PacGui2_OutputFcn(hObject, eventdata, handles) 
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;


% --- Executes on slider movement.
function saturation_Callback(hObject, eventdata, handles)
% hObject    handle to saturation (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider
global cam;
cam.Saturation = get(hObject,'Value');


% --- Executes during object creation, after setting all properties.
function saturation_CreateFcn(hObject, eventdata, handles)
% hObject    handle to saturation (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called
set(hObject,'Min',0,'Max',127,'Value',38);
% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes on slider movement.
function contrast_Callback(hObject, eventdata, handles)
% hObject    handle to contrast (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider
global cam;
cam.Contrast = get(hObject,'Value');


% --- Executes during object creation, after setting all properties.
function contrast_CreateFcn(hObject, eventdata, handles)
% hObject    handle to contrast (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called
set(hObject,'Min',0,'Max',30,'Value',13);
% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes on button press in input_intersections.
function input_intersections_Callback(hObject, eventdata, handles)
% hObject    handle to input_intersections (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% global cam;
% img = snapshot(cam);
this = get(hObject,'Value');
if this==0
    set(handles.axes2,'ButtonDownFcn', {});
else
    set(handles.axes2,'ButtonDownFcn', {@LogPoint});
end

% --- Executes on button press in clearlast.
function clearlast_Callback(hObject, eventdata, handles)
% hObject    handle to clearlast (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global dotPoints;
global dotPointEntry;
global plotHandle;
if dotPointEntry<=0
    return;
end
dotPoints(dotPointEntry,:) = [0,0];
dotPointEntry = dotPointEntry - 1;
children = plotHandle.h1.Parent.Children;
delete(findobj(children,'Type','Line'));
set(plotHandle.h1,'xdata',dotPoints(:,1),'ydata',dotPoints(:,2));


% --- Executes on button press in clearall.
function clearall_Callback(hObject, eventdata, handles)
% hObject    handle to clearall (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global dotPoints;
global dotPointEntry;
global plotHandle;
dotPoints = zeros(81,2);
dotPointEntry = 0;
children = plotHandle.h1.Parent.Children;
delete(findobj(children,'Type','Line'));
set(plotHandle.h1,'xdata',dotPoints(:,1),'ydata',dotPoints(:,2));

function LogPoint(hObject, eventdata)
global plotHandle;
global dotPoints;
global dotPointEntry;
if dotPointEntry==81
    return;
end;
dotPointEntry = dotPointEntry+1;
axesHandle = get(hObject,'Parent');
clickCoord = get(axesHandle, 'CurrentPoint');
dotPoints(dotPointEntry,:) = clickCoord(1,1:2);
set(plotHandle.h1,'xdata',dotPoints(:,1),'ydata',dotPoints(:,2));
if dotPointEntry==81
    % call function
    T = reshape(dotPoints,9,9,2);
    yy = mean(T(:,:,2));
    xx = mean(T(:,:,1)');
    axes = plotHandle.h2.Parent;
    for i=1:9
%         set(plotHandle.h2,'xdata',[xx(i),xx(i)],'ydata',[0,360]);
          line([xx(i),xx(i)],[0,360],'Color','g');
          line([0,480],[yy(i),yy(i)],'Color','g');
    end;
%     linex = [xx,xx;
    
    
end;


% --- Executes on slider movement.
function brightness_Callback(hObject, eventdata, handles)
% hObject    handle to brightness (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider
global cam;
cam.Brightness = get(hObject,'Value');


% --- Executes during object creation, after setting all properties.
function brightness_CreateFcn(hObject, eventdata, handles)
% hObject    handle to brightness (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called

% Hint: slider controls usually have a light gray background.
% global cam;
val = 0;
set(hObject,'Min',-64,'Max',64,'Value',val);
% set(hObject,'Value',val);
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end


% --- Executes on slider movement.
function whitebalance_Callback(hObject, eventdata, handles)
% hObject    handle to whitebalance (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider
global cam;
cam.WhiteBalance = get(hObject,'Value');

% --- Executes during object creation, after setting all properties.
function whitebalance_CreateFcn(hObject, eventdata, handles)
% hObject    handle to whitebalance (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called
val = 5000;
set(hObject,'Min',2800,'Max',6500,'Value',val);
% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end

% --- Executes on slider movement.
function exposure_Callback(hObject, eventdata, handles)
% hObject    handle to exposure (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider
global cam;
cam.Exposure = get(hObject,'Value');

% --- Executes during object creation, after setting all properties.
function exposure_CreateFcn(hObject, eventdata, handles)
% hObject    handle to exposure (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called
val = -5;
set(hObject,'Min',-12,'Max',-1.01,'Value',val);
% Hint: slider controls usually have a light gray background.
if isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor',[.9 .9 .9]);
end

% --- Executes on button press
function zoom_Callback(hObject, eventdata, handles)
% hObject    handle to zoom (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hints: get(hObject,'Value') returns position of slider
%        get(hObject,'Min') and get(hObject,'Max') to determine range of slider
global cam;
this = get(hObject,'Value');
if this==0
    cam.Zoom = 1;
else
    cam.Zoom = 2;
end

% --- Executes during object creation, after setting all properties.
function zoom_CreateFcn(hObject, eventdata, handles)
% hObject    handle to zoom (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    empty - handles not created until after all CreateFcns called



% --- Executes on button press in reset2defaut.
function reset2defaut_Callback(hObject, eventdata, handles)
% hObject    handle to reset2defaut (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)


% --- Executes on button press in camera_auto.
function camera_auto_Callback(hObject, eventdata, handles)
% hObject    handle to camera_auto (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global cam;
this = get(hObject,'Value');
if this==0
    cam.ExposureMode = 'manual';
    cam.WhiteBalanceMode = 'manual';
else
    cam.ExposureMode = 'auto';
    cam.WhiteBalanceMode = 'auto';
end
 
    
% Hint: get(hObject,'Value') returns toggle state of camera_auto


% --- Executes on button press in toggleLEDs.
function toggleLEDs_Callback(hObject, eventdata, handles)
% hObject    handle to toggleLEDs (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of toggleLEDs


% --- Executes during object deletion, before destroying properties.
function axes1_DeleteFcn(hObject, eventdata, handles)
% hObject    handle to axes1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)


% --- Executes when user attempts to close figure1.
function figure1_CloseRequestFcn(hObject, eventdata, handles)
% hObject    handle to figure1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
test = 1;
% Hint: delete(hObject) closes the figure
delete(hObject);

% --- Executes during object deletion, before destroying properties.
function figure1_DeleteFcn(hObject, eventdata, handles)
% hObject    handle to figure1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global cam;

closePreview(cam);
delete(cam);
clear cam;



% --- Executes on button press in closegui.
function closegui_Callback(hObject, eventdata, handles)
% hObject    handle to closegui (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global specialObject;
delete(specialObject);
