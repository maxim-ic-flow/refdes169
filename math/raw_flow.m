function o = raw_flow( up, down )

% given the time-of-flight in the 'up' direction and the time of flight
% in the 'down', returns speed-of-sound independant unitless flow rate metric

    o = 1./down - 1./up;

end