function o = profile_flow(com_config, setpoints, sample_rate_hz, sample_count, flags )

% This function will profile the flowbody.
%
% setpoints is an array of flow setpoints that will be iteratively sent 
% to the mass flow controller.
%
% sample_count is the number of time measurments to be made at each
% setpoint
%
% sample_rate_hz is the sampling frequeny
%
% flags is a string to be supplied to the refdes 'report' command
%
% returns a cell array, one cell for each setpoint
% each cell contains data specific to which flags were specified
%
% Example:
%   setpoints = 0.4:0.1:42.0;
%   p = profile_flow( setpoints, 100, 200, 'tracked temp' );

    % change the comports for your system

    if ~exist('flags','var')
        flags = 'tracked';
    end
    try
        mf = mass_flow(com_config.massflow_comport);
    catch ME
        warning('mass flow controller comport is busy');
        return
    end
    try
        rd = refdes(com_config.refdes_comport);
    catch ME
        warning('refdes comport is busy');
        delete(mf);
        return
    end
    for i=1:length(setpoints)
        mf.set_point(setpoints(i));
        if( rd.tofsr( sample_rate_hz ) == false )
            o = {};
            break;
        end
        pause(5);   % give the mass flow regulator some time to settle
        r = rd.report( sample_count, flags );
        if( isempty(r) )
            o = {};
            break;
        end
        o{i} = parse_report( r );
        o{i}.setpoint = setpoints(i);
    end
    delete(rd);
    mf.set_point(0);
    delete(mf);
end
