function varargout = PacGui(varargin)
%PACGUI M-file for PacGui.fig
%      PACGUI, by itself, creates a new PACGUI or raises the existing
%      singleton*.
%
%      H = PACGUI returns the handle to a new PACGUI or the handle to
%      the existing singleton*.
%
%      PACGUI('Property','Value',...) creates a new PACGUI using the
%      given property value pairs. Unrecognized properties are passed via
%      varargin to PacGui_OpeningFcn.  This calling syntax produces a
%      warning when there is an existing singleton*.
%
%      PACGUI('CALLBACK') and PACGUI('CALLBACK',hObject,...) call the
%      local function named CALLBACK in PACGUI.M with the given input
%      arguments.
%
%      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
%      instance to run (singleton)".
%
% See also: GUIDE, GUIDATA, GUIHANDLES

% Edit the above text to modify the response to help PacGui

% Last Modified by GUIDE v2.5 22-May-2015 02:11:17

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @PacGui_OpeningFcn, ...
                   'gui_OutputFcn',  @PacGui_OutputFcn, ...
                   'gui_LayoutFcn',  [], ...
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


% --- Executes just before PacGui is made visible.
function PacGui_OpeningFcn(hObject, eventdata, handles, varargin)
% This function has no output args, see OutputFcn.
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
% varargin   unrecognized PropertyName/PropertyValue pairs from the
%            command line (see VARARGIN)

% Choose default command line output for PacGui
handles.output = hObject;

handles.axes2 = image(zeros(360,480,3),'Parent',handles.axes1);
hold on;
% preview(cam,handles.axes2);
camera_start('game',handles.axes2);

% Update handles structure
guidata(hObject, handles);

% UIWAIT makes PacGui wait for user response (see UIRESUME)
% uiwait(handles.figure1);


% --- Outputs from this function are returned to the command line.
function varargout = PacGui_OutputFcn(hObject, eventdata, handles)
% varargout  cell array for returning output args (see VARARGOUT);
% hObject    handle to figure
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Get default command line output from handles structure
varargout{1} = handles.output;

% --- Executes on button press in load_cam_calibration.
function load_cam_calibration_Callback(hObject, eventdata, handles)
% hObject    handle to load_cam_calibration (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
global cam;
try
    s = load('camConfig.mat');
    camConfig = s.camConfig;
    cam.WhiteBalance = camConfig.WhiteBalance;
    cam.Saturation = camConfig.Saturation;
    cam.ExposureMode = camConfig.ExposureMode;
    cam.Brightness = camConfig.Brightness;
    cam.Exposure = camConfig.Exposure;
    cam.WhiteBalanceMode = camConfig.WhiteBalanceMode;
    cam.Contrast = camConfig.Contrast;
catch MExc
    disp('Error loading camera config file');
end

% --- Executes on button press in start_game.
function start_game_Callback(hObject, eventdata, handles)
% hObject    handle to start_game (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
game_command('start');


% --- Executes on button press in stopgame.
function stopgame_Callback(hObject, eventdata, handles)
% hObject    handle to stopgame (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
game_command('stop');


% --- Executes on button press in recognition_overlay.
function recognition_overlay_Callback(hObject, eventdata, handles)
% hObject    handle to recognition_overlay (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of recognition_overlay


% --- Executes on button press in toggleLEDs.
function toggleLEDs_Callback(hObject, eventdata, handles)
% hObject    handle to toggleLEDs (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: get(hObject,'Value') returns toggle state of toggleLEDs


% --- Executes on button press in resume.
function resume_Callback(hObject, eventdata, handles)
% hObject    handle to resume (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
game_command('resume');


% --- Executes on button press in pause.
function pause_Callback(hObject, eventdata, handles)
% hObject    handle to pause (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)
game_command('pause');


% --- Executes when user attempts to close figure1.
function figure1_CloseRequestFcn(hObject, eventdata, handles)
% hObject    handle to figure1 (see GCBO)
% eventdata  reserved - to be defined in a future version of MATLAB
% handles    structure with handles and user data (see GUIDATA)

% Hint: delete(hObject) closes the figure
% stop(timerfind);
delete(timerfind);
delete(hObject);
