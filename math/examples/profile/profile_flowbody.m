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

lsb = 1/4E6/2^16;   % MAX3510x time value conversion factor 

profile = profile_flow( massflow_comport, refdes_comport, setpoints, sample_count, 'tracked' );

time_std = zeros(1,length(profile));
time_mean = zeros(1,length(profile));
flow_mean = zeros(1,length(profile));

for i=1:length(profile)
    up = profile{i}.tracked.up * lsb;
    down = profile{i}.tracked.down * lsb;
    delta_t = up - down;
    time_std(i) = std(delta_t);
    time_mean(i) = mean(delta_t);
    flow_mean(i) = mean( raw_flow( up, down ) );
end

profile_result = table( setpoints', time_mean', time_std', flow_mean' );
profile_result.Properties.VariableNames = {'SetPoint','Mean','StdDev','Flow' };

path( oldpath );
clear olddpath up down delta_t flow_mean time_mean time_std i;

