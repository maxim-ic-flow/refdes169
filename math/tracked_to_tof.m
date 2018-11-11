function tof = tracked_to_tof( tracked_profile )
    
% condenses data produced by profile_flow.m ('report tracked temp'
% firmware command) into a generally more useful form

    len = length(tracked_profile);
    for i=1:len
       tof.up(i) = mean( tracked_profile{i}.tracked.up ); 
       tof.down(i) = mean( tracked_profile{i}.tracked.down );
       tof.up_period(i) = mean( tracked_profile{i}.tracked.up_period );
       tof.down_period(i) = mean( tracked_profile{i}.tracked.down_period );
       tof.setpoint(i) = tracked_profile{i}.setpoint;
       if( isfield( tracked_profile{i}, 'temperature' ) )
           % convert temperature data into celcius if it was collected
           tof.temperature(i) = mean( time2celcius( tracked_profile{i}.temperature.therm, tracked_profile{i}.temperature.reference ) );
       end
    end

end
