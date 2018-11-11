function adj = zero_flow_offset( up_zero_flow, down_zero_flow )

% returns the offset to be added to the up and subtracted form down TOF 
% values returned by the MAX3510x to compensate for transducer 
% directional delay differential

% arguments are time-of-flight values recorded during zero flow conditions.
% this value is typically highly temperature dependant

    m = ( up_zero_flow + down_zero_flow ) / 2;
    adj = m - up_zero_flow;
   
end