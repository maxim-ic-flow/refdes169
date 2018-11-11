% This is an example of how to generate a flow profile for a flowbody
% using a digital mass flow regulator and the MAXREFDES169
%
% Define the variables below or change the defaults below to suit
% your system and test requirements.

oldpath = path;
path(oldpath,'../..');

if ~exist('refdes_comport','var')
    refdes_comport = 'COM9';
end

if ~exist('massflow_comport','var')
    
    massflow_comport = 'COM15';
end

if ~exist('setpoints','var')
    setpoints = 1:1:5;
end

if ~exist('sample_count','var')
    sample_count = 100;
end

%%

profile = profile_flow( massflow_comport, refdes_comport, setpoints, sample_count, 'tracked' );

%%

lsb = 1/4E6/2^16;   % MAX3510x time value conversion factor 

time_std = zeros(1,length(profile));
time_mean = zeros(1,length(profile));
flow_mean = zeros(1,length(profile));

zfo = zero_flow_offset( mean(profile{1}.tracked.up), mean(profile{1}.tracked.down) );  % assuming first setpoint is zero flow

for i=1:length(profile)
    up = profile{i}.tracked.up + zfo;   % adjust directional delay offset
    down = profile{i}.tracked.down - zfo;
    delta_t = (up - down) * lsb;    % delta_t unit is 'seconds'
    time_std(i) = std(delta_t);
    time_mean(i) = mean(delta_t);
    flow_mean(i) = mean( raw_flow( up, down ) );  % flow mean is in terms of time-of-flight register values
end

profile_result = table( setpoints', time_mean', time_std', flow_mean' );
profile_result.Properties.VariableNames = {'SetPoint','Mean','StdDev','Flow' };

%  profile_result.Coefficient can be used by an interpolated linearization
%  algorithm to convert time-of-flight values into linear flow rates.
%  Linear flow rates can be converted into volumetric flow rates according
%  to the geometry of your flowbody

coef_header('..\..\..\flow_mpli_autogen.h', flow_mean, setpoints ); % generate c-file for use with the kit firmware

%%
path( oldpath );
clear olddpath up down delta_t flow_mean time_mean time_std i;

