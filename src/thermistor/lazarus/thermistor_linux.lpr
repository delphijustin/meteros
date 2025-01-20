program thermistor_linux;
(*
This is the calibrator tool for linux.
*)
{$mode objfpc}{$H+}

uses
  {$IFDEF UNIX}
  cthreads,
  {$ENDIF}
  Classes, SysUtils, CustApp,math,BaseUnix;

type

  { TThermistorCalc }

  TThermistorCalc = class(TCustomApplication)
  protected
    procedure DoRun; override;
  public
    constructor Create(TheOwner: TComponent); override;
    destructor Destroy; override;
  end;

{ TThermistorCalc }


type
  TMeasurement = record
    X: Single;
    Y: Single;
  end;
    TTemperatureFormula=function(X:Single;Reverse:boolean):Single;
  PMeasurement=^TMeasurement;
  const MAXDWORD=$ffffffff;
var measurements:TList=nil;
I,selUnit:Integer;
TemperatureFormats:array[0..7]of TIdentMapEntry;
TempFormulas:array[1..3]of TTemperatureFormula;
x,m,b:Single;
inofile:tstringlist=nil;
CSVList:tstringlist=nil;
CSVReading:tstringlist=nil;
CSVHeader:TStringlist=nil;
input:string;
args:TStringlist;
lastdata:PMeasurement;

procedure quit(dwType:DWord);stdcall;
var p:PMeasurement;
begin
 if assigned(measurements)then begin
while measurements.Count>0do begin
p:=measurements.First;
dispose(P);
measurements.Remove(p);
end;
measurements.Free;
end;
 if assigned(inofile)then inofile.Free;
halt(ord(dwtype=maxdword-1));
end;
function LinearRegression(readings:tlist; out Slope, Intercept: Single): dword;
var
 I: Integer;
  SumX, SumY, SumXY, SumX2: Single;
begin
  Result := 1;
  if readings.Count < 2 then Exit;

  SumX := 0;
  SumY := 0;
  SumXY := 0;
  SumX2 := 0;

  // Calculate sums
  for I := 0 to readings.Count - 1 do
  begin
    SumX := SumX + PMeasurement(Readings[I])^.X;
    SumY := SumY + PMeasurement(Readings[I])^.Y;
    SumXY := SumXY + PMeasurement(Readings[I])^.X * PMeasurement(Readings[I])^.Y;
    SumX2 := SumX2 + PMeasurement(Readings[I])^.X * PMeasurement(Readings[I])^.X;
  end;
  result:=2;
  if IsZero(readings.Count * SumX2 - SumX * SumX)then exit;
  // Calculate slope (m) and intercept (b)
  Slope := (Readings.Count * SumXY - SumX * SumY) / (readings.Count * SumX2 - SumX * SumX);
  Intercept := (SumY - Slope * SumX) / readings.Count;

  Result := 0;
end;
function getaccuracy:integer;
var I:Integer;
y:Single;
begin
result:=0;
y:=0;
//get accuracy percentage by using average method
if measurements.Count<2then exit;
for I:= 0to measurements.Count-1do
y:=y+((1/(m*PMeasurement(measurements[I])^.X+b))/(1/PMeasurement(Measurements[I]
)^.Y));
result:=round((y/measurements.Count)*100);
end;
procedure calibrate;
var inocode:String;
begin
if LinearRegression(Measurements,m,b)>0then
raise exception.Create('Regression failed');
inocode:='//CALIBRATED_THERMISTOR_HERE'#13#10'calibrateThermistor('+floattostr(m)+','+floattostr(b)+');';
if args.IndexOfName('/ino')>-1then begin
inofile:=tstringlist.Create;
inofile.LoadFromFile(Args.Values['/ino']);
inofile.SaveToFile(changefileext(args.Values['/ino'],'._ino'));
inofile.Text:=stringReplace(inofile.Text,'//CALIBRATED_THERMISTOR_HERE',inocode,
[rfignorecase]);
inofile.SaveToFile(args.Values['/ino']);writeln(SysErrorMessage(0));quit(maxdword);
end;
writeln('Copy and paste this code to the top of setup() function');
writeln(inocode);
quit(MAXDWORD);
end;
function Kelvins(X:Single;Reverse:Boolean):Single;
begin
//since this is used to convert to and from Kelvins just return X
result:=x;
end;
function Celsius(X:Single;Reverse:Boolean):Single;
begin
if Reverse then result:=x - 273.15 else result:=x+273.15;
end;
function Fahrenheit(X:Single;Reverse:Boolean):Single;
begin
if Reverse then result:=((9*x)/5)-459.49 else result:=((5*x)/9)+255.272;
end;
function RoomTemp:single;
begin
result:=fahrenheit(68+(Random*10),false);
end;

procedure TThermistorCalc.DoRun;
label tryagain,redo;
begin

