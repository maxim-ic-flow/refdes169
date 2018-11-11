function out = mpli( setpoint, flow, in )

    % this function is the matlab implimentation of mpli_calc() in
    % mpli.c
    %
    % tof is a struct that must have 'setpoint' and 'flow' members
    
    len = length(setpoint);
    for i = 2:len
        if( in < flow(i) )
            x1 = flow(i-1);s
            x2 = flow(i);
            y1 = setpoint(i-1);
            y2 = setpoint(i);
            m = (y2-y1) / (x2-x1);
            out = y1 + (in-x1)*m;
            break;
        end
    end
end
