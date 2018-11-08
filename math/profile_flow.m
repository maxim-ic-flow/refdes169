function o = profile_flow(mass_flow_comport, refdes_comport, setpoints, sample_count, report_flags )

% This function will profile the flowbody.
%
% setpoints is an array of flow setpoints that will be iteratively sent 
% to the mass flow controller.
%
% sample_count is the number of time measurments to be made at each
% setpoint
%
% report flags is a string to be supplied to the refdes 'report' command
%
% Example:
%   setpoints = 0.4:0.1:42.0;
%   p = profile_flow( setpoitns, 100, 'tracked temp' );

    % change the comports for your system
    
    mf = mass_flow(mass_flow_comport);
    rd = refdes(refdes_comport);
    
    for i=1:length(setpoints)
        mf.set_point(setpoints(i));
        pause(3);   % give the mass flow regulator some time to settle
        r = rd.report( sample_count, report_flags );
        o{i} = parse_report( r );
    end
    delete(rd);
    delete(mf);
end