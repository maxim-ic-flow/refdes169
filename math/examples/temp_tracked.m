% This example reads data collected from the refdes169 via the USB serial
% interface using the 'report tracked temp' command and generates
% a graph of flow and temperature over time

% temp_tracked.csv is the serial data collected from the interface

oldpath = path;
path(oldpath,'..');

data = read_log( 'temp_tracked.csv' );

flow = raw_flow( data.tracked.up, data.tracked.down ); % this is unitless
% to convert raw flow into real flow with proper units requires a conversion 
% function specific to your flowbody

temperature = time2celcius( data.temperature.therm, data.temperature.reference );
% temperature assumes a 1K ohm RTD is used

figure
plot( data.tracked.ndx, flow );
yyaxis right
plot( data.temperature.ndx, temperature );

path( oldpath );
clear olddpath;
