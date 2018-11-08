function o = sos( up, down )

% given the time-of-flight in the 'up' direction and the time of flight
% in the 'down', returns flow independant unitless speed-of-sound metric

    o = 1./down + 1./up;

end