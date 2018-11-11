% This is an example of how to use the various matlab functions to collect
% and work with data retrieved from the MAXREFDES169 firmware via USB
% serial port

%% Create a flow profile using the MAXREFDES169 and a mass flow regulator
%
% This section requires a Parker digital mass flow regulator
%
% setpoints include zero, and 0.4LPM to 42.0LPM in 0.1 LPM steps
% 100Hz sampling rate, with 500 samples per setpoint
% collected data using the firmware command 'report tracked'
%
% config is a structure that has two members:
%   massflow_comport
%   refdes_ccmport
%
%  These members are specific to your system.  Both are strings and specify
%  a system serial port,  for example 'COM3'

% setpoints = [0 0.4:.1:42.0]; % zero flow + entire Parker DMFR range

tracked = profile_flow( config, 0:2:42, 100, 100, 'tracked temp' );

%% Condense tracked data into a condensed time-of-flight (TOF) form for anaylysis
%
% This is an optional step and depends on what you want to do with the
% collected data.  In this example we are just interested in mean TOF and
% flow data and generating a compensation table.

tof = tracked_to_tof( tracked );
%tof.zfo = zero_flow_offset( tof.up(1), tof.down(1) ); % calculate zero-flow
%tof.flow = raw_flow( tof.up + tof.zfo, tof.down - tof.zfo ); % compensate for transducer offset
tof.flow = raw_flow( tof.up, tof.down ); % compensate for transducer offset

%% Use TOF data to create flow compensation table that can be compiled into the kit firmware
%
% This will create 'flow_mpli_autogen.h'.  To use, copy the file into the root
% source directory and recompile the kit firmware.

tof_to_mpli( tof, 'flow_mpli_autogen.h' );

%% Plot the flow profile

plot( tof.setpoint, tof.flow );