try
randomize;
temperatureformats[0].Value:=1;temperatureformats[0].Name:='K';
temperatureformats[1].Value:=2;temperatureformats[1].Name:='F';
temperatureformats[2].Value:=3;temperatureformats[2].Name:='C';
temperatureformats[3].Value:=1;temperatureformats[3].Name:='Kelvins';
temperatureformats[4].Value:=2;temperatureformats[4].Name:='Fahrenheit';
temperatureformats[5].Value:=3;temperatureformats[5].Name:='Celsius';
args:=tstringlist.create;
for I:=1to paramcount do args.add(paramstr(I));
if args.IndexOf('/?')>-1 then begin
writeln('Usage: ',extractfilename(Paramstr(0)),
' [/unit=degree_unit] "[/csv=csv_file]" "[/ino=Arduino_File]"');
writeln('Switches:');
writeln('/unit  Temperature unit, this can be "c" for Celsius, "f" for fahrenheit or "k" for kelvins you can use the names instead of the first letters');
writeln('/csv   comma seperated values file of Measurements. CSV header must have a "resistance" and "temperature" column');
writeln('/ino   Arduino sketch to calibrate to.');
writeln;
quit(maxdword-1);
end;
tempformulas[1]:=@kelvins;
tempformulas[2]:=@Fahrenheit;
tempformulas[3]:=@Celsius;
measurements:=tlist.Create;
if (args.indexofname('/unit')<0) then begin
writeln('Please choose the type of temperature measurements to use:');
writeln('1. Kelvins');
writeln('2. Fahrenheit');
writeln('3. Celsius');
tryagain:write('Please pick a number:');readln(Input);
selUnit:=strtointdef(input,0);
if not inrange(selUnit,1,3)then goto tryagain;
end else if not identtoint(args.values['/unit'],selUnit,temperatureformats)then
raise exception.CreateFmt('"%s" is not a valid temperature unit',[args.values['/unit']]);
if(args.IndexOfName('/csv')>-1)then begin
csvheader:=tstringlist.Create;csvlist:=tstringlist.Create;
csvreading:=tstringlist.Create;csvlist.loadfromfile(args.Values['/csv']);
if csvlist.count<3then raise exception.CreateFmt(
'CSV File needs 3 lines but has %d',[csvlist.count]);
csvheader.CommaText:=csvlist[0];if(csvheader.IndexOf('resistance')<0)then
raise exception.Create('Column "resistance" is missing in the header');
if(csvheader.IndexOf('temperature')<0)then
raise exception.Create('Column "temperature" is missing in the header');
for I:=1to csvlist.count-1do begin
new(lastdata);csvreading.CommaText:=csvlist[I];lastdata^.X:=ln(StrToFloat(
stringreplace(StringReplace(stringreplace(csvreading[csvheader.indexof('resistance')
],#32,emptystr,[rfreplaceall]),'K','e+3',[rfignorecase]),'M','e+6',[rfignorecase])));
if(pos('room',lowercase(csvreading[csvheader.IndexOf('temperature')]))=0)then
lastdata^.Y:=1/TempFormulas[selUnit](strtofloat(stringreplace(csvreading[
csvheader.indexof('temperature')],#32,emptystr,[rfreplaceall])),false)else
lastdata^.Y:=1/roomtemp;write('(',floattostr(lastdata^.x),',',floattostr(
lastdata^.Y),')');measurements.Add(Lastdata);if linearregression(measurements,m,
b)=0then write(' m=',floattostr(M),' b=',floattostr(b),#32,getaccuracy,'% accurite');writeln;
end;
calibrate;
quit(maxdword);
end;
writeln('Please enter the data that you collect,when done type the word "done"');
writeln('For a random room temperarture type in "room" for temperature measurement');
writeln('Also 1000 ohms can be written as(1k or 1K) and 2 Meg can be entered as(2M or 2m)');
while true do begin
new(LastData);redo:lastdata^.Y:=minSingle;
input:=emptystr;
write('Resistance measurement#',measurements.count+1,': ');readln(Input);
input:=stringreplace(Input,#32,emptystr,[rfReplaceAll]);
input:=stringreplace(Input,'k','e+3',[rfignorecase]);
input:=stringreplace(Input,'m','e+6',[rfignorecase]);
if(pos('done',lowercase(input))>0)then break;
if not trystrtofloat(input,x)then goto redo;lastdata^.X:=ln(x);
write('Temperature measurement#',measurements.count+1,': ');input:=emptystr;
readln(input);input:=stringreplace(Input,#32,emptystr,[rfReplaceAll]);
if not tryStrtofloat(Input,x) then begin
if(Pos('room',lowercase(input))>0)then lastdata^.Y:=1/roomtemp else
if(pos('done',lowercase(input))>0)then break else begin
writeln('Non numeric input or bad metric prefix.');
goto redo end;
end;
if pos('room',lowercase(Input))=0then begin
if not iszero(TempFormulas[selunit](x,false))then begin
lastdata^.Y:=1/TempFormulas[selunit](x,false);end else begin
writeln('That will divide by 0!');goto redo; end;
end;
measurements.Add(lastdata);
case linearRegression(Measurements,m,b)of
0:writeln('(',floattostr(lastdata^.X),',',floattostr(Lastdata^.Y),') m=',
floattostr(m),' b=',floattostr(b),#32,getaccuracy,'% accurite');
1:writeln('(',floattostr(lastdata^.x),',',floattostr(lastdata^.y),
') Need 1 more measurement');
2:Writeln('Regression Error#2');
end;
end;
calibrate;
except on e:Exception do begin writeln(e.classname,': ',e.message);quit(
MAXDWORD-1);end;end;
quit(maxdword);
end;

constructor TThermistorCalc.Create(TheOwner: TComponent);
begin
  inherited Create(TheOwner);
  StopOnException:=True;
end;

destructor TThermistorCalc.Destroy;
begin
quit(0);
  inherited Destroy;
end;

var
  Application: TThermistorCalc;
begin
  Application:=TThermistorCalc.Create(nil);
  Application.Title:='Thermistor Calibrator';
  Application.Run;
  Application.Free;
end.

